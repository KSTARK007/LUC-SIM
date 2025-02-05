#include "Replica.hpp"

Replica::Replica(int replica_id, size_t cache_size) : id(replica_id), cache(cache_size) {}

int Replica::processRequest(int key)
{
    return cache.get(key);
}

bool Replica::hasKey(int key)
{
    return cache.contains(key);
}