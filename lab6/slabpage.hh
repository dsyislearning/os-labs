#ifndef SLABPAGE_HH
#define SLABPAGE_HH

#include <deque>

#include "buddy.hh"
#include "block.hh"

class SlabPage
{
private:
    Zone &zone;             // 伙伴系统区域
    Block *block_ptr;       // 伙伴系统分配的内存块指针
    Block block;            // 伙伴系统分配的内存块
    int slab_size;          // SlabPage 的大小，单位为字节
    int object_size;        // 该 SlabPage 管理的对象大小，单位为字节
    int object_num;         // 该 SlabPage 管理的对象数量
    std::vector<bool> used; // 该 SlabPage 中对象的使用情况

public:
    SlabPage(Zone &zone, int object_size);
    Block get_block();

    bool failed();     // 判断是否从伙伴系统分配内存失败
    int alloc();       // 从 SlabPage 中分配一个对象，返回对象的下标
    bool free();       // 释放一个对象
    bool is_full();    // 判断 SlabPage 是否完全分配
    bool is_free();    //  判断 SlabPage 是否完全未分配
    bool is_partial(); // 判断 SlabPage 是否部分分配

    std::string str();
};

#endif // SLABPAGE_HH
