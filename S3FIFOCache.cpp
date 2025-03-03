#include "S3FIFOCache.hpp"
#include <bits/algorithmfwd.h>
#include <algorithm>
#include <iostream>

S3FIFOCache::S3FIFOCache(size_t cap, double fifo_ratio, double ghost_ratio, int threshold)
    : capacity(cap), move_to_main_threshold(threshold)
{
    fifo_size = static_cast<size_t>(capacity * fifo_ratio);
    ghost_size = static_cast<size_t>(capacity * ghost_ratio);
}

int S3FIFOCache::get(int key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache_map.find(key);
    if (it == cache_map.end())
        return -1; // Cache miss

    // If found in the cache, increase the access count
    access_count[key]++;

    // If it's in the small FIFO, move to the main FIFO on threshold
    if (std::find(small_fifo.begin(), small_fifo.end(), key) != small_fifo.end())
    {
        if (access_count[key] >= move_to_main_threshold)
        {
            small_fifo.remove(key);
            main_fifo.push_front(key);
        }
    }
    return it->second.first;
}

void S3FIFOCache::put(int key, int value)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache_map.find(key);

    if (it != cache_map.end()) // Key already exists, update value
    {
        it->second.first = value;
        access_count[key]++;
        return;
    }

    // If the key was in the ghost list, promote it directly to the main FIFO
    if (std::find(ghost_list.begin(), ghost_list.end(), key) != ghost_list.end())
    {
        ghost_list.remove(key);
        main_fifo.push_front(key);
    }
    else
    {
        // Insert into small FIFO if space allows
        if (small_fifo.size() >= fifo_size)
        {
            evictFromSmallFIFO();
        }
        small_fifo.push_front(key);
    }

    cache_map[key] = {value, small_fifo.begin()};
    access_count[key] = 1;
}

void S3FIFOCache::evictFromSmallFIFO()
{
    if (small_fifo.empty())
        return;

    int evict_key = small_fifo.back();
    small_fifo.pop_back();

    // If the object has been accessed more than threshold, move it to main FIFO
    if (access_count[evict_key] >= move_to_main_threshold)
    {
        main_fifo.push_front(evict_key);
    }
    else
    {
        ghost_list.push_front(evict_key);
        if (ghost_list.size() > ghost_size)
        {
            ghost_list.pop_back();
        }
    }
    cache_map.erase(evict_key);
    access_count.erase(evict_key);
}

void S3FIFOCache::evictFromMainFIFO()
{
    if (main_fifo.empty())
        return;

    int evict_key = main_fifo.back();
    main_fifo.pop_back();
    cache_map.erase(evict_key);
    access_count.erase(evict_key);
}

size_t S3FIFOCache::size()
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache_map.size();
}

std::set<int> S3FIFOCache::getKeys()
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    std::set<int> keys;
    for (const auto &kv : cache_map)
    {
        keys.insert(kv.first);
    }
    return keys;
}

bool S3FIFOCache::contains(int key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache_map.find(key) != cache_map.end();
}

void S3FIFOCache::remove(int key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache_map.find(key);
    if (it != cache_map.end())
    {
        cache_map.erase(it);
        access_count.erase(key);
    }
}
