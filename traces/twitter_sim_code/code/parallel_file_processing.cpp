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

tbb::concurrent_unordered_map<std::string, int> freq_map;
std::unordered_map<std::string, int> key_to_id;
std::mutex key_to_id_mutex;

std::atomic<size_t> processed_lines(0);
std::atomic<size_t> total_lines(0);

// **Accurate progress bar**
void print_progress()
{
    while (processed_lines < total_lines)
    {
        float progress = (processed_lines.load() * 100.0) / total_lines.load();
        std::cerr << "\rProgress: " << progress << "% completed. (" << processed_lines << "/" << total_lines << " lines processed)" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cerr << "\rProgress: 100% completed. (" << processed_lines << "/" << total_lines << " lines processed)\n"
              << std::flush;
}

// **Process a chunk of the file in parallel using TBB**
void process_chunk(const char *data, size_t start, size_t end, std::vector<std::string> &output_lines)
{
    std::string line, col1, key, col3, col4, col5, operation, col7;

    for (size_t pos = start; pos < end;)
    {
        size_t line_end = std::find(data + pos, data + end, '\n') - data;
        if (line_end >= end)
            break;

        line.assign(data + pos, data + line_end);
        pos = line_end + 1;

        std::istringstream linestream(line);
        if (std::getline(linestream, col1, ',') &&
            std::getline(linestream, key, ',') &&
            std::getline(linestream, col3, ',') &&
            std::getline(linestream, col4, ',') &&
            std::getline(linestream, col5, ',') &&
            std::getline(linestream, operation, ',') &&
            std::getline(linestream, col7))
        {
            if (operation == "get")
            {
                freq_map[key]++;
                output_lines.push_back(key);
            }
        }
        processed_lines++;
    }
}

// **Process file in manageable chunks**
void process_file_chunk(int fd, off_t chunk_start, off_t chunk_size, std::vector<std::vector<std::string>> &thread_outputs)
{
    char *data = static_cast<char *>(mmap(nullptr, chunk_size, PROT_READ, MAP_PRIVATE, fd, chunk_start));
    if (data == MAP_FAILED)
    {
        perror("Error mapping file chunk");
        return;
    }

    tbb::parallel_for(0, NUM_THREADS, [&](int i)
                      {
                          size_t thread_chunk_size = chunk_size / NUM_THREADS;
                          size_t start = i * thread_chunk_size;
                          size_t end = (i == NUM_THREADS - 1) ? chunk_size : (i + 1) * thread_chunk_size;

                          while (start > 0 && data[start] != '\n') start++;
                          while (end < chunk_size && data[end] != '\n') end++;

                          process_chunk(data, start, end, thread_outputs[i]); });

    munmap(data, chunk_size);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
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
    if (fd == -1)
    {
        perror("Error opening file");
        return 1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    // **Count total lines for accurate progress tracking**
    char *data = static_cast<char *>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (data == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        return 1;
    }
    total_lines = std::count(data, data + file_size, '\n');
    munmap(data, file_size);

    processed_lines = 0;

    std::vector<std::vector<std::string>> thread_outputs(NUM_THREADS);
    std::thread progress_thread(print_progress);

    for (off_t chunk_start = 0; chunk_start < file_size; chunk_start += CHUNK_SIZE)
    {
        off_t chunk_size = std::min(CHUNK_SIZE, static_cast<size_t>(file_size - chunk_start));
        process_file_chunk(fd, chunk_start, chunk_size, thread_outputs);
    }

    processed_lines.store(total_lines);
    progress_thread.join();

    std::cout << "Processing complete. Writing sorted frequency files...\n";

    std::vector<std::pair<std::string, int>> sorted_freq(freq_map.begin(), freq_map.end());
    tbb::parallel_sort(sorted_freq.begin(), sorted_freq.end(),
                       [](const auto &a, const auto &b)
                       { return a.second > b.second; });

    freq_map.clear();

    std::cout << "Sorting complete. Writing to files...\n";

    int id_counter = 1;
    for (const auto &[key, _] : sorted_freq)
    {
        key_to_id[key] = id_counter++;
    }

    std::ofstream freq_out(freq_file);
    std::ofstream key_map_out(key_map_file);
    std::ofstream output_seq(seq_file);
    std::ofstream stats_out(stats_file);

    long total_requests = 0;
    long unique_keys = sorted_freq.size();
    long max_freq = sorted_freq.empty() ? 0 : sorted_freq.front().second;
    long min_freq = sorted_freq.empty() ? 0 : sorted_freq.back().second;

    for (const auto &[key, count] : sorted_freq)
    {
        freq_out << key << "," << count << "\n";
        key_map_out << key << "," << key_to_id[key] << "\n";
        total_requests += count;
    }

    sorted_freq.clear();

    stats_out << "Trace Statistics:\n";
    stats_out << "  - Total 'get' requests: " << total_requests << "\n";
    stats_out << "  - Unique keys: " << unique_keys << "\n";
    stats_out << "  - Most frequent key count: " << max_freq << "\n";
    stats_out << "  - Least frequent key count: " << min_freq << "\n";

    for (const auto &lines : thread_outputs)
    {
        for (const auto &key : lines)
        {
            output_seq << key_to_id[key] << "\n";
        }
    }

    key_to_id.clear();

    freq_out.close();
    key_map_out.close();
    output_seq.close();
    stats_out.close();
    close(fd);

    std::cout << "Processing complete! Files saved in folder: " << output_folder << "\n";

    return 0;
}
