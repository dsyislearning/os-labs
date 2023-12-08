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

### 运行方法

确保在当前工作目录下有以下文件：

```
.
├── block.cpp
├── block.hh
├── buddy.cpp
├── buddy.hh
├── main.cpp
├── main_slab.cpp
├── makefile
├── process.cpp
├── process.hh
├── slabcache.cpp
├── slabcache.hh
├── slab.cpp
├── slab.hh
├── slabobject.cpp
├── slabobject.hh
├── slabpage.cpp
├── slabpage.hh
└── utils.hh
```

#### 伙伴系统模拟程序

构建伙伴系统模拟程序

```zsh
make buddy
```

运行伙伴系统模拟程序

```zsh
./buddy
```

#### Slab 模拟程序

构建 Slab 模拟程序

```zsh
make slab
```

运行 Slab 模拟程序

```zsh
./slab
```

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

在Linux中，伙伴算法是以页为单位管理和分配内存。但是现实的需求却以字节为单位，假如我们需要申请20Bytes，总不能分配一页吧！那此不是严重浪费内存。那么该如何分配呢？slab分配器就应运而生了，专为小内存分配而生。slab分配器分配内存以字节为单位。但是slab分配器并没有脱离伙伴算法，而是基于伙伴算法分配的大内存进一步细分成小内存分配。

#### 数据对象定义

##### Slab 系统

外部调用 Slab 的接口

```cpp
class Slab
{
private:
    Zone &zone; // 调用的伙伴系统区域

    std::deque<SlabCache> caches; // 缓存列表

    bool cache_exists(int size);    // 判断是否存在对应对象大小的缓存
    SlabCache *get_cache(int size); // 获取对应对象大小的缓存

public:
    Slab(Zone &zone);
    ~Slab();

    bool alloc(int object_size); // 从缓存中分配一个对象
    bool free(int object_size);  // 释放一个对象

    void print_slab(); // 打印缓存列表
};
```

##### 缓存列表

缓存以缓存对象的大小来区分，所包含的三种slab都是链表，里面有一个或多个slab，每个slab由一个或多个连续的物理页组成，在物理页上保存的才是对象。

```cpp
class SlabCache
{
private:
    Zone &zone;                         // 调用的伙伴系统区域
    int object_size;                    // 该缓存管理的对象大小
    std::deque<SlabPage> slabs_full;    // 完全分配的 SlabPage
    std::deque<SlabPage> slabs_partial; // 部分分配的 SlabPage
    std::deque<SlabPage> slabs_free;    // 未分配的 SlabPage

    void reap(SlabPage &slabpage); // 回收 SlabPage 给伙伴系统

public:
    SlabCache(Zone &zone, int object_size);
    int get_object_size(); // 获取管理的对象大小
    bool alloc();          // 从缓存中分配一个对象
    bool free();           // 释放一个对象

    std::string str();
};
```

##### Slab

slab是slab分配器的最小单位，在实现上一个slab由一个或多个连续的物理页组成（通常只有一页）。单个slab可以在slab链表之间移动，例如如果一个半满slab被分配了对象后变满了，就要从slabs_partial中被删除，同时插入到slabs_full中去。

```cpp
class SlabPage
{
private:
    Zone &zone;             // 伙伴系统区域
    Block *block_ptr;       // 伙伴系统分配的内存块指针
    Block block;            // 伙伴系统分配的内存块
    int slab_size;          // SlabPage 的大小，单位为字节
    int object_size;        // 该 SlabPage 管理的对象大小，单位为字节
    int object_num;         // 该 SlabPage 管理的对象数量
    std::vector<bool> used; // 该 SlabPage 中对象的使用情况

public:
    SlabPage(Zone &zone, int object_size);
    Block get_block();

    bool failed();     // 判断是否从伙伴系统分配内存失败
    int alloc();       // 从 SlabPage 中分配一个对象，返回对象的下标
    bool free();       // 释放一个对象
    bool is_full();    // 判断 SlabPage 是否完全分配
    bool is_free();    //  判断 SlabPage 是否完全未分配
    bool is_partial(); // 判断 SlabPage 是否部分分配

    std::string str();
};
```

#### 分配

首先判断是否有管理某种对象大小的缓存，如果没有就创建，向伙伴系统申请内存，实现动态分配 Cache 的功能。

```cpp
bool Slab::alloc(int object_size)
{
    if (!cache_exists(object_size)) // 如果没有对应对象大小的缓存，就创建一个
    {
        caches.push_back(SlabCache(zone, object_size));
        // Log("创建 ", object_size, "B 对象的缓存");
    }

    SlabCache *cache = get_cache(object_size); // 获取对应对象大小的缓存
    // Log("获取 ", object_size, "B 对象的缓存");

    return cache->alloc(); // 从缓存中分配一个对象
}
```

#### 释放

首先根据释放对象大小找到管理该类对象的缓存，然后调用缓存的方法来释放对象

```cpp
bool Slab::free(int object_size)
{
    SlabCache *cache = get_cache(object_size); // 获取对应对象大小的缓存
    // Log("获取 ", object_size, "B 对象的缓存");

    if (cache != nullptr)
    {
        return cache->free(); // 释放对象
    }
    else
    {
        return false;
    }
}
```

缓存首先从 `slabs_full` 中释放对象，如果没有完全分配的 slab 就从 `slabs_partial` 中释放，如果部分分配的 slab 也没有，就说明释放失败

最后检查一下有没有完全未分配的 slab 在 `slabs_free` 中，如果有就将其回收给伙伴系统。

```cpp
bool SlabCache::free()
{
    bool success = false;

    if (!slabs_full.empty())
    {
        SlabPage slabpage = slabs_full.front();

        success = slabpage.free();

        // Log("从 slabs_full 中释放一个对象");

        if (slabpage.is_partial())
        {
            slabs_full.pop_front();
            slabs_partial.push_back(slabpage);
        }
        else if (slabpage.is_free())
        {
            slabs_full.pop_front();
            slabs_free.push_back(slabpage);
        }
    }
    else if (!slabs_partial.empty())
    {
        SlabPage &slabpage = slabs_partial.front();

        success = slabpage.free();

        // Log("从 slabs_partial 中释放一个对象");

        if (slabpage.is_free())
        {
            slabs_partial.pop_front();
            slabs_free.push_back(slabpage);
        }
    }

    if (!slabs_free.empty())
    {
        SlabPage &slabpage = slabs_free.front();
        slabs_free.pop_front();

        // Log("从 slabs_free 中回收 slabpage");

        reap(slabpage);
    }

    return success;
}
```

#### 空闲 Slab 回收

将 slab 回收给伙伴系统

```cpp
void SlabCache::reap(SlabPage &slabpage)
{
    Block block = slabpage.get_block();
    zone.free(block);
    // Log("回收页块 ", block.str());
}
```

如下是一部分 Slab 模拟的内容，可以清楚的看到 slab 的不同状态（处在哪一个链表中），以及分配与释放和回收的过程

```
[2023-12-08 23:10:02] #页面分配# Block{4[0-3],-1} 从空闲页面链表中分配给进程 -1
[2023-12-08 23:10:02] 分配 3072B 对象
[2023-12-08 23:10:02] slab:
slabcache[3072]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
[2023-12-08 23:10:02] #页面分配# Block{1[4-4],-1} 从空闲页面链表中分配给进程 -1
[2023-12-08 23:10:02] 分配 2048B 对象
[2023-12-08 23:10:02] slab:
slabcache[3072]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
slabcache[2048]:
	slabs_full: 
	slabs_partial: slabpage{1/2}
	slabs_free: 
[2023-12-08 23:10:02] #页面分配# Block{1[5-5],-1} 从空闲页面链表中分配给进程 -1
[2023-12-08 23:10:02] 分配 1024B 对象
[2023-12-08 23:10:02] slab:
slabcache[3072]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
slabcache[2048]:
	slabs_full: 
	slabs_partial: slabpage{1/2}
	slabs_free: 
slabcache[1024]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
[2023-12-08 23:10:02] #热页队列# Block{1[4-4],-1} 加入热页队列
[2023-12-08 23:10:02] 释放 2048B 对象
[2023-12-08 23:10:02] slab:
slabcache[3072]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
slabcache[2048]:
	slabs_full: 
	slabs_partial: 
	slabs_free: 
slabcache[1024]:
	slabs_full: 
	slabs_partial: slabpage{1/4}
	slabs_free: 
[2023-12-08 23:10:02] #热页队列# Block{1[5-5],-1} 加入热页队列
...
...
```

## 实验结果

### 伙伴系统模拟输出

在 `output.txt` 中

### Slab 模拟输出

在 `output_slab.txt` 中

## 实验总结

此次实验最主要的两大收获是：

- 理解了伙伴系统在内核分配中的作用，掌握了伙伴算法的分配与合并过程
- 理解了 Slab 在分配小内核内存中对伙伴系统的改进，掌握了 Slab 、Cache 、伙伴系统之间的数据流动关系

除了理解了两大内核分配方式的原理以外，还对我的程序设计能力以及 C++ 语言的理解有很大的提升：

- 巩固了 C++ 面向对象编程的方法
- 巩固了 C++ 多线程编程的方法，学习了 RAII 方式的加锁同步
- 深化了对引用和指针的理解，深化了对容器与迭代器的理解
- 巩固了程序设计规范性，如命名、方法设计、模块组织
- 加强了排错能力，根据日志、单步调试等方式进行排错
- 巩固了编写 makefile 构建 C++ 项目的方法
