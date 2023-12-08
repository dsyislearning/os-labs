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
    std::atomic<int> max_order;      // 该区域物理连续页面阶数，即最大可分配的页面数为 2^max_order
    std::atomic<int> total_requests; // 统计总共的请求次数
    std::atomic<int> success;        // 统计成功分配的次数
    std::atomic<int> shortage_fail;  // 统计因为伙伴系统区域的页面数不足而无法分配的次数
    std::atomic<int> fragment_fail;  // 统计因为伙伴系统区域的碎片而无法分配的次数
    std::atomic<int> other_fail;     // 统计因为其他原因无法分配的次数
    std::atomic<int> full_pages;     // 统计伙伴系统区域中已分配的页面数
    std::atomic<int> empty_pages;    // 统计伙伴系统区域中未分配的页面数

    std::vector<std::deque<Block>> free_list; // 伙伴系统区域的空闲链表
    void insert_block(Block &block);          // 将 block 插入到空闲链表中
    Block *get_block(int order, int owner);   // 从空闲链表中获取一个 order 阶的 block 分配给 owner 进程
    std::recursive_mutex free_list_mutex;     // 伙伴系统区域的空闲链表的互斥锁

    std::queue<Block> hot_pages_queue; // 热页面队列
    std::atomic<int> lazy;             // 热页面队列的惰性阈值
    std::mutex hot_pages_mutex;        // 热页面队列的互斥锁

    std::thread hot_pages_deamon_thread;        // 热页面守护线程
    void hot_pages_deamon();                    // 热页面守护线程的主函数，每隔 lazy 秒将热页面队列中的页面移动到空闲链表中
    std::atomic<bool> hot_pages_deamon_running; // 热页面守护线程的运行标志

    bool because_of_shortage(int request); // 判断是否因为伙伴系统区域的页面数不足而无法分配

    void print_free_list();       // 打印空闲链表
    void print_hot_pages_queue(); // 打印热页面队列
    void audit();                 // 打印伙伴系统区域的统计信息

public:
    Zone(int order, int lazy);         // 构造函数
    ~Zone();                           // 析构函数
    Block *alloc(int size, int owner); // 从伙伴系统区域中分配 size 个页面给 owner 进程
    void free(Block &block);           // 将 block 释放到伙伴系统区域中
};

#endif // BUDDY_HH
