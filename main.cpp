#include "ReplicaManager.hpp"
#include "RequestProcessor.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::atomic<int> completed_requests(0);

int main()
{
    ConfigManager config("config.json");

    completed_requests = 0;

    // config.printConfig();

    ReplicaManager manager(config);

    std::string folder_path = config.workload_folder; // Folder containing request files
    RequestProcessor requestProcessor(folder_path);

    // requestProcessor.processAllFilesParallel(manager);
    requestProcessor.processFirstFile(manager);

    int cache_percent = static_cast<int>(config.cache_percentage * 100);
    std::string is_rdma = config.rdma_enabled ? "rdma" : "no_rdma";
    std::string is_cba = config.enable_cba ? "cba" : "no_cba";
    std::string is_dedup = config.enable_de_duplication ? "dedup" : "no_dedup";
    std::string is_fixed_access_rate = config.is_access_rate_fixed ? "fixed" : "variable";

    // Extract workload and workload number from folder path
    size_t last_slash = folder_path.find_last_of("/");
    size_t second_last_slash = folder_path.find_last_of("/", last_slash - 1);
    std::string workload = (second_last_slash != std::string::npos) ? folder_path.substr(second_last_slash + 1, last_slash - second_last_slash - 1) : "unknown";
    std::string workload_number = (last_slash != std::string::npos) ? folder_path.substr(last_slash + 1) : "unknown";

    // Construct directory path: "workload/<workload>/<workload_number>/"
    std::string output_folder = "workload/" + workload + "/" + workload_number;
    fs::create_directories(output_folder);

    // Construct filename
    std::string filename = output_folder + "/workload_" + workload + "_" + workload_number + "_cache_" +
                           std::to_string(cache_percent) + "_" + is_rdma + "_" + is_cba + "_access_rate" +
                           is_fixed_access_rate + "_" + is_dedup + ".txt";
    manager.computeAndWriteMetrics(filename, config.cache_percentage, config.total_dataset_size);

    std::cout << "Simulation complete. Stats written to " << filename << "\n";

    return 0;
}
