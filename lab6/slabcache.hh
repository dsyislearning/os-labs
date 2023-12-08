#ifndef SlabCache_HH
#define SlabCache_HH

#include <deque>

#include "buddy.hh"
#include "slabpage.hh"

class SlabCache
{
private:
    Zone &zone;
    int object_size;
    std::deque<SlabPage> slabs_full;
    std::deque<SlabPage> slabs_partial;
    std::deque<SlabPage> slabs_free;

    void reap(SlabPage &slabpage);

public:
    SlabCache(Zone &zone, int object_size);
    int get_object_size();
    bool alloc();
    bool free();

    std::string str();
};

#endif // SlabCache_HH
