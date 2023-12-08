#ifndef BLOCK_HH
#define BLOCK_HH

#include <iostream>
#include <atomic>

#define LEFT_BUDDY 0  // 左伙伴标志
#define RIGHT_BUDDY 1 // 右伙伴标志

#define PAGE_SIZE 4096 // 单个页面大小（单位为字节）

class Block
{
private:
    int order;     // 该 block 的阶数，即该 block 的大小为 2^order 个页面
    int start;     // 该 block 的起始页面号
    int end;       // 该 block 的结束页面号
    int num_pages; // 该 block 的页面数
    int owner;     // 该 block 的所有者，-1 表示该 block 为空闲，否则表示该 block 属于某个进程

public:
    Block();
    Block(int order, int start, int end, int num_pages, int owner);
    int get_order();
    int get_start();
    int get_end();
    int get_num_pages();
    int get_owner();
    void set_order(int order);
    void set_start(int start);
    void set_end(int end);
    void set_num_pages(int num_pages);
    void set_owner(int owner);
    int is_buddy(Block &block); // 判断某个Block是否是this的伙伴

    std::string str();
};

#endif // BLOCK_HH
