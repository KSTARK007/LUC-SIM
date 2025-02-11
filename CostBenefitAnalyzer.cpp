#include "CostBenefitAnalyzer.hpp"
#include <algorithm>
#include <fstream>
#include <numeric>
#include <chrono>

extern std::atomic<int> completed_requests;

CostBenefitAnalyzer::CostBenefitAnalyzer(uint64_t num_replicas, uint64_t total_dataset_size, uint64_t _cache_size,
                                         uint64_t latency_local, uint64_t latency_rdma, uint64_t latency_disk)
    : num_replicas(num_replicas), total_dataset_size(total_dataset_size), cache_size(total_dataset_size),
      latency_local(latency_local), latency_rdma(latency_rdma), latency_disk(latency_disk),
      R_opt(0), best_performance(std::numeric_limits<double>::lowest())
{
    std::cout << "Cache size: " << cache_size << std::endl;
    std::cout << "Total dataset size: " << total_dataset_size << std::endl;
    std::cout << "Latency local: " << latency_local << std::endl;
    std::cout << "Latency rdma: " << latency_rdma << std::endl;
    std::cout << "Latency disk: " << latency_disk << std::endl;
    std::cout << "Num replicas: " << num_replicas << std::endl;
}

void CostBenefitAnalyzer::updateAccessFrequencies(const std::vector<std::pair<uint64_t, uint64_t>> &new_frequencies)
{
    access_frequencies = new_frequencies;
    sum_cdf();
}

bool CostBenefitAnalyzer::shouldCacheLocally(const std::string &key)
{
    return dup_keys_map[std::stoi(key)];
}

uint64_t CostBenefitAnalyzer::getOptimalRedundancy() const
{
    return R_opt;
}

void CostBenefitAnalyzer::update_dup_keys_map()
{
    dup_keys_map.clear();
    for (uint64_t i = 0; i < R_opt && i < access_frequencies.size(); ++i)
    {
        dup_keys_map[access_frequencies[i].second] = true;
    }
}

void CostBenefitAnalyzer::sum_cdf()
{

    uint64_t sum = 0;
    c_sum.clear();
    for (const auto &kv : access_frequencies)
    {
        sum += kv.first;
        c_sum.emplace_back(kv.second, sum);
    }
}

uint64_t CostBenefitAnalyzer::get_sum_freq_till_index(uint64_t start, uint64_t end)
{
    if (start >= c_sum.size())
    {
        return 1;
    }

    if (end > c_sum.size())
    {
        return 1;
    }

    uint64_t start_value = start ? c_sum[start - 1].second : 0;
    uint64_t end_value = c_sum[end - 1].second;
    if (end_value == start_value)
        return 1;
    return end_value - start_value;
}

uint64_t CostBenefitAnalyzer::calculate_performance(uint64_t water_mark_local, uint64_t water_mark_remote, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg)
{
    uint64_t total_local = get_sum_freq_till_index(0, water_mark_local);
    uint64_t total_remote = get_sum_freq_till_index(water_mark_local, water_mark_local + water_mark_remote);
    uint64_t total_disk = get_sum_freq_till_index(water_mark_local + water_mark_remote, c_sum.size());
    uint64_t latency = (total_local * cache_ns_avg) + (((2 * total_remote / 3) * rdma_ns_avg) + (total_remote / 3 * cache_ns_avg)) + (total_disk * disk_ns_avg);
    return latency ? std::numeric_limits<uint64_t>::max() / latency : 0;
}

uint64_t CostBenefitAnalyzer::find_optimal_access_rates()
{
    uint64_t best_performance = 0, best_local = 0, best_remote = cache_size;
    for (uint64_t local = 0; local < c_sum.size(); local++)
    {
        uint64_t remote = cache_size - (3 * local);
        if (remote < 0 || local > cache_size / 3)
            break;
        uint64_t performance = calculate_performance(local, remote, latency_local, latency_disk, latency_rdma);
        // if (local % 10000 == 0)
        //     std::cout << "Local: " << local << ", Remote: " << remote << ", Performance: " << performance << std::endl;
        if (performance > best_performance)
        {
            best_performance = performance;
            best_local = local;
            best_remote = remote;
        }
    }
    std::cout << "Best local: " << best_local << ", Best remote: " << best_remote << ", Best performance: " << best_performance << std::endl;
    R_opt = best_local;
    // if (R_opt == 1)
    // {
    //     update_dup_keys_map();
    //     print_cdf_to_file();
    //     std::cout << "Optimal redundancy level: " << R_opt << " exiting to debug" << std::endl;
    //     exit(0);
    // }
    update_dup_keys_map();
    return R_opt;
}

void CostBenefitAnalyzer::print_cdf_to_file()
{
    std::ofstream file("hotspot_cdf.txt");
    for (const auto &kv : c_sum)
    {
        file << kv.first << " " << kv.second << std::endl;
    }
    file.close();
    std::ofstream file2("access_frequencies.txt");
    for (const auto &kv : access_frequencies)
    {
        file2 << kv.first << " " << kv.second << std::endl;
    }
    file2.close();

    std::ofstream file3("dup_keys_map.txt");
    for (const auto &kv : dup_keys_map)
    {
        file3 << kv.first << " " << kv.second << std::endl;
    }
    file3.close();
    std::cout << completed_requests << " requests completed\n";
}