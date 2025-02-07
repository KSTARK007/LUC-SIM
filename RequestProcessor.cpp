#include "RequestProcessor.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <vector>
#include <filesystem>
#include <regex>
#include <charconv>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;
extern std::atomic<int> completed_requests;

void displayProgressBar(int total_requests)
{
    const int bar_width = 50;
    while (completed_requests < total_requests)
    {
        float progress = static_cast<float>(completed_requests) / total_requests;
        int pos = static_cast<int>(bar_width * progress);

        std::cout << "[";
        for (int i = 0; i < bar_width; ++i)
        {
            if (i < pos)
                std::cout << "=";
            else if (i == pos)
                std::cout << ">";
            else
                std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << "%\r";
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[==================================================] 100%\n";
}

// Regex pattern to validate file format
bool RequestProcessor::isValidFileFormat(const std::string &filename)
{
    // std::regex pattern(R"(client_\d+_thread_\d+\_clientPerThread_\d+\.txt)");
    std::regex pattern(R"(seq.txt)");
    return std::regex_match(filename, pattern);
}

RequestProcessor::RequestProcessor(const std::string &folder) : folder_path(folder) {}

// Load requests from a file into a vector
std::vector<std::pair<int, int>> RequestProcessor::loadRequestsFromFile(const std::string &file_path)
{
    std::vector<std::pair<int, int>> requests;
    std::ifstream file(file_path);
    int total_requests = 0;
    if (!file)
    {
        std::cerr << "Error: Could not open file " << file_path << "\n";
        return requests;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        int key, replica;
        std::string operation; // Ignore this column

        if (!(iss >> key >> replica >> operation))
        {
            std::cerr << "Warning: Invalid format in file " << file_path << ": " << line << "\n";
            continue;
        }

        requests.emplace_back(key, replica);
        // if (requests.size() >= 100000)
        //     break;
    }
    file.close();
    std::cout << "Loaded " << requests.size() << " requests from " << file_path << "\n";
    return requests;
}

std::vector<std::pair<int, int>> RequestProcessor::loadRequestsFromTracesFile(const std::string &file_path)
{
    std::vector<std::pair<int, int>> requests;

    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        std::cerr << "Error: Could not open file " << file_path << "\n";
        return requests;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        std::cerr << "Error: Invalid file size\n";
        close(fd);
        return requests;
    }

    char *data = static_cast<char *>(mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    close(fd);

    if (data == MAP_FAILED)
    {
        std::cerr << "Error: Memory mapping failed: " << strerror(errno) << "\n";
        return requests;
    }

    madvise(data, file_size, MADV_SEQUENTIAL | MADV_WILLNEED);

    requests.resize(file_size / 10);

    char *ptr = data;
    char *end = data + file_size;
    size_t index = 0;

    const size_t PROGRESS_STEP = file_size / 100; // Update progress every 1% read
    size_t next_progress = PROGRESS_STEP;         // Next update threshold

    while (ptr < end)
    {
        char *line_start = ptr;
        ptr = static_cast<char *>(memchr(ptr, '\n', end - ptr));

        if (!ptr)
            break;

        *ptr++ = '\0';

        char *end_ptr;
        unsigned long long key = std::strtoull(line_start, &end_ptr, 10);

        if (line_start == end_ptr)
            continue;

        requests[index++] = {static_cast<int>(key), 0};

        // **Update progress bar**
        if (ptr - data >= next_progress)
        {
            int percentage = static_cast<int>((ptr - data) * 100 / file_size);
            std::cout << "\r[";
            int bar_width = 50; // Progress bar width
            int pos = bar_width * percentage / 100;

            for (int i = 0; i < bar_width; ++i)
                std::cout << (i < pos ? '#' : ' ');

            std::cout << "] " << percentage << "%";
            std::cout.flush();

            next_progress += PROGRESS_STEP;
        }
    }

    requests.resize(index);
    munmap(data, file_size);

    std::cout << "\nLoaded " << requests.size() << " requests from " << file_path << " using optimized mmap()\n";
    return requests;
}

// Process all valid files in the folder in parallel
void RequestProcessor::processAllFilesParallel(ReplicaManager &manager)
{
    std::vector<std::thread> threads;

    for (const auto &entry : fs::directory_iterator(folder_path))
    {
        if (entry.is_regular_file())
        {
            std::string file_path = entry.path().string();
            std::string filename = entry.path().filename().string();

            if (!isValidFileFormat(filename))
            {
                std::cerr << "Skipping invalid file: " << filename << "\n";
                continue;
            }

            // Launch a new thread for each valid file
            threads.emplace_back([&, file_path]()
                                 {
                std::vector<std::pair<int, int>> requests = loadRequestsFromFile(file_path);
                
                for (const auto &[key, replica] : requests)
                {

                    manager.handleRequest(key, replica);
                    completed_requests++;
                }
                
                std::cout << "Completed processing file: " << file_path << "\n"; });
        }
    }

    // Wait for all threads to finish
    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }
    std::cout << "All files processed\n";
}

// Process only the first valid file in the folder
void RequestProcessor::processFirstFile(ReplicaManager &manager)
{
    for (const auto &entry : fs::directory_iterator(folder_path))
    {
        if (entry.is_regular_file())
        {
            std::string file_path = entry.path().string();
            std::string filename = entry.path().filename().string();

            if (!isValidFileFormat(filename))
            {
                std::cerr << "Skipping invalid file: " << filename << "\n";
                continue;
            }

            std::vector<std::pair<int, int>> requests = loadRequestsFromTracesFile(file_path);
            std::thread progress_thread(displayProgressBar, requests.size());

            for (const auto &[key, replica] : requests)
            {
                manager.handleRequest(key);
                completed_requests++;
            }
            progress_thread.join();
            std::cout << "Completed processing file: " << file_path << "\n";
            break; // Process only the first valid file and exit
        }
    }
}