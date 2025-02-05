#include "Metrics.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

Metrics::Metrics(const std::vector<std::set<int>> &key_sets, float cache_pct, int num_keys_seen,
                 float overall_miss_ratio, float remote_miss_ratio, float local_miss_ratio,
                 const std::vector<float> &replica_miss_ratios, int total_keys_admitted)
    : overall_miss_ratio(overall_miss_ratio), remote_miss_ratio(remote_miss_ratio),
      local_dup_miss_ratio(local_miss_ratio), replica_miss_ratios(replica_miss_ratios), total_keys_admitted(total_keys_admitted)
{

    int nsets = key_sets.size();
    std::set<int> keys_union, keys_intersect;
    for (const auto &s : key_sets)
    {
        keys_union.insert(s.begin(), s.end());
        if (keys_intersect.empty())
        {
            keys_intersect = s;
        }
        else
        {
            std::set<int> temp;
            std::set_intersection(keys_intersect.begin(), keys_intersect.end(),
                                  s.begin(), s.end(),
                                  std::inserter(temp, temp.begin()));
            keys_intersect = std::move(temp);
        }
    }

    int total_size = std::accumulate(key_sets.begin(), key_sets.end(), 0,
                                     [](int sum, const std::set<int> &s)
                                     { return sum + s.size(); });

    dataset_coverage = static_cast<float>(keys_union.size()) / num_keys_seen;
    replica_utilization = dataset_coverage / std::min(1.0f, nsets * cache_pct);

    int nunique = keys_union.size();
    sorensen_similarity = (nsets / static_cast<float>(nsets - 1)) * (1.0f - (static_cast<float>(nunique) / total_size));

    float avg_cache_size = static_cast<float>(total_size) / nsets;
    overlap_pct = avg_cache_size > 0 ? static_cast<float>(keys_intersect.size()) / avg_cache_size : 0;

    std::unordered_map<int, int> key_counts;
    for (const auto &s : key_sets)
    {
        for (int k : s)
        {
            key_counts[k]++;
        }
    }
}

void Metrics::writeToFile(const std::string &filename) const
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return;
    }

    file << "Sorensen Similarity: " << sorensen_similarity << "\n"
         << "Dataset Coverage: " << dataset_coverage << "\n"
         << "Replica Utilization: " << replica_utilization << "\n"
         << "Overall Miss Ratio: " << overall_miss_ratio << "\n"
         << "Remote Hit Ratio: " << remote_miss_ratio << "\n"
         << "Local Miss Ratio: " << local_dup_miss_ratio << "\n"
         << "Individual Replica Miss Ratios: ";
    for (float ratio : replica_miss_ratios)
    {
        file << ratio << " ";
    }
    file << "\n";
    file << "Total Keys Admitted: " << total_keys_admitted << "\n";
    file.close();
}
