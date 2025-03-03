#include "Replica.hpp"

Replica::Replica(int replica_id, size_t cache_size, const std::string &cache_type)
    : id(replica_id)
{
    if (cache_type == "S3FIFO")
    {
        cache = std::make_unique<S3FIFOCache>(cache_size);
        std::cout << "Replica " << id << "created with S3FIFO cache\n";
    }
    else if (cache_type == "LRU")
    {
        cache = std::make_unique<LRUCache>(cache_size);
        std::cout << "Replica " << id << "created with LRU cache\n";
    }
    else
    {
        throw std::invalid_argument("Unsupported cache type: " + cache_type);
    }
}

int Replica::processRequest(int key)
{
    return cache->get(key);
}

bool Replica::hasKey(int key)
{
    return cache->contains(key);
}
