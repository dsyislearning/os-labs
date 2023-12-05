#ifndef BLOCK_HH
#define BLOCK_HH

#define LEFT_BUDDY 0
#define RIGHT_BUDDY 1

class Block
{
private:
    int order;
    int start;
    int end;
    int num_pages;
    int owner;

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
    int is_buddy(Block *block);
};

#endif // BLOCK_HH
