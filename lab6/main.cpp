#include <iostream>
#include <fstream>
#include <thread>

#include "buddy.hh"
#include "process.hh"
#include "utils.hh"

#define PROCESS_NUM 5

int main()
{
    Log("伙伴系统模拟开始... 请勿对输出文件进行操作");
    std::ofstream file("output.txt");
    std::streambuf *coutbuf = std::cout.rdbuf();
    std::cout.rdbuf(file.rdbuf());

    {
        Zone zone{10, 1};
        std::mutex zone_mutex;

        Process small{0, zone, 1, 4};
        Process middle{1, zone, 32, 8};
        Process large{2, zone, 64, 16};

        small.join();
        middle.join();
        large.join();
    }

    file.close();
    std::cout.rdbuf(coutbuf);
    Log("伙伴系统模拟结束... 请在 output.txt 中查看输出结果");

    return 0;
}
