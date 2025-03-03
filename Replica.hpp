#ifndef REPLICA_HPP
#define REPLICA_HPP

#include "CacheBase.hpp"
#include "LRUCache.hpp"
#include "S3FIFOCache.hpp"
#include <memory>
#include <iostream>

class Replica
{
public:
    int id;
    std::unique_ptr<CacheBase> cache;

    Replica(int replica_id, size_t cache_size, const std::string &cache_type);

    Replica(const Replica &) = delete;
    Replica &operator=(const Replica &) = delete;

    int processRequest(int key);
    bool hasKey(int key);
};

#endif // REPLICA_HPP
