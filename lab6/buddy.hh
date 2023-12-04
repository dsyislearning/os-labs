#ifndef BUDDY_HH
#define BUDDY_HH

#include <vector>
#include <deque>
#include <queue>

#include "block.hh"

class Zone
{
private:
    int max_order;
    std::vector<std::deque<Block>> free_list;
    std::queue<Block> hot_pages;

public:
    Zone(int order);
    Block *alloc(int size, int owner);
    void free(Block *block);
};

#endif // BUDDY_HH
