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
    std::atomic<int> total_requests;
    std::atomic<int> success;
    std::atomic<int> shortage_fail;
    std::atomic<int> fragment_fail;
    std::atomic<int> other_fail;
    std::atomic<int> full_pages;
    std::atomic<int> empty_pages;

    std::vector<std::deque<Block>> free_list;
    void insert_block(Block &block);
    Block *get_block(int order, int owner);
    std::recursive_mutex free_list_mutex;

    std::queue<Block> hot_pages_queue;
    std::atomic<int> lazy;
    std::mutex hot_pages_mutex;

    std::thread hot_pages_deamon_thread;
    void hot_pages_deamon();
    std::atomic<bool> hot_pages_deamon_running;

    bool because_of_shortage(int request);

    void print_free_list();
    void print_hot_pages_queue();
    void audit();

public:
    Zone(int order, int lazy);
    ~Zone();
    Block *alloc(int size, int owner);
    void free(Block &block);

    void test();
};

#endif // BUDDY_HH
