#include "buddy.hh"

#include <iostream>

Zone::Zone(int order)
{
    this->max_order = order;
    this->free_list = std::vector<std::deque<Block>>(order + 1, std::deque<Block>(0));
    this->hot_pages = std::queue<Block>();
    this->free_list[order].push_back(Block(0, (1 << order) - 1, 1 << order, -1));
}

Block *Zone::alloc(int size, int owner)
{
    if (size <= 0) {
        std::cerr << "请求页面数必须大于 0" << std::endl;
        return nullptr;
    }

    // 请求分配页面的数量是比 size 大的最小的 2 次幂，即 2^order
    int order = 0;
    while ((1 << order) < size)
        order++;

    // 如果请求的页面数量超过了该伙伴系统区域的最大页面数，则无法分配
    if (order > this->max_order)
    {
        std::cerr << "请求 " << order << " 个页面，超过该伙伴系统区域的最大页面数 " << this->max_order << std::endl;
        return nullptr;
    }

    // 找到第一个空闲页面数大于等于请求页面数的空闲页面链表
    int i = order;
    while (i <= this->max_order && this->free_list[i].size() == 0)
        i++;

    // 如果遍历整个空闲页面链表都没有找到空闲页面，则无法分配
    if (i > this->max_order)
    {
        std::cerr << "没有足够的空闲页面" << std::endl;
        return nullptr;
    }

    // 从找到的空闲页面链表中取出第一个空闲页面
    Block *block = &this->free_list[i].front();
    this->free_list[i].pop_front();

    // 如果该空闲页面的页面数大于请求页面数，则将该空闲页面分裂成两个伙伴页面，加入到空闲页面链表中
    while (i > order)
    {
        i--;
        int start = block->get_start();
        int end = block->get_end();
        int num_pages = block->get_num_pages();
        Block *left = new Block(start, start + num_pages / 2 - 1, num_pages / 2, -1);
        Block *right = new Block(start + num_pages / 2, end, num_pages / 2, -1);

        this->free_list[i].push_back(*left);
        this->free_list[i].push_back(*right);

        block = left;
    }

    // 从空闲页面链表表头取出一个空闲页面，将其分配给请求页面的进程
    block = &this->free_list[order].front();
    this->free_list[order].pop_front();
    block->set_owner(owner);

    return block;
}

void Zone::free(Block *block)
{
}
