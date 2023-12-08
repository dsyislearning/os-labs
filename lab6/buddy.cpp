#include "buddy.hh"

#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>

#include "utils.hh"

// 构造函数

Zone::Zone(int order, int lazy)
    : max_order(order),
      total_requests(0),
      success(0),
      shortage_fail(0),
      fragment_fail(0),
      other_fail(0),
      full_pages(0),
      empty_pages(0),
      lazy(lazy),
      hot_pages_deamon_running(true)
{
    this->free_list = std::vector<std::deque<Block>>(order + 1, std::deque<Block>(0));
    int pages = 1 << order;
    this->free_list[order].push_back(Block(order, 0, pages - 1, pages, -1));
    this->empty_pages = pages;

    this->hot_pages_queue = std::queue<Block>();
    this->hot_pages_deamon_thread = std::thread(&Zone::hot_pages_deamon, this);
}

// 析构函数

Zone::~Zone()
{
    this->hot_pages_deamon_running = false;

    if (this->hot_pages_deamon_thread.joinable())
        this->hot_pages_deamon_thread.join();

    this->audit();
}

void Zone::print_free_list()
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::recursive_mutex> lock(this->free_list_mutex);

    std::ostringstream oss;
    oss << "free_list:" << std::endl;
    for (int i = 0; i <= this->max_order; i++)
    {
        oss << "free_list[" << i << "]: ";
        for (auto it = this->free_list[i].begin(); it != this->free_list[i].end(); it++)
            oss << it->str() << " ";
        oss << std::endl;
    }

    Log(oss.str());
}

void Zone::print_hot_pages_queue()
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::mutex> lock(this->hot_pages_mutex);

    std::ostringstream oss;
    oss << "hot_pages_queue: " << std::endl;
    std::queue<Block> temp = this->hot_pages_queue;
    if (temp.size() == 0)
    {
        oss << "empty";
    }
    else
    {

        while (temp.size() > 0)
        {
            oss << temp.front().str() << " ";
            temp.pop();
        }
    }
    oss << std::endl;

    Log(oss.str());
}

void Zone::audit()
{
    std::ostringstream oss;
    oss << "audit:" << std::endl;
    oss << "total_requests: " << this->total_requests << std::endl;
    oss << "success: " << this->success << std::endl;
    oss << "shortage_fail: " << this->shortage_fail << std::endl;
    oss << "fragment_fail: " << this->fragment_fail << std::endl;
    oss << "other_fail: " << this->other_fail << std::endl;
    oss << "full_pages: " << this->full_pages << std::endl;
    oss << "empty_pages: " << this->empty_pages << std::endl;
    Log(oss.str());

    this->print_hot_pages_queue();

    this->print_free_list();
}

Block *Zone::get_block(int order, int owner)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::recursive_mutex> lock(this->free_list_mutex);

    // 找到第一个空闲页面数大于等于请求页面数的空闲页面链表
    int i = order;
    while (i <= max_order && this->free_list[i].size() == 0)
        i++;

    // 如果遍历整个空闲页面链表都没有找到空闲页面，则无法分配
    if (i > max_order)
    {
        int pages = 1 << order;
        if (this->because_of_shortage(pages))
        {
            Log("#空闲链表# 由于实际空间不足无法分配 2^", order, "=", pages, " 个 Block 给进程 ", owner);
            this->shortage_fail += 1;
        }
        else
        {
            Log("#空闲链表# 由于内存碎片无法分配 2^", order, "=", pages, " 个 Block 给进程 ", owner);
            this->fragment_fail += 1;
        }
        this->print_hot_pages_queue();
        this->print_free_list();
        return nullptr;
    }

    // 从找到的空闲页面链表中取出第一个空闲Block
    Block *block = &this->free_list[i].front();

    // 如果该空闲页面的页面数大于请求页面数，则将该空闲页面分裂成两个伙伴页面，加入到空闲页面链表中
    while (i > order)
    {
        this->free_list[i].pop_front();

        // Log("#空闲链表# 分裂 ", block->str());

        i--;
        int start = block->get_start();
        int end = block->get_end();
        int num_pages = block->get_num_pages();

        Block *left = new Block(i, start, start + num_pages / 2 - 1, num_pages / 2, -1);
        Block *right = new Block(i, start + num_pages / 2, end, num_pages / 2, -1);

        this->free_list[i].push_back(*left);
        this->free_list[i].push_back(*right);

        // Log("#空闲链表# ", left->str(), " ", right->str(), " 加入到 free_list[", i, "]");

        block = &this->free_list[i].front();
    }

    // 从空闲页面链表表头取出一个空闲页面
    this->free_list[order].pop_front();
    block->set_owner(owner);

    // Log("#空闲链表# ", block->str(), " 被取出");

    return block;
}

void Zone::insert_block(Block &block)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::recursive_mutex> lock(this->free_list_mutex);

    int order = block.get_order();
    int start = block.get_start();
    int end = block.get_end();
    int num_pages = block.get_num_pages();

    // 如果空闲页面链表为空，则直接将该空闲页面加入到空闲页面链表中
    if (this->free_list[order].size() == 0)
    {
        block.set_owner(-1);
        this->free_list[order].push_back(block);

        // Log("#空闲链表# free_list[", order, "] 为空，直接插入 ", block.str());

        return;
    }

    // 遍历空闲页面链表，找伙伴
    int is_buddy = -1;
    int buddy_order = order;
    std::deque<Block>::iterator buddy;

    for (auto it = this->free_list[order].begin(); it != this->free_list[order].end(); it++)
    {
        buddy = it;

        is_buddy = block.is_buddy(*buddy);

        if (is_buddy != -1)
            break;
    }

    // 没有伙伴，则直接将该空闲页面加入到空闲页面链表中
    if (is_buddy == -1)
    {
        block.set_owner(-1);
        this->free_list[order].push_back(block);

        // Log("#空闲链表# ", block.str(), " 插入 ", "free_list[", order, "]");
    }
    else
    {
        order += 1;
        num_pages *= 2;
        if (is_buddy == LEFT_BUDDY) // 左伙伴
        {
            start = buddy->get_start();
            end = block.get_end();
            Block new_block{order, start, end, num_pages, -1};

            // Log("#空闲链表# ", buddy->str(), " ", block.str(), " 被合并成 ", new_block.str());

            this->insert_block(new_block); // 插入合并后的 block
        }
        else if (is_buddy == RIGHT_BUDDY) // 右伙伴
        {
            start = block.get_start();
            end = buddy->get_end();
            Block new_block{order, start, end, num_pages, -1};

            // Log("#空闲链表# ", block.str(), " ", buddy->str(), " 被合并成 ", new_block.str());

            this->insert_block(new_block); // 插入合并后的 block
        }
        this->free_list[buddy_order].erase(buddy); // 删除 free_list[buddy_order] 中已经被合并的 buddy
    }
}

Block *Zone::alloc(int size, int owner)
{
    this->total_requests += 1;

    // 请求页面数小于等于 0，则无法分配
    if (size <= 0)
    {
        Log("#页面分配# ", "请求页面数 ", size, " 小于等于 0");
        this->other_fail += 1;
        return nullptr;
    }

    // 请求分配页面的数量是比 size 大的最小的 2 次幂，即 2^order
    int order = 0;
    while ((1 << order) < size)
        order++;

    // 如果请求的页面数量超过了该伙伴系统区域的最大页面数，则无法分配
    if (order > this->max_order)
    {
        int max_order = this->max_order;
        Log("#页面分配# ", "请求 2^", order, " 个 Block ，超过该伙伴系统区域的最大页面数 ", max_order);
        this->shortage_fail += 1;
        return nullptr;
    }

    // 首先从热页面队列中查找是否有空闲页面
    this->hot_pages_mutex.lock();
    while (this->hot_pages_queue.size() > 0)
    {
        Block *block = &this->hot_pages_queue.front();

        // 如果找到的空闲页面的页面数大于等于请求页面数，则将其分配给请求页面的进程
        if (block->get_order() >= order)
        {
            this->hot_pages_queue.pop();

            Log("#热页队列# ", block->str(), " 从热页队列中取出");

            block->set_owner(owner);

            Log("#页面分配# ", block->str(), " 从热页队列中分配给进程 ", owner);

            this->success += 1;
            this->full_pages += block->get_num_pages();
            this->empty_pages -= block->get_num_pages();

            // 释放互斥锁
            this->hot_pages_mutex.unlock();

            return block;
        }
        else // 没有找到就把当前的 block 插入回空闲页面链表中
        {
            this->hot_pages_queue.pop();

            Log("#热页队列# ", block->str(), " 从热页队列中取出");

            this->insert_block(*block);
        }
    }
    this->hot_pages_mutex.unlock();

    // 如果热页面队列中没有空闲页面，则从空闲页面链表中查找是否有空闲页面
    Block *block = this->get_block(order, owner);

    if (block != nullptr)
    {
        this->success += 1;
        this->full_pages += block->get_num_pages();
        this->empty_pages -= block->get_num_pages();
        Log("#页面分配# ", block->str(), " 从空闲页面链表中分配给进程 ", owner);
    }

    return block;
}

void Zone::free(Block &block)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::mutex> lock(this->hot_pages_mutex);

    block.set_owner(-1);
    this->hot_pages_queue.push(block);

    this->full_pages -= block.get_num_pages();
    this->empty_pages += block.get_num_pages();

    Log("#热页队列# ", block.str(), " 加入热页队列");

}

void Zone::hot_pages_deamon()
{
    // Log("#热页队列# 守护线程启动");
    while (this->hot_pages_deamon_running)
    {
        // 加锁
        this->hot_pages_mutex.lock();

        // 每隔 lazy 时间，将热页面队列中的页面加入到空闲页面链表中
        if (this->hot_pages_queue.size() > 0)
        {
            Block block = this->hot_pages_queue.front();
            this->hot_pages_queue.pop();

            // Log("#热页队列# ", block.str(), " 加入到空闲页面链表中");

            this->insert_block(block);
        }

        // 解锁
        this->hot_pages_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(this->lazy));
    }

    this->hot_pages_mutex.lock();
    while (this->hot_pages_queue.size() > 0)
    {
        Block block = this->hot_pages_queue.front();
        this->hot_pages_queue.pop();

        // Log("#热页队列# ", block.str(), " 加入到空闲页面链表中");

        this->insert_block(block);
    }
    this->hot_pages_mutex.unlock();
}

bool Zone::because_of_shortage(int request)
{
    return this->empty_pages < request;
}
