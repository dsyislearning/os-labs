# lab6 内存管理

## 实验目的

通过模拟实现内存分配的伙伴算法和请求页式存储管理的几种基本页面置换算法，了解存储技术的特点。掌握虚拟存储请求页式存储管理中几种基本页面置换算法的基本思想和实现过程，并比较它们的效率。

## 实验内容

### 6-1 伙伴系统

基于内存管理的伙伴算法，实现内存块申请时的分配和释放后的回收（dmalloc/dfree），同时在回收过程中可对块进行合并。 

实验准备：

用随机函数仿真进程进行内存申请，并且以较为随机的次序进行释放。对其碎片进行统计，当申请分配内存失败时区分实际空间不足和由于碎片而不能满足。 

1）按一定分布随机生成一个请求序列，对不同大小下分配成功和失败的次数进行统计。

2）解决切分与合并过于频繁的问题。

系统中用得较多的是单个页。位于处理器Cache中页称为热页（hot page），其余页称为冷页（cold page）。处理器对热页的访问速度要远快于冷页。

可考虑为每个CPU建一个热页队列，暂存刚释放的单个物理页，将合并工作向后推迟（ Lazy）。进行分配时总试图从热页队列中分配单个物理页，且分配与释放都在热页队列的队头进行。

### 6-2 Slab

由于内核对小内存的使用极为频繁、种类繁多，时机和数量难以预估，所以难以预先分配，只能动态地创建和撤销。

操作系统Slab 向伙伴算法申请大的页块，将其划分成小对象分配出去，并将回收的小对象组合成大页块后还给伙伴算法。Slab 采用等尺寸静态分区法，将页块预先划分成一组大小相等的小块，称为内存对象；具有相同属性的多个Slab构成一个Cache，一个Cache管理一种类型（相同大小）的内存对象。当需要小内存时，从预建的Cache中申请内存对象，用完之后再将其还给Cache。当Cache中缺少对象时，就追加新的Slab；而当物理内存紧缺时，可回收完全空闲的Slab。

用随机函数仿真不同大小的内核对象内存申请，并且以较为随机的次序进行释放。通过实验展示Slab所具有的不同状态；能够可动态分配cache；展示Slab的分配及空闲Slab的回收，并对期间的动作进行统计分析。

## 实验环境

实验环境为本人计算机，平台参数如下：

- 操作系统：Arch Linux
- 内核版本：6.6.3-arch1-1 (64-bit)
- 处理器：8 × 11th Gen Intel® Core™ i5-11300H @ 3.10GHz
- 内存：15.4 GiB of RAM

## 实验步骤

### 6-1 伙伴系统实现

伙伴系统的实现在以下文件中：

- `block.hh` ， `block.cpp`
- `buddy.hh` ，`buddy.cpp`

#### 数据对象定义

##### 伙伴系统区域

经查阅资料，Linux 内核中的伙伴算法使用位图和空闲链表作为辅助工具，其中位图用于跟踪内存块的使用情况，空闲链表用来维护内存中还没有被分配的块。

`Zone` 为一整块由伙伴系统管理的物理连续的页面，其大小为 2 的幂次。其数据结构（`buddy.hh`）如下：

```cpp
class Zone
{
private:
    std::atomic<int> max_order;      // 该区域物理连续页面阶数，即最大可分配的页面数为 2^max_order
    std::atomic<int> total_requests; // 统计总共的请求次数
    std::atomic<int> success;        // 统计成功分配的次数
    std::atomic<int> shortage_fail;  // 统计因为伙伴系统区域的页面数不足而无法分配的次数
    std::atomic<int> fragment_fail;  // 统计因为伙伴系统区域的碎片而无法分配的次数
    std::atomic<int> other_fail;     // 统计因为其他原因无法分配的次数
    std::atomic<int> full_pages;     // 统计伙伴系统区域中已分配的页面数
    std::atomic<int> empty_pages;    // 统计伙伴系统区域中未分配的页面数

    std::vector<std::deque<Block>> free_list; // 伙伴系统区域的空闲链表
    void insert_block(Block &block);          // 将 block 插入到空闲链表中
    Block *get_block(int order, int owner);   // 从空闲链表中获取一个 order 阶的 block 分配给 owner 进程
    std::recursive_mutex free_list_mutex;     // 伙伴系统区域的空闲链表的互斥锁

    std::queue<Block> hot_pages_queue; // 热页面队列
    std::atomic<int> lazy;             // 热页面队列的惰性阈值
    std::mutex hot_pages_mutex;        // 热页面队列的互斥锁

    std::thread hot_pages_deamon_thread;        // 热页面守护线程
    void hot_pages_deamon();                    // 热页面守护线程的主函数，每隔 lazy 秒将热页面队列中的页面移动到空闲链表中
    std::atomic<bool> hot_pages_deamon_running; // 热页面守护线程的运行标志

    bool because_of_shortage(int request); // 判断是否因为伙伴系统区域的页面数不足而无法分配

    void print_free_list();       // 打印空闲链表
    void print_hot_pages_queue(); // 打印热页面队列
    void audit();                 // 打印伙伴系统区域的统计信息

public:
    Zone(int order, int lazy);         // 构造函数
    ~Zone();                           // 析构函数
    Block *alloc(int size, int owner); // 从伙伴系统区域中分配 size 个页面给 owner 进程
    void free(Block &block);           // 将 block 释放到伙伴系统区域中
};
```

##### 伙伴系统分配的页块

`Block` 为伙伴系统分配出来的页块对象，里面存储了在连续物理内存中的下标与所含页面数量等信息（`block.hh`）

```cpp
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
```

##### 使用伙伴系统的内核进程

`Process` 用来模拟使用伙伴系统的进程，请求和释放页块通过调用 `this->zone.alloc` 和 `this->zone.free` 实现

```cpp
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
```

#### 空闲链表的工作过程

##### 从空闲链表中取出页块

假设定义了一个最大阶数为 10 ，即物理连续页面数为 $2^{10}=1024$ 的伙伴系统区域：

```cpp
Zone zone{10, 1};
```

初始时的空闲链表为：

```
free_list[0]: 
free_list[1]: 
free_list[2]: 
free_list[3]: 
free_list[4]: 
free_list[5]: 
free_list[6]: 
free_list[7]: 
free_list[8]: 
free_list[9]: 
free_list[10]: Block{1024[0-1023],-1} 
```

这个空闲链表的位图表示阶数为 10 的空闲块有一个：`Block{1024[0-1023],-1} ` ，他包含 1024 页面，在连续物理内存中的下标为 0 - 1023 ，-1 表示没有任何进程拥有这个空闲页块。

假如有一个进程 1 请求 150 个页面，调用 `zone->alloc(150, 1)` 来向伙伴系统区域请求页块，伙伴系统分配页面过程如下：

- 首先找到分配 150 个页面的最小页块（一定是 2 的幂）为 256 页，$256=2^8$ ，查找 `free_list[8]` ，为空，则往后找第一个不为空的空闲页块链表

- 找到 `free_list[10]` 不为空，取出 `Block{1024[0-1023],-1}` 。由于 10 > 8，需要分裂，第一次分裂后空闲链表变为：

  ```
  free_list[0]: 
  free_list[1]: 
  free_list[2]: 
  free_list[3]: 
  free_list[4]: 
  free_list[5]: 
  free_list[6]: 
  free_list[7]: 
  free_list[8]: 
  free_list[9]: Block{512[0-511],-1} Block{512[512-1023],-1}
  free_list[10]: 
  ```

  取出 `Block{512[0-511],-1}` ，9 > 8 ，继续分裂：

  ```
  free_list[0]: 
  free_list[1]: 
  free_list[2]: 
  free_list[3]: 
  free_list[4]: 
  free_list[5]: 
  free_list[6]: 
  free_list[7]: 
  free_list[8]: Block{256[0-255],-1} Block{256[256-511],-1}
  free_list[9]: Block{512[512-1023],-1}
  free_list[10]: 
  ```

  取出 `Block{256[0-255],-1}` ，分配给进程 1 ：`Block{256[0-255],1}`

- 分配完成后空闲链表变为：

  ```
  free_list[0]: 
  free_list[1]: 
  free_list[2]: 
  free_list[3]: 
  free_list[4]: 
  free_list[5]: 
  free_list[6]: 
  free_list[7]: 
  free_list[8]: Block{256[256-511],-1}
  free_list[9]: Block{512[512-1023],-1}
  free_list[10]: 
  ```

从空闲链表取出页块的算法（`buddy.cpp`）如下：

```cpp
Block *Zone::get_block(int order, int owner)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::recursive_mutex> lock(this->free_list_mutex);

    // 找到第一个空闲页面数大于等于请求页面数的空闲页面链表
    int i = order;
    while (i <= max_order && this->free_list[i].size() == 0)
        i++;

    // 如果遍历整个空闲页面链表都没有找到空闲页面，则无法分配
    if (i > max_order)
    {
        int pages = 1 << order;
        if (this->because_of_shortage(pages))
        {
            Log("#空闲链表# 由于实际空间不足无法分配 2^", order, "=", pages, " 个 Block 给进程 ", owner);
            this->shortage_fail += 1;
        }
        else
        {
            Log("#空闲链表# 由于内存碎片无法分配 2^", order, "=", pages, " 个 Block 给进程 ", owner);
            this->fragment_fail += 1;
        }
        this->print_hot_pages_queue();
        this->print_free_list();
        return nullptr;
    }

    // 从找到的空闲页面链表中取出第一个空闲Block
    Block *block = &this->free_list[i].front();

    // 如果该空闲页面的页面数大于请求页面数，则将该空闲页面分裂成两个伙伴页面，加入到空闲页面链表中
    while (i > order)
    {
        this->free_list[i].pop_front();

        // Log("#空闲链表# 分裂 ", block->str());

        i--;
        int start = block->get_start();
        int end = block->get_end();
        int num_pages = block->get_num_pages();

        Block *left = new Block(i, start, start + num_pages / 2 - 1, num_pages / 2, -1);
        Block *right = new Block(i, start + num_pages / 2, end, num_pages / 2, -1);

        this->free_list[i].push_back(*left);
        this->free_list[i].push_back(*right);

        // Log("#空闲链表# ", left->str(), " ", right->str(), " 加入到 free_list[", i, "]");

        block = &this->free_list[i].front();
    }

    // 从空闲页面链表表头取出一个空闲页面
    this->free_list[order].pop_front();
    block->set_owner(owner);

    // Log("#空闲链表# ", block->str(), " 被取出");

    return block;
}
```

##### 将页块插入回空闲链表

上面取出 256 页后空闲链表变为：

```
free_list[0]: 
free_list[1]: 
free_list[2]: 
free_list[3]: 
free_list[4]: 
free_list[5]: 
free_list[6]: 
free_list[7]: 
free_list[8]: Block{256[256-511],-1}
free_list[9]: Block{512[512-1023],-1}
free_list[10]: 
```

假如再将这 256 页 `Block{256[0-255],1}` 插回空闲链表，过程如下：

- `free_list[8]` 不为空，在里面找有没有当前页块的伙伴，找到 `Block{256[256-511],-1}` 是右伙伴，将两个合并为 `Block{512[0-511],-1}` 后递归调用插入函数插回空闲链表

- 最终插入回后的效果为：

  ```
  free_list[0]: 
  free_list[1]: 
  free_list[2]: 
  free_list[3]: 
  free_list[4]: 
  free_list[5]: 
  free_list[6]: 
  free_list[7]: 
  free_list[8]: 
  free_list[9]: 
  free_list[10]: Block{1024[0-1023],-1} 
  ```

将页块插回空闲链表的方法（`buddy.cpp`）如下：

```cpp
void Zone::insert_block(Block &block)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::recursive_mutex> lock(this->free_list_mutex);

    int order = block.get_order();
    int start = block.get_start();
    int end = block.get_end();
    int num_pages = block.get_num_pages();

    // 如果空闲页面链表为空，则直接将该空闲页面加入到空闲页面链表中
    if (this->free_list[order].size() == 0)
    {
        block.set_owner(-1);
        this->free_list[order].push_back(block);

        // Log("#空闲链表# free_list[", order, "] 为空，直接插入 ", block.str());

        return;
    }

    // 遍历空闲页面链表，找伙伴
    int is_buddy = -1;
    int buddy_order = order;
    std::deque<Block>::iterator buddy;

    for (auto it = this->free_list[order].begin(); it != this->free_list[order].end(); it++)
    {
        buddy = it;

        is_buddy = block.is_buddy(*buddy);

        if (is_buddy != -1)
            break;
    }

    // 没有伙伴，则直接将该空闲页面加入到空闲页面链表中
    if (is_buddy == -1)
    {
        block.set_owner(-1);
        this->free_list[order].push_back(block);

        // Log("#空闲链表# ", block.str(), " 插入 ", "free_list[", order, "]");
    }
    else
    {
        order += 1;
        num_pages *= 2;
        if (is_buddy == LEFT_BUDDY) // 左伙伴
        {
            start = buddy->get_start();
            end = block.get_end();
            Block new_block{order, start, end, num_pages, -1};

            // Log("#空闲链表# ", buddy->str(), " ", block.str(), " 被合并成 ", new_block.str());

            this->insert_block(new_block); // 插入合并后的 block
        }
        else if (is_buddy == RIGHT_BUDDY) // 右伙伴
        {
            start = block.get_start();
            end = buddy->get_end();
            Block new_block{order, start, end, num_pages, -1};

            // Log("#空闲链表# ", block.str(), " ", buddy->str(), " 被合并成 ", new_block.str());

            this->insert_block(new_block); // 插入合并后的 block
        }
        this->free_list[buddy_order].erase(buddy); // 删除 free_list[buddy_order] 中已经被合并的 buddy
    }
}
```

#### 分配与释放伙伴系统内存

为了解决切分与合并过于频繁的问题，引入热页队列，暂存刚释放的单个物理页，将合并工作向后推迟（Lazy）。进行分配时总试图从热页队列中分配单个物理页，且分配与释放都在热页队列的队头进行。

##### 分配伙伴系统内存

进程调用 `this->zone.alloc(size, this->id)` 来向伙伴系统请求内存，`size` 为要请求的页面数量，`this->id` 是进程编号

过程如下：

- 首先向热页队列请求是否有足够的空闲页块，如果没有空闲页块就向空闲链表请求
- 如果有空闲页块但不足以满足请求，将队头的空闲页块插回空闲链表，看下一个队头的空闲页块是否满足
  - 如果找到可以满足的就分配
  - 如果热页队列为空了也没找到，就向空闲链表请求

```cpp
Block *Zone::alloc(int size, int owner)
{
    this->total_requests += 1;

    // 请求页面数小于等于 0，则无法分配
    if (size <= 0)
    {
        Log("#页面分配# ", "请求页面数 ", size, " 小于等于 0");
        this->other_fail += 1;
        return nullptr;
    }

    // 请求分配页面的数量是比 size 大的最小的 2 次幂，即 2^order
    int order = 0;
    while ((1 << order) < size)
        order++;

    // 如果请求的页面数量超过了该伙伴系统区域的最大页面数，则无法分配
    if (order > this->max_order)
    {
        int max_order = this->max_order;
        Log("#页面分配# ", "请求 2^", order, " 个 Block ，超过该伙伴系统区域的最大页面数 ", max_order);
        this->shortage_fail += 1;
        return nullptr;
    }

    // 首先从热页面队列中查找是否有空闲页面
    this->hot_pages_mutex.lock();
    while (this->hot_pages_queue.size() > 0)
    {
        Block *block = &this->hot_pages_queue.front();

        // 如果找到的空闲页面的页面数大于等于请求页面数，则将其分配给请求页面的进程
        if (block->get_order() >= order)
        {
            this->hot_pages_queue.pop();

            Log("#热页队列# ", block->str(), " 从热页队列中取出");

            block->set_owner(owner);

            Log("#页面分配# ", block->str(), " 从热页队列中分配给进程 ", owner);

            this->success += 1;
            this->full_pages += block->get_num_pages();
            this->empty_pages -= block->get_num_pages();

            // 释放互斥锁
            this->hot_pages_mutex.unlock();

            return block;
        }
        else // 没有找到就把当前的 block 插入回空闲页面链表中
        {
            this->hot_pages_queue.pop();

            Log("#热页队列# ", block->str(), " 从热页队列中取出");

            this->insert_block(*block);
        }
    }
    this->hot_pages_mutex.unlock();

    // 如果热页面队列中没有空闲页面，则从空闲页面链表中查找是否有空闲页面
    Block *block = this->get_block(order, owner);

    if (block != nullptr)
    {
        this->success += 1;
        this->full_pages += block->get_num_pages();
        this->empty_pages -= block->get_num_pages();
        Log("#页面分配# ", block->str(), " 从空闲页面链表中分配给进程 ", owner);
    }

    return block;
}
```

##### 释放伙伴系统内存

进程释放伙伴系统内存的操作（`buddy.hh`），直接加入热页队列，由热页队列守护线程处理

```cpp
void Zone::free(Block &block)
{
    // 使用 RAII 机制加锁
    std::lock_guard<std::mutex> lock(this->hot_pages_mutex);

    block.set_owner(-1);
    this->hot_pages_queue.push(block);

    this->full_pages -= block.get_num_pages();
    this->empty_pages += block.get_num_pages();

    Log("#热页队列# ", block.str(), " 加入热页队列");
}
```

#### 热页队列守护线程

分配与释放都首先在热页队列上进行，热页队列守护线程（`buddy.cpp`）会每隔 `lazy` 秒将热页队列队头的页块插回空闲链表

```cpp
void Zone::hot_pages_deamon()
{
    // Log("#热页队列# 守护线程启动");
    while (this->hot_pages_deamon_running)
    {
        // 加锁
        this->hot_pages_mutex.lock();

        // 每隔 lazy 时间，将热页面队列中的页面加入到空闲页面链表中
        if (this->hot_pages_queue.size() > 0)
        {
            Block block = this->hot_pages_queue.front();
            this->hot_pages_queue.pop();

            // Log("#热页队列# ", block.str(), " 加入到空闲页面链表中");

            this->insert_block(block);
        }

        // 解锁
        this->hot_pages_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(this->lazy));
    }

    this->hot_pages_mutex.lock();
    while (this->hot_pages_queue.size() > 0)
    {
        Block block = this->hot_pages_queue.front();
        this->hot_pages_queue.pop();

        // Log("#热页队列# ", block.str(), " 加入到空闲页面链表中");

        this->insert_block(block);
    }
    this->hot_pages_mutex.unlock();
}
```

当主线程结束后，会自动将热页队列内的所有页块插回空闲链表（`buddy.cpp`）

```cpp
Zone::~Zone()
{
    this->hot_pages_deamon_running = false;

    if (this->hot_pages_deamon_thread.joinable())
        this->hot_pages_deamon_thread.join();

    this->audit();
}
```

#### 统计信息

在页面分配释放时会自动记录统计信息，程序结束时输出，包含（`output.txt`）：

- 总的页块请求次数
- 成功次数
- 由于实际空间不足分配失败的次数
- 由于碎片而不能满足的失败次数
- 其他失败次数
- 已分配的页数
- 空闲的页数
- 热页队列当前状态
- 空闲链表当前状态

例如：

```
[2023-12-08 14:52:48] audit:
total_requests: 149
success: 142
shortage_fail: 6
fragment_fail: 1
other_fail: 0
full_pages: 0
empty_pages: 1024

[2023-12-08 14:52:48] hot_pages_queue: 
empty

[2023-12-08 14:52:48] free_list:
free_list[0]: 
free_list[1]: 
free_list[2]: 
free_list[3]: 
free_list[4]: 
free_list[5]: 
free_list[6]: Block{64[192-255],-1} Block{64[0-63],-1} 
free_list[7]: Block{128[64-191],-1} Block{128[256-383],-1} Block{128[640-767],-1} 
free_list[8]: Block{256[768-1023],-1} Block{256[384-639],-1} 
free_list[9]: 
free_list[10]: 

```

总共请求了 149 次，142 次分配成功，6 次由于实际空间不足分配失败，1 次由于碎片不能满足

最后空闲链表如上，可以看到由于随机的分配和释放有碎片的存在

分配失败的日志输出如下：

```
[2023-12-08 14:52:29] #空闲链表# 由于实际空间不足无法分配 2^6=64 个 Block 给进程 2
[2023-12-08 14:52:29] hot_pages_queue: 
empty

[2023-12-08 14:52:29] free_list:
free_list[0]: Block{1[1-1],-1} 
free_list[1]: Block{2[2-3],-1} 
free_list[2]: Block{4[4-7],-1} 
free_list[3]: Block{8[8-15],-1} 
free_list[4]: Block{16[16-31],-1} 
free_list[5]: Block{32[32-63],-1} 
free_list[6]: 
free_list[7]: 
free_list[8]: 
free_list[9]: 
free_list[10]: 

```

```
[2023-12-08 14:52:40] #空闲链表# 由于内存碎片无法分配 2^7=128 个 Block 给进程 2
[2023-12-08 14:52:40] hot_pages_queue: 
empty

[2023-12-08 14:52:40] free_list:
free_list[0]: Block{1[225-225],-1} 
free_list[1]: Block{2[226-227],-1} 
free_list[2]: Block{4[228-231],-1} 
free_list[3]: Block{8[232-239],-1} 
free_list[4]: Block{16[240-255],-1} 
free_list[5]: Block{32[704-735],-1} 
free_list[6]: Block{64[0-63],-1} Block{64[384-447],-1} 
free_list[7]: 
free_list[8]: 
free_list[9]: 
free_list[10]: 

```

详细日志均在 `output.txt` 中

### 6-2 Slab 实现



## 实验结果



## 实验总结

