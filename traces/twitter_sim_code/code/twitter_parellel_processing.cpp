#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

constexpr int NUM_THREADS = 96;
constexpr size_t MAX_MEMORY_LIMIT = 200ULL * 1024 * 1024 * 1024; // 200 GB
constexpr size_t CHUNK_SIZE = MAX_MEMORY_LIMIT / 4;              // Process smaller chunks for memory safety

std::atomic<size_t> processed_lines(0);
std::atomic<size_t> total_lines(0);

// **Accurate Progress Bar**
void print_progress() {
    while (processed_lines < total_lines) {
        float progress = (processed_lines.load() * 100.0) / total_lines.load();
        std::cerr << "\rProgress: " << progress << "% completed. (" << processed_lines << "/" << total_lines << " lines processed)" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cerr << "\rProgress: 100% completed. (" << processed_lines << "/" << total_lines << " lines processed)\n" << std::flush;
}

// **Process a chunk and write intermediate frequency results to a file**
void process_chunk(const char *data, size_t start, size_t end, int thread_id, const std::string &output_folder) {
    std::unordered_map<std::string, int> freq_map;
    std::string line, col1, key, col3, col4, col5, operation, col7;

    for (size_t pos = start; pos < end;) {
        size_t line_end = std::find(data + pos, data + end, '\n') - data;
        if (line_end >= end) break;

        line.assign(data + pos, data + line_end);
        pos = line_end + 1;

        std::istringstream linestream(line);
        if (std::getline(linestream, col1, ',') &&
            std::getline(linestream, key, ',') &&
            std::getline(linestream, col3, ',') &&
            std::getline(linestream, col4, ',') &&
            std::getline(linestream, col5, ',') &&
            std::getline(linestream, operation, ',') &&
            std::getline(linestream, col7)) {
            
            if (operation == "get") {
                freq_map[key]++;
            }
        }
        processed_lines++;
    }

    // Write intermediate results to disk
    std::string temp_file = output_folder + "/temp_freq_" + std::to_string(thread_id) + ".txt";
    std::ofstream temp_out(temp_file);
    for (const auto &[key, count] : freq_map) {
        temp_out << key << "," << count << "\n";
    }
    temp_out.close();

    // Free memory
    freq_map.clear();
}

// **Process file in parallel chunks**
void process_file_chunk(int fd, off_t chunk_start, off_t chunk_size, int thread_id, const std::string &output_folder) {
    char *data = static_cast<char *>(mmap(nullptr, chunk_size, PROT_READ, MAP_PRIVATE, fd, chunk_start));
    if (data == MAP_FAILED) {
        perror("Error mapping file chunk");
        return;
    }

    process_chunk(data, 0, chunk_size, thread_id, output_folder);
    munmap(data, chunk_size);
}

// **Merge Intermediate Results**
void merge_results(const std::string &output_folder, const std::string &freq_file, const std::string &key_map_file, const std::string &seq_file, const std::string &stats_file) {
    std::unordered_map<std::string, int> final_freq_map;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        std::string temp_file = output_folder + "/temp_freq_" + std::to_string(i) + ".txt";
        std::ifstream temp_in(temp_file);
        std::string line, key;
        int count;

        while (std::getline(temp_in, line)) {
            std::istringstream linestream(line);
            if (std::getline(linestream, key, ',') && (linestream >> count)) {
                final_freq_map[key] += count;
            }
        }
        temp_in.close();
        std::remove(temp_file.c_str());
    }

    // Sort by frequency
    std::vector<std::pair<std::string, int>> sorted_freq(final_freq_map.begin(), final_freq_map.end());
    tbb::parallel_sort(sorted_freq.begin(), sorted_freq.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    // Write final outputs
    std::ofstream freq_out(freq_file);
    std::ofstream key_map_out(key_map_file);
    std::ofstream seq_out(seq_file);
    std::ofstream stats_out(stats_file);

    int id_counter = 1;
    long total_requests = 0, unique_keys = sorted_freq.size();
    long max_freq = sorted_freq.empty() ? 0 : sorted_freq.front().second;
    long min_freq = sorted_freq.empty() ? 0 : sorted_freq.back().second;

    for (const auto &[key, count] : sorted_freq) {
        freq_out << key << "," << count << "\n";
        key_map_out << key << "," << id_counter++ << "\n";
        total_requests += count;
    }

    stats_out << "Total 'get' requests: " << total_requests << "\n";
    stats_out << "Unique keys: " << unique_keys << "\n";
    stats_out << "Most frequent key count: " << max_freq << "\n";
    stats_out << "Least frequent key count: " << min_freq << "\n";

    freq_out.close();
    key_map_out.close();
    seq_out.close();
    stats_out.close();
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <large_file_path> <output_folder>\n";
        return 1;
    }

    const char *file_path = argv[1];
    std::string output_folder = argv[2];
    mkdir(output_folder.c_str(), 0777);

    std::string freq_file = output_folder + "/freq.txt";
    std::string key_map_file = output_folder + "/key_map.txt";
    std::string seq_file = output_folder + "/seq.txt";
    std::string stats_file = output_folder + "/stats.txt";

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    // total_lines = std::count(std::istreambuf_iterator<char>(std::ifstream(file_path).rdbuf()), std::istreambuf_iterator<char>(), '\n');
    total_lines = 1500000000;
    processed_lines = 0;

    std::thread progress_thread(print_progress);

    tbb::parallel_for(0, NUM_THREADS, [&](int i) {
        off_t chunk_start = i * (file_size / NUM_THREADS);
        off_t chunk_size = std::min((file_size / NUM_THREADS), file_size - chunk_start);
        process_file_chunk(fd, chunk_start, chunk_size, i, output_folder);
    });

    progress_thread.join();
    merge_results(output_folder, freq_file, key_map_file, seq_file, stats_file);
    
    close(fd);
    return 0;
}
