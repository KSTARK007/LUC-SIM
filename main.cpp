#include "ReplicaManager.hpp"
#include "ConfigManager.hpp"
#include "RequestProcessor.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<int> completed_requests(0);

int main()
{
    ConfigManager config("config.json");

    int cache_size = static_cast<int>(config.cache_percentage * config.total_dataset_size);
    completed_requests = 0;

    // config.printConfig();

    ReplicaManager manager(config.num_replicas, cache_size, config.rdma_enabled, config.enable_cba,
                           config.latency_local, config.latency_rdma, config.latency_disk, config.update_interval, config.total_dataset_size);

    std::string folder_path = config.workload_folder; // Folder containing request files
    RequestProcessor requestProcessor(folder_path);

    // requestProcessor.processAllFilesParallel(manager);
    requestProcessor.processFirstFile(manager);

    std::string is_rdma = config.rdma_enabled ? "rdma" : "no_rdma";
    std::string is_cba = config.enable_cba ? "cba" : "no_cba";
    std::string filename = cache_size + "_cache_" + is_rdma + "_" + is_cba + "_simulation_stats.txt";
    manager.computeAndWriteMetrics(filename, config.cache_percentage, config.total_dataset_size);
    std::cout << "Simulation complete. Stats written to simulation_stats.txt\n";

    return 0;
}
