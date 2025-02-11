#include "ReplicaManager.hpp"
#include <chrono>
#include <algorithm>

ReplicaManager::ReplicaManager(int num_replicas, size_t cache_size, bool rdma_enabled, bool enable_cba, bool enable_de_duplication,
                               uint64_t latency_local, uint64_t latency_rdma, uint64_t latency_disk, uint64_t update_interval, uint64_t dataset_size)
    : replica_misses(num_replicas, 0), remote_fetches(num_replicas, 0), cache_contents(num_replicas),
      rdma_enabled(rdma_enabled), enable_cba(enable_cba), stop_cba_thread(false), update_interval(update_interval),
      dataset_size(dataset_size), latency_local(latency_local), latency_rdma(latency_rdma), latency_disk(latency_disk),
      enable_de_duplication(enable_de_duplication)
{
    for (int i = 0; i < num_replicas; ++i)
    {
        replicas.emplace_back(std::make_unique<Replica>(i, cache_size));
    }
    if (enable_cba)
    {
        cba = std::make_unique<CostBenefitAnalyzer>(num_replicas, dataset_size, cache_size, latency_local, latency_rdma, latency_disk);
    }
}

ReplicaManager::~ReplicaManager()
{
    if (enable_cba)
    {
        stop_cba_thread = true;
        if (cba_thread.joinable())
        {
            cba_thread.join();
        }
    }
}

int ReplicaManager::handleRequest(int key, int replica_id)
{
    std::lock_guard<std::mutex> lock(manager_mutex);
    total_requests++;
    if (total_requests % update_interval == 0 && enable_cba)
    {
        runCBAUpdater();
    }
    int primary_replica_id;
    if (replica_id == -1)
    {
        primary_replica_id = rand() % replicas.size();
    }
    else
    {
        primary_replica_id = replica_id - 1;
    }
    int result = replicas[primary_replica_id]->processRequest(key);

    // Track access frequency
    access_frequencies[key]++;

    // Case 1: Key found in primary replica (Hit)
    if (result != -1)
    {
        return result;
    }

    if (rdma_enabled)
    {
        // Case 2: Key not found, check other replicas (Remote Fetch)
        for (int i = 0; i < replicas.size(); ++i)
        {
            if (i == primary_replica_id)
                continue;

            if (replicas[i]->hasKey(key))
            {
                total_remote_fetches++;
                remote_fetches[i]++;

                // CBA checks if this key should be cached locally
                if (enable_cba && cba->shouldCacheLocally(std::to_string(key)))
                {
                    total_keys_admitted++;
                    replicas[primary_replica_id]->cache.put(key, key);
                }
                return key;
            }
        }
    }

    // Case 3: Key does not exist in any replica (Miss)
    replicas[primary_replica_id]->cache.put(key, key);
    total_misses++;
    replica_misses[primary_replica_id]++;
    failed_remote_fetches++;
    return -1;
}

void ReplicaManager::computeAndWriteMetrics(const std::string &filename, float cache_pct, int total_dataset_size)
{
    std::vector<float> miss_ratios;
    for (int misses : replica_misses)
    {
        miss_ratios.push_back(static_cast<float>(misses) / total_requests);
    }

    float overall_miss_ratio = static_cast<float>(total_misses + total_remote_fetches) / total_requests;
    float tmp = static_cast<float>(total_remote_fetches) / total_requests;
    float remote_miss_ratio = static_cast<float>(tmp) / overall_miss_ratio;
    float local_miss_ratio = static_cast<float>(total_misses) / total_requests;

    float total_latency = (total_misses * latency_disk) +
                          (total_remote_fetches * latency_rdma) +
                          ((total_requests - total_misses - total_remote_fetches) * latency_local);

    float avg_latency = total_latency / total_requests;

    // Store cache contents for metrics tracking
    for (int i = 0; i < replicas.size(); ++i)
    {
        cache_contents[i] = replicas[i]->cache.getKeys();
    }

    Metrics metrics(cache_contents, cache_pct, total_dataset_size, overall_miss_ratio, remote_miss_ratio, local_miss_ratio, miss_ratios, total_keys_admitted, avg_latency);
    metrics.writeToFile(filename);
    std::string modified_filename = "optimal_redundancy_" + filename;
    print_optimal_redundanc_to_file(modified_filename);
}

void ReplicaManager::runCBAUpdater()
{
    std::vector<std::pair<uint64_t, uint64_t>> sorted_frequencies;

    {
        // std::lock_guard<std::mutex> lock(manager_mutex);
        if (access_frequencies.empty())
        {
            return;
        }

        for (const auto &kv : access_frequencies)
        {
            sorted_frequencies.emplace_back(kv.second, kv.first);
        }

        std::sort(sorted_frequencies.begin(), sorted_frequencies.end(), std::greater<>());

        for (uint64_t i = 1; i <= dataset_size; i++)
        {
            if (access_frequencies.find(i) == access_frequencies.end())
            {
                sorted_frequencies.emplace_back(0, i);
            }
        }
        access_frequencies.clear();
    }

    // Update CBA with new frequencies
    if (enable_cba && cba)
    {
        // std::lock_guard<std::mutex> lock(manager_mutex);
        cba->updateAccessFrequencies(sorted_frequencies);
        auto start_time = std::chrono::high_resolution_clock::now();
        best_optimal_redundancy.push_back(cba->find_optimal_access_rates());
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "Time taken to compute optimal redundancy: " << elapsed.count() << "s\n";
        // std::cout << "Optimal redundancy level: " << best_optimal_redundancy.back() << std::endl;
        if (enable_de_duplication)
        {
            deDuplicateCache();
        }
    }
    cba->reset();
    return;
}

void ReplicaManager::print_optimal_redundanc_to_file(std::string filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Could not open file optimal_redundancy.txt\n";
        return;
    }

    for (int redundancy : best_optimal_redundancy)
    {
        file << redundancy << std::endl;
    }
    file.close();
}
int ReplicaManager::hashFunction(int key)
{
    return key % replicas.size();
}

void ReplicaManager::deDuplicateCache()
{
    // std::lock_guard<std::mutex> lock(manager_mutex);

    for (int i = 0; i < replicas.size(); ++i)
    {
        std::vector<int> keysToRemove;
        std::set<int> currentKeys = replicas[i]->cache.getKeys();

        for (int key : currentKeys)
        {
            int assigned_replica = hashFunction(key);

            // If this key does not belong to this replica and is not needed anymore, remove it
            if (assigned_replica != i && !(enable_cba && cba->shouldCacheLocally(std::to_string(key))))
            {
                keysToRemove.push_back(key);
            }
        }

        // Remove unnecessary keys
        for (int key : keysToRemove)
        {
            replicas[i]->cache.remove(key);
        }
    }
}
