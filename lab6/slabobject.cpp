#include "slabobject.hh"

SlabObject::SlabObject(int size, bool used)
    : size(size), used(used)
{
}

int SlabObject::get_size()
{
    return size;
}
