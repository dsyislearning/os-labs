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
    shm_id = shmget(key, 0, 0);
    if (shm_id == -1) {
        perror("shmget error");
        return -1;
    }

    printf("shm_id=%d\n", shm_id);

    // 将共享内存映射到当前进程的地址空间
    p_map = (people *)shmat(shm_id, NULL, 0);

    // 从共享内存中读取数据
    for (int i = 0; i < 3; i++) {
        printf("name=%s, age=%d\n", (p_map + i)->name, (p_map + i)->age);
    }

    // 断开与共享内存附加点的地址
    if (shmdt(p_map) == -1) {
        perror("detach error");
        return -1;
    }

    // 删除共享内存
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl error");
        return -1;
    }

    return 0;
}
