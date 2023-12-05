#ifndef PROCESS_HH
#define PROCESS_HH

#include <deque>
#include <thread>

#include "block.hh"
#include "buddy.hh"

class Process
{
private:
    int id;
    Zone *zone;
    std::thread entity;
    std::deque<Block *> blocks;

    void run();

public:
    Process(int id, Zone *zone);
    void join();
};

#endif // PROCESS_HH
