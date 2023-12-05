#include "buddy.hh"

#include <iostream>
#include <thread>
#include <chrono>

// 构造函数

Zone::Zone(int order)
{
    this->max_order = order;

    this->free_list = std::vector<std::deque<Block>>(order + 1, std::deque<Block>(0));
    this->free_list[order].push_back(Block(order, 0, (1 << order) - 1, 1 << order, -1));

    this->hot_pages_queue = std::queue<Block>();
    this->hot_pages_deamon_thread = std::thread(&Zone::hot_pages_deamon);
}

// 析构函数

Zone::~Zone()
{
    this->hot_pages_deamon_thread.join();
}

Block *Zone::get_block(int order)
{
    // 加锁
    this->free_list_mutex.lock();

    // 找到第一个空闲页面数大于等于请求页面数的空闲页面链表
    int i = order;
    this->max_order_mutex.lock();
    int max_order = this->max_order;
    this->max_order_mutex.unlock();
    while (i <= max_order && this->free_list[i].size() == 0)
        i++;

    // 如果遍历整个空闲页面链表都没有找到空闲页面，则无法分配
    if (i > max_order)
    {
        std::cerr << "没有足够的空闲页面" << std::endl;
        return nullptr;
    }

    // 从找到的空闲页面链表中取出第一个空闲Block
    Block *block = &this->free_list[i].front();
    this->free_list[i].pop_front();

    // 如果该空闲页面的页面数大于请求页面数，则将该空闲页面分裂成两个伙伴页面，加入到空闲页面链表中
    while (i > order)
    {
        i--;
        int start = block->get_start();
        int end = block->get_end();
        int num_pages = block->get_num_pages();

        Block *left = new Block(i, start, start + num_pages / 2 - 1, num_pages / 2, -1);
        Block *right = new Block(i, start + num_pages / 2, end, num_pages / 2, -1);

        this->free_list[i].push_back(*left);
        this->free_list[i].push_back(*right);

        block = left;
    }

    // 从空闲页面链表表头取出一个空闲页面
    block = &this->free_list[order].front();
    this->free_list[order].pop_front();

    // 解锁
    this->free_list_mutex.unlock();

    return block;
}

void Zone::insert_block(Block *block)
{
    // 加锁
    this->free_list_mutex.lock();

    int order = block->get_order();
    int start = block->get_start();
    int end = block->get_end();
    int num_pages = block->get_num_pages();

    // 如果空闲页面链表为空，则直接将该空闲页面加入到空闲页面链表中
    if (this->free_list[order].size() == 0)
    {
        this->free_list[order].push_back(*block);
        return;
    }

    // 遍历空闲页面链表，找伙伴
    for (auto it = this->free_list[order].begin(); it != this->free_list[order].end(); it++)
    {
        Block *buddy = &(*it);

        int is_buddy = block->is_buddy(buddy);

        // 没有伙伴，则直接将该空闲页面加入到空闲页面链表中
        if (is_buddy == -1)
        {
            this->free_list[order].push_back(*block);
            break;
        }
        // 左伙伴
        else if (is_buddy == LEFT_BUDDY)
        {
            order += 1;
            start = buddy->get_start();
            end = block->get_end();
            num_pages *= 2;
            Block *new_block = new Block(order, start, end, num_pages, -1);

            delete block;
            delete buddy;

            this->insert_block(new_block);
            break;
        }
        // 右伙伴
        else if (is_buddy == RIGHT_BUDDY)
        {
            order += 1;
            start = block->get_start();
            end = buddy->get_end();
            num_pages *= 2;
            Block *new_block = new Block(order, start, end, num_pages, -1);

            delete block;
            delete buddy;

            this->insert_block(new_block);
            break;
        }
    }

    // 解锁
    this->free_list_mutex.unlock();
}

Block *Zone::alloc(int size, int owner)
{
    if (size <= 0)
    {
        std::cerr << "请求页面数必须大于 0" << std::endl;
        return nullptr;
    }

    // 请求分配页面的数量是比 size 大的最小的 2 次幂，即 2^order
    int order = 0;
    while ((1 << order) < size)
        order++;

    this->max_order_mutex.lock();
    int max_order = this->max_order;
    this->max_order_mutex.unlock();

    // 如果请求的页面数量超过了该伙伴系统区域的最大页面数，则无法分配
    if (order > max_order)
    {
        std::cerr << "请求 " << order << " 个页面，超过该伙伴系统区域的最大页面数 " << this->max_order << std::endl;
        return nullptr;
    }

    // 首先从热页面队列中查找是否有空闲页面
    this->hot_pages_mutex.lock();
    if (this->hot_pages_queue.size() > 0)
    {
        Block *block = &this->hot_pages_queue.front();

        // 如果找到的空闲页面的页面数大于等于请求页面数，则将其分配给请求页面的进程
        if (block->get_num_pages() >= (1 << order))
        {
            this->hot_pages_queue.pop();
            block->set_owner(owner);

            // 释放互斥锁
            this->hot_pages_mutex.unlock();

            return block;
        }
    }
    this->hot_pages_mutex.unlock();

    // 如果热页面队列中没有空闲页面，则从空闲页面链表中查找是否有空闲页面
    Block *block = this->get_block(order);
    block->set_owner(owner);

    return block;
}

void Zone::free(Block *block)
{
    // 加锁
    this->hot_pages_mutex.lock();

    this->hot_pages_queue.push(*block);

    // 解锁
    this->hot_pages_mutex.unlock();
}

void Zone::hot_pages_deamon()
{
    while (true)
    {
        // 加锁
        this->hot_pages_mutex.lock();

        // 每隔 lazy 时间，将热页面队列中的页面加入到空闲页面链表中
        if (this->hot_pages_queue.size() > 0)
        {
            Block *block = &this->hot_pages_queue.front();
            this->hot_pages_queue.pop();

            this->insert_block(block);
        }

        // 解锁
        this->hot_pages_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(this->lazy));
    }
}
