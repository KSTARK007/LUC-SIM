#ifndef REPLICA_MANAGER_HPP
#define REPLICA_MANAGER_HPP

#include "Replica.hpp"
#include "Metrics.hpp"
#include "CostBenefitAnalyzer.hpp"
#include "ConfigManager.hpp"
#include "CacheBase.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <thread>
#include <atomic>

struct CompareAccessFrequency
{
    bool operator()(const std::tuple<uint64_t, std::string, uint64_t> &a,
                    const std::tuple<uint64_t, std::string, uint64_t> &b) const
    {
        return std::get<0>(a) > std::get<0>(b); // Sort in descending order by access frequency
    }
};

class ReplicaManager
{
private:
    std::vector<std::unique_ptr<Replica>> replicas;
    uint64_t dataset_size;
    std::mutex manager_mutex;
    uint64_t total_requests = 0;
    uint64_t total_misses = 0;
    uint64_t total_remote_fetches = 0;
    uint64_t failed_remote_fetches = 0;
    uint64_t total_hits = 0;
    uint64_t total_keys_admitted = 0;
    bool rdma_enabled = false;
    bool enable_cba = false;
    bool enable_de_duplication = false;
    bool is_access_rate_fixed = false;
    uint64_t R_opt = 0;
    std::unique_ptr<CostBenefitAnalyzer> cba;
    std::vector<int> replica_misses;
    std::vector<int> remote_fetches;
    std::vector<std::set<int>> cache_contents;
    std::map<int, uint64_t> access_frequencies;
    std::atomic<bool> stop_cba_thread;
    std::thread cba_thread;
    uint64_t update_interval;
    std::vector<int> best_optimal_redundancy;
    std::map<int, bool> dup_keys_map;
    uint64_t latency_local;
    uint64_t latency_rdma;
    uint64_t latency_disk;
    std::string workload_folder;
    std::string cache_type;

    void runCBAUpdater();

public:
    ReplicaManager(ConfigManager &config);
    ~ReplicaManager();
    int handleRequest(int key, int replica_id = -1);
    void computeAndWriteMetrics(const std::string &filename, float cache_pct, int total_dataset_size);
    void print_optimal_redundanc_to_file(std::string filename);
    int hashFunction(int key);
    void deDuplicateCache();
    void read_cdf_from_file(std::string filename);
    bool shouldCacheLocally(const std::string &key);
};

#endif // REPLICA_MANAGER_HPP
