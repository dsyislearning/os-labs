#include "buddy.hh"
#include "slab.hh"
#include "utils.hh"

#include <mutex>
#include <random>
#include <chrono>
#include <thread>

std::mutex log_mutex;

int main()
{
    Zone zone(10, 1);
    Slab slab(zone);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 4);

    int KB = 1024;
    int object_sizes[5]{1 * KB, 2 * KB, 3 * KB, 4 * KB, 7 * KB};

    // slab.alloc(4096);
    // slab.print_slab();
    for (int i = 0; i < 10; i++)
    {
        if (dis(gen) < 3)
        {
            int index = dis(gen);
            int object_size = object_sizes[index];
            if (slab.alloc(object_size))
            {
                Log("分配 ", object_size, "B 对象");
                slab.print_slab();
            }
        }
        else
        {
            int index = dis(gen);
            int object_size = object_sizes[index];
            if (slab.free(object_size))
            {
                Log("释放 ", object_size, "B 对象");
                slab.print_slab();
            }
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    for (int i = 0; i < 10; i++)
    {
        while (slab.free(object_sizes[i]))
        {
            Log("释放 ", object_sizes[i], "B 对象");
            slab.print_slab();
        }
    }

    return 0;
}
