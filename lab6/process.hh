#ifndef PROCESS_HH
#define PROCESS_HH

#include <deque>
#include <thread>

#include "block.hh"
#include "buddy.hh"

class Process
{
private:
    int id;     // 进程编号
    int mean;   // 正态分布的均值
    int stddev; // 正态分布的标准差

    Zone &zone; // 使用的伙伴系统区域

    std::thread entity; // 进程实体

    std::vector<Block> blocks; // 已分配的 block

    void run(); // 进程实体的主函数

public:
    Process(int id, Zone &zone, int mean, int stddev);
    void join();
};

#endif // PROCESS_HH
