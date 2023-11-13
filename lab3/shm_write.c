#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char name[8];
    int age;
} people;

int main(int argc, char *argv[])
{
    int shm_id;
    key_t key;
    
    char temp[8];
    people * p_map;
    
    char pathname[30];
    strcpy(pathname, "/tmp");
    
    key = ftok(pathname, 0x04);
    if (key == -1) {
        perror("ftok error");
        return -1;
    }
    
    printf("key=%d\n", key);

    // 创建共享内存
    shm_id = shmget(key, 4096, IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        perror("shmget error");
        return -1;
    }

    printf("shm_id=%d\n", shm_id);

    // 将共享内存映射到当前进程的地址空间
    p_map = (people *)shmat(shm_id, NULL, 0);

    memset(temp, 0, sizeof(temp));
    strncpy(temp, "test", 4);
    temp[4] = '\0';

    // 向共享内存中写入数据
    for (int i = 0; i < 3; i++) {
        temp[4] += 1;
        strncpy((p_map + i)->name, temp, 5);
        (p_map + i)->age = 0 + i;
    }

    // 断开与共享内存附加点的地址
    if (shmdt(p_map) == -1) {
        perror("detach error");
        return -1;
    }

    return 0;
}
