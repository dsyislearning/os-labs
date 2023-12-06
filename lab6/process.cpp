#include "process.hh"

#include <iostream>
#include <random>

#include "utils.hh"

Process::Process(int id, Zone &zone, std::mutex &zone_mutex)
    : id(id), zone(zone), zone_mutex(zone_mutex)
{
    this->entity = std::thread(&Process::run, this);
}

void Process::run()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 16);

    int num_alloc = 0;
    int num_free = 0;

    while (num_alloc < 10)
    {
        int num_pages = dis(gen);

        Log("#用户进程# ", this->id, " 请求 ", num_pages, " 个页面");

        Block *block = this->zone.alloc(num_pages, this->id);
        if (block != nullptr)
        {
            this->blocks.push_back(*block);
            num_alloc++;

            Log("#用户进程# ", this->id, " 得到 ", block->str());
        }
        else
        {
            Log("#用户进程# ", this->id, " 请求 ", num_pages, " 个页面失败");
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    while (num_free < 10)
    {
        if (this->blocks.size() > 0)
        {
            Block block = this->blocks.front();
            this->blocks.pop_front();
            this->zone.free(block);

            Log("#用户进程# ", this->id, " 释放 ", block.str());

            num_free++;
        }
    }
}

void Process::join()
{
    this->entity.join();
}
