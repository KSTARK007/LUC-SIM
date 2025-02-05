#ifndef REQUEST_PROCESSOR_HPP
#define REQUEST_PROCESSOR_HPP

#include "ReplicaManager.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <regex>

class RequestProcessor
{
private:
    std::string folder_path;
    uint64_t total_requests = 0;

    bool isValidFileFormat(const std::string &filename);
    std::vector<std::pair<int, int>> loadRequestsFromFile(const std::string &file_path);

public:
    RequestProcessor(const std::string &folder);

    void processAllFilesParallel(ReplicaManager &manager);
    void processFirstFile(ReplicaManager &manager);
};

#endif // REQUEST_PROCESSOR_HPP
