#include <iostream>
#include <thread>

#include "buddy.hh"
#include "process.hh"
#include "utils.hh"

#define PROCESS_NUM 5

int main()
{
    Zone zone{10, 2};
    std::mutex zone_mutex;

    Process *processes[PROCESS_NUM];
    for (int i = 0; i < PROCESS_NUM; i++)
    {
        processes[i] = new Process(i, zone, zone_mutex);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    for (int i = 0; i < PROCESS_NUM; i++)
        processes[i]->join();

    return 0;
}
