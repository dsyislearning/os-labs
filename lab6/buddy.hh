#ifndef BUDDY_HH
#define BUDDY_HH

#include <vector>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

#include "block.hh"

class Zone
{
private:
    std::atomic<int> max_order;

    std::vector<std::deque<Block>> free_list;
    void insert_block(Block &block);
    Block *get_block(int order);
    std::recursive_mutex free_list_mutex;

    std::queue<Block> hot_pages_queue;
    std::atomic<int> lazy;
    std::mutex hot_pages_mutex;

    std::thread hot_pages_deamon_thread;
    void hot_pages_deamon();
    std::atomic<bool> hot_pages_deamon_running;

public:
    Zone(int order, int lazy);
    ~Zone();
    Block *alloc(int size, int owner);
    void free(Block &block);

    void test();
};

#endif // BUDDY_HH
