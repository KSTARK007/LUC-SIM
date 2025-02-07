#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<uint64_t> extract_keys(const std::string &filename, size_t total_keys = 2000000)
{
    std::vector<uint64_t> keys;
    std::ifstream file(filename);
    uint64_t key;
    char node, op;
    int count = 0;
    // while (file >> key >> node >> op)
    while (file >> key)
    {
        if (count++ >= total_keys)
            break;
        keys.push_back(key);
    }
    file.close();
    return keys;
}

std::vector<std::pair<uint64_t, std::string>> sort_keys_by_frequency(const std::vector<uint64_t> &keys, size_t total_keys = 200000)
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

    for (uint64_t i = 1; i <= total_keys; i++)
    {
        if (frequency.find(i) == frequency.end())
        {
            freq_vector.emplace_back(0, std::to_string(i));
        }
    }

    return freq_vector;
}

std::vector<std::pair<std::string, uint64_t>> sum_cdf(const std::vector<std::pair<uint64_t, std::string>> &freq_vector, const std::string &filename)
{

    // std::ofstream file(filename);
    uint64_t total = 0, sum = 0;
    std::vector<std::pair<std::string, uint64_t>> c_sum;

    for (const auto &kv : freq_vector)
    {
        sum += kv.first;
        c_sum.emplace_back(kv.second, sum);
    }

    return c_sum;
}

void print_cdf(const std::vector<std::pair<std::string, uint64_t>> &cdf)
{
    for (const auto &kv : cdf)
    {
        std::cout << kv.first << " " << kv.second << std::endl;
    }
}

std::vector<std::pair<std::string, uint64_t>> load_workload(const std::string &filepath, const std::string &cdf_filename, size_t dataset_size = 200000)
{
    std::cout << "Loading workload from: " << filepath << std::endl;
    auto keys = extract_keys(filepath);
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Extracted keys from file\n << Size of keys: " << keys.size() << std::endl;
    auto freq_vector = sort_keys_by_frequency(keys, dataset_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken to sort keys: " << elapsed.count() << "s\n";
    auto c_sum = sum_cdf(freq_vector, cdf_filename);
    std::cout << "Finished computing cdf\n"
              << "Size of cdf: " << c_sum.size() << std::endl;
    // print_cdf(c_sum);
    return c_sum;
}

std::vector<std::pair<std::string, uint64_t>> load_sorted_cdf(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    std::vector<std::pair<uint64_t, std::string>> cdf;
    std::string line, key;
    uint64_t freq;

    std::cout << "Loading cdf from: " << filename << std::endl;

    while (std::getline(file, line))
    {
        size_t comma_pos = line.find(',');
        if (comma_pos == std::string::npos)
        {
            throw std::runtime_error("Invalid line format: " + line);
        }

        key = line.substr(0, comma_pos);
        freq = std::stoull(line.substr(comma_pos + 1));

        // std::cout << key << " " << freq << std::endl;
        cdf.emplace_back(freq, key);
    }

    file.close();
    auto c_sum = sum_cdf(cdf, "hotspot_cdf.txt");
    return c_sum;
}

uint64_t get_sum_freq_till_index(const std::vector<std::pair<std::string, uint64_t>> &cdf, uint64_t start, uint64_t end)
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

uint64_t calculate_performance(const std::vector<std::pair<std::string, uint64_t>> &cdf, uint64_t water_mark_local, uint64_t water_mark_remote, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg)
{
    uint64_t total_local = get_sum_freq_till_index(cdf, 0, water_mark_local);
    uint64_t total_remote = get_sum_freq_till_index(cdf, water_mark_local, water_mark_local + water_mark_remote);
    uint64_t total_disk = get_sum_freq_till_index(cdf, water_mark_local + water_mark_remote, cdf.size());
    uint64_t latency = (total_local * cache_ns_avg) + (((2 * total_remote / 3) * rdma_ns_avg) + (total_remote / 3 * cache_ns_avg)) + (total_disk * disk_ns_avg);
    return latency ? std::numeric_limits<uint64_t>::max() / latency : 0;
}

size_t find_optimal_access_rates(std::vector<std::pair<std::string, uint64_t>> &cdf, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg, uint64_t cache_size)
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
        // if (local % 10000 == 0)
        //     std::cout << "Local: " << local << ", Remote: " << remote << ", Performance: " << performance << std::endl;
    }
    std::cout << "Best local: " << best_local << ", Best remote: " << best_remote << ", Best performance: " << best_performance << std::endl;
    return best_local;
}

std::vector<uint64_t> extract_keys_windowed(const std::string &filename, size_t start_index, size_t window_size)
{
    std::vector<uint64_t> keys;
    std::ifstream file(filename);
    uint64_t key;
    size_t count = 0;

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    while (file >> key)
    {
        if (count >= start_index && count < start_index + window_size)
        {
            keys.push_back(key);
        }
        if (count >= start_index + window_size)
        {
            break;
        }
        count++;
    }

    file.close();
    return keys;
}

void process_workload_in_windows(const std::string &filepath, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg, uint64_t cache_size, size_t total_ops, size_t window_size)
{
    size_t start_index = 0;
    std::vector<uint64_t> cba_results;

    while (start_index < total_ops)
    {
        auto keys = extract_keys_windowed(filepath, start_index, window_size);
        if (keys.empty())
            break;

        std::cout << "\nProcessing keys from index " << start_index << " to " << start_index + keys.size() << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        auto freq_vector = sort_keys_by_frequency(keys, cache_size);
        auto cdf = sum_cdf(freq_vector, "hotspot_cdf.txt");
        auto best = find_optimal_access_rates(cdf, cache_ns_avg, disk_ns_avg, rdma_ns_avg, cache_size);
        auto end = std::chrono::high_resolution_clock::now();
        cba_results.push_back(best);

        std::chrono::duration<double> elapsed = end - start;
        // std::cout << "Time taken for this window: " << elapsed.count() << "s\n";

        start_index += window_size;
    }
    std::string cba_filename = "cba_results_" + std::to_string(window_size) + ".txt";
    auto cba_results_file = fs::path(filepath).parent_path() / cba_filename;

    std::ofstream file(cba_results_file);
    for (size_t i = 0; i < cba_results.size(); i++)
    {
        file << cba_results[i] << std::endl;
    }
    file.close();
}

void process_cdf_direct(const std::string &cdf_filename, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg, uint64_t cache_size)
{
    auto cdf = load_sorted_cdf(cdf_filename);
    auto start = std::chrono::high_resolution_clock::now();
    find_optimal_access_rates(cdf, cache_ns_avg, disk_ns_avg, rdma_ns_avg, cache_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << "s\n";
}

void process_workload_fully(const std::string &filepath, uint64_t cache_ns_avg, uint64_t disk_ns_avg, uint64_t rdma_ns_avg, uint64_t cache_size, size_t total_keys)
{
    auto cdf = load_workload(filepath, "hotspot_cdf.txt", total_keys);
    auto start = std::chrono::high_resolution_clock::now();
    find_optimal_access_rates(cdf, cache_ns_avg, disk_ns_avg, rdma_ns_avg, cache_size);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << "s\n";
}

int main(int argc, char *argv[])
{
    uint64_t disk_latency = 100, cache_latency = 1, rdma_latency = 50, cache_size = 4467939;
    size_t window_size = 10000000;
    int twitter_wokload = 7;
    if (argc > 1)
    {
        if (argc < 3)
        {
            std::cout << "Usage: " << argv[0] << " <window_size> <twitter_workload number>" << std::endl;
            return 1;
        }
        window_size = std::stoull(argv[1]);
        twitter_wokload = std::stoi(argv[2]);
    }
    std::string workload_folder = "/mydata/twitter/" + std::to_string(twitter_wokload) + "/seq.txt";
    if (!fs::exists(workload_folder))
    {
        std::cout << "Workload file not found: " << workload_folder << std::endl;
        return 1;
    }

    // process_cdf_direct("/mydata/twitter/7/freq.txt", cache_latency, disk_latency, rdma_latency, cache_size);

    // process_workload_fully("/mydata/twitter/7/seq.txt", cache_latency, disk_latency, rdma_latency, cache_size, 2000000);

    process_workload_in_windows(workload_folder, cache_latency, disk_latency, rdma_latency, cache_size, 851370550, 10000000);

    return 0;
}
