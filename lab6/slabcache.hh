#ifndef SlabCache_HH
#define SlabCache_HH

#include <deque>

#include "buddy.hh"
#include "slabpage.hh"

class SlabCache
{
private:
    Zone &zone;                         // 调用的伙伴系统区域
    int object_size;                    // 该缓存管理的对象大小
    std::deque<SlabPage> slabs_full;    // 完全分配的 SlabPage
    std::deque<SlabPage> slabs_partial; // 部分分配的 SlabPage
    std::deque<SlabPage> slabs_free;    // 未分配的 SlabPage

    void reap(SlabPage &slabpage); // 回收 SlabPage 给伙伴系统

public:
    SlabCache(Zone &zone, int object_size);
    int get_object_size(); // 获取管理的对象大小
    bool alloc();          // 从缓存中分配一个对象
    bool free();           // 释放一个对象

    std::string str();
};

#endif // SlabCache_HH
