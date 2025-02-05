#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

std::vector<uint64_t> extract_keys(const std::string &filename, size_t total_keys = 100000)
{
    std::vector<uint64_t> keys;
    std::ifstream file(filename);
    uint64_t key;
    char node, op;
    while (file >> key >> node >> op)
    {
        keys.push_back(key);
    }
    file.close();
    return keys;
}

std::vector<std::pair<uint64_t, std::string>> sort_keys_by_frequency(const std::vector<uint64_t> &keys)
{
    std::map<uint64_t, uint64_t> frequency;
    for (uint64_t key : keys)
    {
        frequency[key]++;
    }

    std::vector<std::pair<uint64_t, std::string>> freq_vector;
    for (const auto &kv : frequency)
    {
        freq_vector.emplace_back(kv.second, std::to_string(kv.first));
    }

    std::sort(freq_vector.begin(), freq_vector.end(), std::greater<>());

    for (uint64_t i = 1; i <= 100000; i++)
    {
        if (frequency.find(i) == frequency.end())
        {
            freq_vector.emplace_back(0, std::to_string(i));
        }
    }

    return freq_vector;
}

std::vector<std::pair<uint64_t, uint64_t>> sum_cdf(const std::vector<std::pair<uint64_t, std::string>> &freq_vector, const std::string &filename)
{

    // std::ofstream file(filename);
    uint64_t total = 0, sum = 0;
    std::vector<std::pair<uint64_t, uint64_t>> c_sum;

    for (const auto &kv : freq_vector)
    {
        sum += kv.first;
        c_sum.emplace_back(stoi(kv.second), sum);
    }

    return c_sum;
}

void print_cdf(const std::vector<std::pair<uint64_t, uint64_t>> &cdf)
{
    for (const auto &kv : cdf)
    {
        std::cout << kv.first << " " << kv.second << std::endl;
    }
}

std::vector<std::pair<uint64_t, uint64_t>> load_workload(const std::string &filepath, const std::string &cdf_filename)
{
    auto keys = extract_keys(filepath);
    auto start = std::chrono::high_resolution_clock::now();
    auto freq_vector = sort_keys_by_frequency(keys);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken to sort keys: " << elapsed.count() << "s\n";
    auto c_sum = sum_cdf(freq_vector, cdf_filename);
    // print_cdf(c_sum);
    return c_sum;
}

uint64_t get_sum_freq_till_index(const std::vector<std::pair<uint64_t, uint64_t>> &cdf, uint64_t start, uint64_t end)
{
    if (start >= cdf.size())
    {
        return 1;
    }

    if (end > cdf.size())
    {
        return 1;
    }

    uint64_t start_value = start ? cdf[start - 1].second : 0;
    uint64_t end_value = cdf[end - 1].second;
    if (end_value == start_value)
        return 1;
    return end_value - start_value;
}

uint64_t calculate_performance(const std::vector<std::pair<uint64_t, uint64_t>> &cdf, uint64_t water_mark_local, uint64_t water_mark_remote, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg)
{
    uint64_t total_local = get_sum_freq_till_index(cdf, 0, water_mark_local);
    uint64_t total_remote = get_sum_freq_till_index(cdf, water_mark_local, water_mark_local + water_mark_remote);
    uint64_t total_disk = get_sum_freq_till_index(cdf, water_mark_local + water_mark_remote, cdf.size());
    uint64_t latency = (total_local * cache_ns_avg) + (((2 * total_remote / 3) * rdma_ns_avg) + (total_remote / 3 * cache_ns_avg)) + (total_disk * disk_ns_avg);
    return latency ? std::numeric_limits<uint64_t>::max() / latency : 0;
}

void find_optimal_access_rates(std::vector<std::pair<uint64_t, uint64_t>> &cdf, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg, uint64_t cache_size)
{
    uint64_t best_performance = 0, best_local = 0, best_remote = cache_size;
    for (uint64_t local = 0; local < cdf.size(); local++)
    {
        uint64_t remote = cache_size - (3 * local);
        if (remote < 0 || local > cache_size / 3)
            break;
        uint64_t performance = calculate_performance(cdf, local, remote, cache_ns_avg, disk_ns_avg, rdma_ns_avg);
        if (performance > best_performance)
        {
            best_performance = performance;
            best_local = local;
            best_remote = remote;
        }
    }
    std::cout << "Best local: " << best_local << ", Best remote: " << best_remote << ", Best performance: " << best_performance << std::endl;
}

int main()
{
    uint64_t disk_latency = 100, cache_latency = 1, rdma_latency = 10, cache_size = 100000;
    // auto cdf = load_workload("/vectordb1/ycsb/zipfian_0.99/client_0_thread_0_clientPerThread_0.txt", "hotspot_cdf.txt");
    // auto cdf = load_workload("/vectordb1/ycsb/uniform/client_0_thread_0_clientPerThread_0.txt", "hotspot_cdf.txt");
    auto cdf = load_workload("/vectordb1/ycsb/hotspot_80_20/client_0_thread_0_clientPerThread_0.txt", "hotspot_cdf.txt");
    auto start = std::chrono::high_resolution_clock::now();
    find_optimal_access_rates(cdf, cache_latency, disk_latency, rdma_latency, cache_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << "s\n";
    return 0;
}
