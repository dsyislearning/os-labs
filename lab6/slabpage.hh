#ifndef SLABPAGE_HH
#define SLABPAGE_HH

#include <deque>

#include "buddy.hh"
#include "block.hh"

class SlabPage
{
private:
    Zone &zone;       // 伙伴系统区域
    Block *block_ptr; // 伙伴系统分配的内存块指针
    Block block;      // 伙伴系统分配的内存块
    int slab_size;
    int object_size;
    int object_num;
    std::vector<bool> used;

public:
    SlabPage(Zone &zone, int object_size);
    Block get_block();

    bool failed();
    int alloc();
    bool free();
    bool is_full();
    bool is_free();
    bool is_partial();

    std::string str();
};

#endif // SLABPAGE_HH
