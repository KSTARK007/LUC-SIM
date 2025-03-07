#include "S3FIFOCache.hpp"
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

    access_count[key]++;

    // Move from small FIFO to main FIFO if access count reaches threshold
    auto sit = small_fifo_map.find(key);
    if (sit != small_fifo_map.end() && access_count[key] >= move_to_main_threshold)
    {
        small_fifo.erase(sit->second);
        small_fifo_map.erase(sit);
        main_fifo.push_front(key);
        main_fifo_map[key] = main_fifo.begin();
    }

    return it->second.first;
}

void S3FIFOCache::put(int key, int value)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache_map.find(key);

    if (it != cache_map.end()) // Key exists, update value
    {
        it->second.first = value;
        access_count[key]++;
        return;
    }

    // If the key is in the ghost list, promote it directly to main FIFO
    auto git = ghost_list_map.find(key);
    if (git != ghost_list_map.end())
    {
        ghost_list.erase(git->second);
        ghost_list_map.erase(git);
        main_fifo.push_front(key);
        main_fifo_map[key] = main_fifo.begin();
    }
    else
    {
        // Insert into small FIFO if space allows
        if (small_fifo.size() >= fifo_size)
        {
            evictFromSmallFIFO();
        }
        small_fifo.push_front(key);
        small_fifo_map[key] = small_fifo.begin();
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
    small_fifo_map.erase(evict_key);

    // Move to main FIFO if accessed enough
    if (access_count[evict_key] >= move_to_main_threshold)
    {
        main_fifo.push_front(evict_key);
        main_fifo_map[evict_key] = main_fifo.begin();
    }
    else
    {
        ghost_list.push_front(evict_key);
        ghost_list_map[evict_key] = ghost_list.begin();
        if (ghost_list.size() > ghost_size)
        {
            int remove_key = ghost_list.back();
            ghost_list.pop_back();
            ghost_list_map.erase(remove_key);
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
    main_fifo_map.erase(evict_key);
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

        // Remove from FIFO lists
        auto sit = small_fifo_map.find(key);
        if (sit != small_fifo_map.end())
        {
            small_fifo.erase(sit->second);
            small_fifo_map.erase(sit);
        }

        auto mit = main_fifo_map.find(key);
        if (mit != main_fifo_map.end())
        {
            main_fifo.erase(mit->second);
            main_fifo_map.erase(mit);
        }

        auto git = ghost_list_map.find(key);
        if (git != ghost_list_map.end())
        {
            ghost_list.erase(git->second);
            ghost_list_map.erase(git);
        }
    }
}
