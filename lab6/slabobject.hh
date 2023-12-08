#ifndef SLABOBJECT_HH
#define SLABOBJECT_HH

class SlabObject
{
private:
    int size;
    bool used;

public:
    SlabObject(int size, bool used);
    int get_size();
};

#endif // SLABOBJECT_HH
