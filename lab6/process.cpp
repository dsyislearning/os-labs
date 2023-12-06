#include "process.hh"

#include <iostream>
#include <random>

#include "utils.hh"

Process::Process(int id, Zone &zone, int mean, int stddev)
    : id(id), zone(zone), mean(mean), stddev(stddev)
{
    this->entity = std::thread(&Process::run, this);
}

void Process::run()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    std::normal_distribution<> ndis(this->mean, this->stddev);

    int times = 0;

    while (times < 100)
    {
        if (dis(gen) < 5)
        {
            // 按正态分布随机生成一个请求序列
            int num_pages = std::abs(ndis(gen));

            Log("#用户进程# ", this->id, " 请求 ", num_pages, " 个页面");

            Block *block = this->zone.alloc(num_pages, this->id);
            if (block != nullptr)
            {
                this->blocks.push_back(*block);

                Log("#用户进程# ", this->id, " 得到 ", block->str());

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            else
            {
                Log("#用户进程# ", this->id, " 请求 ", num_pages, " 个页面失败");
            }
        }
        else
        {
            if (this->blocks.size() > 0)
            {
                // 以较为随机的次序进行释放
                std::uniform_int_distribution<> dis(0, this->blocks.size() - 1);
                Block block_to_free = this->blocks[dis(gen)];
                this->zone.free(block_to_free);

                Log("#用户进程# ", this->id, " 释放 ", block_to_free.str());
            }
        }

        times++;
    }

    while (this->blocks.size() > 0)
    {
        Block block{this->blocks.front()};
        this->blocks.pop_front();
        this->zone.free(block);

        Log("#用户进程# ", this->id, " 释放 ", block.str());
    }
}

void Process::join()
{
    this->entity.join();
}
