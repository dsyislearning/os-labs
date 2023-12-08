#include "slabpage.hh"

#include <numeric>

SlabPage::SlabPage(Zone &zone, int object_size)
    : zone(zone), object_size(object_size), block_ptr(nullptr), used()
{
    slab_size = (PAGE_SIZE * object_size) / std::gcd(PAGE_SIZE, object_size);
    object_num = slab_size / object_size;

    int page_num = slab_size / PAGE_SIZE > 0 ? slab_size / PAGE_SIZE : 1;

    block_ptr = zone.alloc(page_num, -1);

    if (block_ptr != nullptr)
    {
        block = Block{*block_ptr};
        used.resize(object_num, false);
    }
}

bool SlabPage::failed()
{
    return block_ptr == nullptr;
}

int SlabPage::alloc()
{
    if (failed())
    {
        return -1;
    }

    for (int i = 0; i < object_num; i++)
    {
        if (!used[i])
        {
            used[i] = true;
            return i;
        }
    }

    return -1;
}

Block SlabPage::get_block()
{
    return block;
}

bool SlabPage::free()
{
    if (failed())
    {
        return false;
    }

    for (int i = 0; i < object_num; i++)
    {
        if (used[i])
        {
            used[i] = false;
            return true;
        }
    }

    return false;
}

bool SlabPage::is_full()
{
    if (failed())
    {
        return false;
    }

    for (int i = 0; i < object_num; i++)
    {
        if (!used[i])
        {
            return false;
        }
    }

    return true;
}

bool SlabPage::is_free()
{
    if (failed())
    {
        return false;
    }

    for (int i = 0; i < object_num; i++)
    {
        if (used[i])
        {
            return false;
        }
    }

    return true;
}

bool SlabPage::is_partial()
{
    if (failed())
    {
        return false;
    }

    return !is_full() && !is_free();
}

std::string SlabPage::str()
{
    std::string str = "slabpage{";
    int used = 0;
    for (int i = 0; i < object_num; i++)
    {
        if (this->used[i])
        {
            used++;
        }
    }
    str += std::to_string(used) + "/" + std::to_string(object_num) + "}";    
    return str;
}
