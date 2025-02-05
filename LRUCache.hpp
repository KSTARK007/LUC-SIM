#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>
#include <set>

class LRUCache
{
private:
    size_t capacity;
    std::list<int> keys;
    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> cache;
    mutable std::mutex cache_mutex;

public:
    LRUCache(size_t cap);
    int get(int key);
    void put(int key, int value);
    size_t size();
    std::set<int> getKeys();
    bool contains(int key);
};

#endif // LRU_CACHE_HPP
