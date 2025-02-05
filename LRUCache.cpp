#include "LRUCache.hpp"

LRUCache::LRUCache(size_t cap) : capacity(cap) {}

int LRUCache::get(int key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache.find(key);
    if (it == cache.end())
        return -1;

    keys.splice(keys.begin(), keys, it->second.second);
    return it->second.first;
}

void LRUCache::put(int key, int value)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache.find(key);
    if (it != cache.end())
    {
        keys.splice(keys.begin(), keys, it->second.second);
        it->second.first = value;
        return;
    }

    if (cache.size() >= capacity)
    {
        int lru = keys.back();
        keys.pop_back();
        cache.erase(lru);
    }

    keys.push_front(key);
    cache[key] = {value, keys.begin()};
}

size_t LRUCache::size()
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache.size();
}

std::set<int> LRUCache::getKeys()
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    return std::set<int>(keys.begin(), keys.end());
}

bool LRUCache::contains(int key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache.find(key) != cache.end();
}
