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
    float cache_percentage;
    bool rdma_enabled;
    bool enable_cba;
    bool enable_de_duplication;
    uint64_t update_interval;
    int latency_local;
    int latency_rdma;
    int latency_disk;
    std::string workload_folder;

    ConfigManager(const std::string &config_file);
    void loadConfig(const std::string &config_file);
    void printConfig();
};

#endif // CONFIG_MANAGER_HPP
