#include <iostream>

#include "buddy.hh"
#include "process.hh"

int main()
{
    Zone zone(10);
    Block *block[10];
    int random_size[10] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
    for (int i = 0; i < 10; i++)
    {
        int size = random_size[i];
        block[i] = zone.alloc(size, i);
        std::cout << "进程 " << i << " 请求 " << size << " 个页面：" << std::endl;
        if (block[i] != nullptr)
        {
            std::cout << "分配成功，分配的页面号为 " << block[i]->get_start() << " 到 " << block[i]->get_end() << std::endl;
        }
        else
        {
            std::cout << "分配失败" << std::endl;
        }
    }
    return 0;
}
