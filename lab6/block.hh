#ifndef BLOCK_HH
#define BLOCK_HH

class Block
{
private:
    int start;
    int end;
    int num_pages;
    int owner;

public:
    Block();
    Block(int start, int end, int num_pages, int owner);
    int get_start();
    int get_end();
    int get_num_pages();
    int get_owner();
    void set_start(int start);
    void set_end(int end);
    void set_num_pages(int num_pages);
    void set_owner(int owner);
};

#endif // BLOCK_HH
