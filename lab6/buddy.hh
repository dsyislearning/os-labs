#ifndef BUDDY_HH
#define BUDDY_HH

#include <vector>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>

#include "block.hh"

class Zone
{
private:
    int max_order;
    std::mutex max_order_mutex;

    std::vector<std::deque<Block>> free_list;
    void insert_block(Block *block);
    Block *get_block(int order);
    std::mutex free_list_mutex;

    std::queue<Block> hot_pages_queue;
    const int lazy = 1;
    std::mutex hot_pages_mutex;
    std::thread hot_pages_deamon_thread;
    void hot_pages_deamon();

public:
    Zone(int order);
    ~Zone();
    Block *alloc(int size, int owner);
    void free(Block *block);
};

#endif // BUDDY_HH
