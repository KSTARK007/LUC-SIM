#ifndef REPLICA_HPP
#define REPLICA_HPP

#include "LRUCache.hpp"

class Replica
{
public:
    int id;
    LRUCache cache;
    Replica(int replica_id, size_t cache_size);

    // Delete copy constructor and copy assignment operator
    Replica(const Replica &) = delete;
    Replica &operator=(const Replica &) = delete;

    int processRequest(int key);
    bool hasKey(int key);
};

#endif // REPLICA_HPP
