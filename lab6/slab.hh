#ifndef SLAB_HH
#define SLAB_HH

#include <deque>

#include "buddy.hh"
#include "slabcache.hh"

class Slab
{
private:
    Zone &zone;

    std::deque<SlabCache> caches;

    bool cache_exists(int size);
    SlabCache *get_cache(int size);

public:
    Slab(Zone &zone);
    ~Slab();
    
    bool alloc(int object_size);
    bool free(int object_size);

    void print_slab();
};

#endif // SLAB_HH
