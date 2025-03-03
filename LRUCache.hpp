#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include "CacheBase.hpp"
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>
#include <set>

class LRUCache : public CacheBase
{
private:
    size_t capacity;
    std::list<int> keys;
    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> cache;
    mutable std::mutex cache_mutex;

public:
    LRUCache(size_t cap);
    int get(int key) override;
    void put(int key, int value) override;
    size_t size() override;
    std::set<int> getKeys() override;
    bool contains(int key) override;
    void remove(int key) override;
};

#endif // LRU_CACHE_HPP
