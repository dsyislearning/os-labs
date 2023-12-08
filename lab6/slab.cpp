#include "slab.hh"

#include "utils.hh"

Slab::Slab(Zone &zone)
    : zone(zone)
{
}

Slab::~Slab()
{
}

bool Slab::cache_exists(int size)
{
    for (auto &cache : caches)
    {
        if (cache.get_object_size() == size)
        {
            return true;
        }
    }

    return false;
}

SlabCache *Slab::get_cache(int size)
{
    for (auto it = caches.begin(); it != caches.end(); ++it)
    {
        SlabCache *cache = &(*it);

        if (cache->get_object_size() == size)
        {
            return cache;
        }
    }

    return nullptr;
}

bool Slab::alloc(int object_size)
{
    if (!cache_exists(object_size)) // 如果没有对应对象大小的缓存，就创建一个
    {
        caches.push_back(SlabCache(zone, object_size));
        // Log("创建 ", object_size, "B 对象的缓存");
    }

    SlabCache *cache = get_cache(object_size); // 获取对应对象大小的缓存
    // Log("获取 ", object_size, "B 对象的缓存");

    return cache->alloc(); // 从缓存中分配一个对象
}

bool Slab::free(int object_size)
{
    SlabCache *cache = get_cache(object_size); // 获取对应对象大小的缓存
    // Log("获取 ", object_size, "B 对象的缓存");

    if (cache != nullptr)
    {
        return cache->free(); // 释放对象
    }
    else
    {
        return false;
    }
}

void Slab::print_slab()
{
    Log("slab:");
    for (auto &cache : caches)
    {
        std::cout << cache.str();
    }
}
