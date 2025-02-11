#ifndef METRICS_HPP
#define METRICS_HPP

#include <vector>
#include <set>
#include <unordered_map>
#include <numeric>
#include <fstream>

class Metrics
{
private:
    float overall_miss_ratio;
    float remote_miss_ratio;
    float local_dup_miss_ratio;
    float dataset_coverage;
    float replica_utilization;
    float sorensen_similarity;
    float overlap_pct;
    float avg_latency;
    int total_keys_admitted;
    std::vector<float> replica_miss_ratios;

public:
    Metrics(const std::vector<std::set<int>> &key_sets, float cache_pct, int num_keys_seen,
            float overall_miss_ratio, float remote_miss_ratio, float local_miss_ratio,
            const std::vector<float> &replica_miss_ratios, int total_keys_admitted, float avg_latency);

    void writeToFile(const std::string &filename) const;
};

#endif // METRICS_HPP
