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
    int mean;
    int stddev;

    Zone &zone;

    std::thread entity;

    std::vector<Block> blocks;

    void run();

public:
    Process(int id, Zone &zone, int mean, int stddev);
    void join();
};

#endif // PROCESS_HH
