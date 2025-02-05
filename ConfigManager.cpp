#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ConfigManager::ConfigManager(const std::string &config_file)
{
    loadConfig(config_file);
}

void ConfigManager::loadConfig(const std::string &config_file)
{
    std::ifstream file(config_file);
    if (!file)
    {
        std::cerr << "Error: Could not open config file " << config_file << "\n";
        exit(1);
    }

    json config;
    file >> config;

    num_threads = config["num_threads"];
    num_replicas = config["num_replicas"];
    total_dataset_size = config["total_dataset_size"];
    requests_per_thread = config["requests_per_thread"];
    cache_percentage = config["cache_percentage"];
    rdma_enabled = config["rdma_enabled"];
    enable_cba = config["enable_cba"];
    update_interval = config.value("cba_update_interval", 10);
    latency_local = config["latency_local"];
    latency_rdma = config["latency_rdma"];
    latency_disk = config["latency_disk"];
    workload_folder = config["workload_folder"];
}

void ConfigManager::printConfig()
{
    std::cout << "Configuration:\n"
              << "  Number of threads: " << num_threads << "\n"
              << "  Number of replicas: " << num_replicas << "\n"
              << "  Total dataset size: " << total_dataset_size << "\n"
              << "  Requests per thread: " << requests_per_thread << "\n"
              << "  Cache percentage: " << cache_percentage << "\n"
              << "  RDMA enabled: " << (rdma_enabled ? "true" : "false") << "\n"
              << "  CBA enabled: " << (enable_cba ? "true" : "false") << "\n"
              << "  CBA update interval: " << update_interval << " seconds\n"
              << "  Local latency: " << latency_local << " us\n"
              << "  RDMA latency: " << latency_rdma << " us\n"
              << "  Disk latency: " << latency_disk << " us\n"
              << "  Workload folder: " << workload_folder << "\n";
}