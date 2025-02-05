#ifndef COST_BENEFIT_ANALYZER_HPP
#define COST_BENEFIT_ANALYZER_HPP

#include <vector>
#include <tuple>
#include <map>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <limits>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>

class CostBenefitAnalyzer
{
private:
    uint64_t num_replicas;
    uint64_t total_dataset_size;
    uint64_t cache_size;

    std::vector<std::pair<uint64_t, uint64_t>> access_frequencies;
    std::vector<std::pair<uint64_t, uint64_t>> c_sum;
    uint64_t latency_local;
    uint64_t latency_rdma;
    uint64_t latency_disk;
    uint64_t R_opt;

    static constexpr uint64_t NUM_THREADS = 80;
    std::mutex result_mutex;
    std::atomic<double> best_performance;

    std::map<int, bool> dup_keys_map;

    static void computeOptimalRedundancyThread(const std::pair<uint64_t, uint64_t> *access_ptr,
                                               uint64_t start_R, uint64_t end_R, uint64_t C, uint64_t N, uint64_t D,
                                               uint64_t latency_local, uint64_t latency_rdma, uint64_t latency_disk,
                                               std::atomic<uint64_t> &best_R, std::atomic<double> &best_performance,
                                               std::mutex &result_mutex);

public:
    CostBenefitAnalyzer(uint64_t num_replicas, uint64_t total_dataset_size, uint64_t cache_size,
                        uint64_t latency_local, uint64_t latency_rdma, uint64_t latency_disk);

    void updateAccessFrequencies(const std::vector<std::pair<uint64_t, uint64_t>> &new_frequencies);
    bool shouldCacheLocally(const std::string &key);
    uint64_t getOptimalRedundancy() const;
    void update_dup_keys_map();
    void sum_cdf();
    uint64_t get_sum_freq_till_index(uint64_t start, uint64_t end);
    uint64_t calculate_performance(uint64_t water_mark_local, uint64_t water_mark_remote, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg);
    uint64_t find_optimal_access_rates();
    void reset()
    {
        R_opt = 0;
        best_performance = std::numeric_limits<double>::lowest();
    }
};

#endif // COST_BENEFIT_ANALYZER_HPP
