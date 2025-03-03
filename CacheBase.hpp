#ifndef CACHE_BASE_HPP
#define CACHE_BASE_HPP

#include <set>
#include <cstddef>

class CacheBase
{
public:
    virtual ~CacheBase() = default;
    virtual int get(int key) = 0;
    virtual void put(int key, int value) = 0;
    virtual size_t size() = 0;
    virtual std::set<int> getKeys() = 0;
    virtual bool contains(int key) = 0;
    virtual void remove(int key) = 0;
};

#endif // CACHE_BASE_HPP
