#include "block.hh"

Block::Block()
{
    this->start = 0;
    this->end = 0;
    this->num_pages = 0;
    this->owner = -1;
}

Block::Block(int start, int end, int num_pages, int owner)
{
    this->start = start;
    this->end = end;
    this->num_pages = num_pages;
    this->owner = owner;
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

void Block::set_start(int start)
{
    this->start = start;
}

void Block::set_end(int end)
{
    this->end = end;
}

void Block::set_num_pages(int num_pages)
{
    this->num_pages = num_pages;
}

void Block::set_owner(int owner)
{
    this->owner = owner;
}
