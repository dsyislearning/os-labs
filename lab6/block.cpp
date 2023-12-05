#include "block.hh"

// 构造函数

Block::Block()
{
    this->order = 0;
    this->start = 0;
    this->end = 0;
    this->num_pages = 0;
    this->owner = -1;
}

Block::Block(int order, int start, int end, int num_pages, int owner)
{
    this->order = order;
    this->start = start;
    this->end = end;
    this->num_pages = num_pages;
    this->owner = owner;
}

// Getter

int Block::get_order()
{
    return this->order;
}

int Block::get_start()
{
    return this->start;
}

int Block::get_end()
{
    return this->end;
}

int Block::get_num_pages()
{
    return this->num_pages;
}

int Block::get_owner()
{
    return this->owner;
}

// Setter

void Block::set_owner(int owner)
{
    this->owner = owner;
}

void Block::set_start(int start)
{
    this->start = start;
}

void Block::set_end(int end)
{
    this->end = end;
}

void Block::set_order(int order)
{
    this->order = order;
}

void Block::set_num_pages(int num_pages)
{
    this->num_pages = num_pages;
}

// 判断某个Block是否是this的伙伴
int Block::is_buddy(Block *block)
{
    if (this->order != block->get_order())
        return -1;

    if (this->start == block->get_end() + 1)
        return LEFT_BUDDY;

    if (this->end == block->get_start() - 1)
        return RIGHT_BUDDY;

    return -1;
}
