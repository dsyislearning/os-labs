#include "process.hh"

#include <iostream>
#include <random>

Process::Process(int id, Zone *zone)
{
    this->id = id;
    this->zone = zone;
    this->entity = std::thread(&Process::run);
}

void Process::run()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 256);

    int num_alloc = 0;
    int num_free = 0;

    while (num_alloc < 10)
    {
        int num_pages = dis(gen);
        Block *block = this->zone->alloc(num_pages, this->id);
        if (block != nullptr)
        {
            this->blocks.push_back(block);
            num_alloc++;
        }
    }

    while (num_free < 10)
    {
        if (this->blocks.size() > 0)
        {
            Block *block = this->blocks.front();
            this->blocks.pop_front();
            this->zone->free(block);
            num_free++;
        }
    }
}