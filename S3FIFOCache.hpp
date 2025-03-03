#ifndef S3_FIFO_CACHE_HPP
#define S3_FIFO_CACHE_HPP

#include "CacheBase.hpp"
#include <unordered_map>
#include <list>
#include <mutex>

class S3FIFOCache : public CacheBase
{
private:
    size_t capacity;
    size_t fifo_size;
    size_t ghost_size;
    int move_to_main_threshold;

    std::list<int> small_fifo;
    std::list<int> main_fifo;
    std::list<int> ghost_list;

    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> cache_map;
    std::unordered_map<int, int> access_count;

    mutable std::mutex cache_mutex;

public:
    S3FIFOCache(size_t cap, double fifo_ratio = 0.1, double ghost_ratio = 0.9, int threshold = 2);
    int get(int key) override;
    void put(int key, int value) override;
    size_t size() override;
    std::set<int> getKeys() override;
    bool contains(int key) override;
    void remove(int key) override;

private:
    void evictFromSmallFIFO();
    void evictFromMainFIFO();
};

#endif // S3_FIFO_CACHE_HPP
