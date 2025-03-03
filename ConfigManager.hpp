#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include "nlohmann/json.hpp"

class ConfigManager
{
public:
    int num_threads;
    int num_replicas;
    int total_dataset_size;
    int requests_per_thread;
    int cache_size;
    float cache_percentage;
    bool rdma_enabled;
    bool enable_cba;
    bool enable_de_duplication;
    bool is_access_rate_fixed;
    uint64_t update_interval;
    uint64_t fixed_access_rate;
    int latency_local;
    int latency_rdma;
    int latency_disk;
    std::string workload_folder;
    std::string cache_type;

    ConfigManager(const std::string &config_file);
    void loadConfig(const std::string &config_file);
    void printConfig();
    void updateCacheSize();
};

#endif // CONFIG_MANAGER_HPP
