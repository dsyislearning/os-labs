#ifndef SLAB_HH
#define SLAB_HH

#include <deque>

#include "buddy.hh"
#include "slabcache.hh"

class Slab
{
private:
    Zone &zone; // 调用的伙伴系统区域

    std::deque<SlabCache> caches; // 缓存列表

    bool cache_exists(int size);    // 判断是否存在对应对象大小的缓存
    SlabCache *get_cache(int size); // 获取对应对象大小的缓存

public:
    Slab(Zone &zone);
    ~Slab();

    bool alloc(int object_size); // 从缓存中分配一个对象
    bool free(int object_size);  // 释放一个对象

    void print_slab(); // 打印缓存列表
};

#endif // SLAB_HH
