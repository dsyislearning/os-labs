#include "slabcache.hh"
#include "slabpage.hh"

#include "utils.hh"
#include <string>

SlabCache::SlabCache(Zone &zone, int object_size)
    : zone(zone), object_size(object_size)
{
    SlabPage slabpage(zone, object_size);
    if (!slabpage.failed())
        slabs_free.push_back(slabpage);
}

int SlabCache::get_object_size()
{
    return object_size;
}

bool SlabCache::alloc()
{
    if (slabs_partial.empty() && slabs_free.empty())
    {
        SlabPage slabpage(zone, object_size);
        if (slabpage.failed())
        {
            Log("分配 ", object_size, "B 对象的缓存失败");
            return false;
        }
        slabs_free.push_back(slabpage);
    }

    if (slabs_partial.empty())
    {
        slabs_partial.push_back(slabs_free.front());
        slabs_free.pop_front();
    }

    SlabPage &slabpage = slabs_partial.front();
    int index = slabpage.alloc();
    if (index == -1)
    {
        // Log("分配 ", object_size, "dB 对象的缓存失败");
        return false;
    }
    if (slabpage.is_full())
    {
        slabs_full.push_back(slabpage);
        slabs_partial.pop_front();
    }

    // Log("分配 ", object_size, "B 对象的缓存成功");
    return true;
}

bool SlabCache::free()
{
    bool success = false;

    if (!slabs_full.empty())
    {
        SlabPage slabpage = slabs_full.front();

        success = slabpage.free();

        // Log("从 slabs_full 中释放一个对象");

        if (slabpage.is_partial())
        {
            slabs_full.pop_front();
            slabs_partial.push_back(slabpage);
        }
        else if (slabpage.is_free())
        {
            slabs_full.pop_front();
            slabs_free.push_back(slabpage);
        }
    }
    else if (!slabs_partial.empty())
    {
        SlabPage &slabpage = slabs_partial.front();

        success = slabpage.free();

        // Log("从 slabs_partial 中释放一个对象");

        if (slabpage.is_free())
        {
            slabs_partial.pop_front();
            slabs_free.push_back(slabpage);
        }
    }

    if (!slabs_free.empty())
    {
        SlabPage &slabpage = slabs_free.front();
        slabs_free.pop_front();

        // Log("从 slabs_free 中回收 slabpage");

        reap(slabpage);
    }

    return success;
}

void SlabCache::reap(SlabPage &slabpage)
{
    Block block = slabpage.get_block();
    zone.free(block);
    // Log("回收页块 ", block.str());
}

std::string SlabCache::str()
{
    std::string str = "slabcache[" + std::to_string(object_size) + "]:\n";
    str += "\tslabs_full: ";
    for (auto &slabpage : slabs_full)
    {
        str += slabpage.str();
    }
    str += "\n\tslabs_partial: ";
    for (auto &slabpage : slabs_partial)
    {
        str += slabpage.str();
    }
    str += "\n\tslabs_free: ";
    for (auto &slabpage : slabs_free)
    {
        str += slabpage.str();
    }
    str += "\n";
    return str;
}
