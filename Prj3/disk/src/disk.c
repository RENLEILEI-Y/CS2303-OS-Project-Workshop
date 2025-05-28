#include "disk.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include<math.h>

#include "log.h"

// global variables
static struct Disk {
    long FILESIZE;  // 磁盘大小
    int _ncyl;      // 磁盘柱面数
    int _nsec;      // 每个柱面的扇区数
    int ttd;        // 相邻磁道间的寻道时间
    int fd;         // 文件描述符
    char *diskfile; // 内存映射

} disk;

static const int BLOCKSIZE = 512; // 数据块大小
static int cur_cyl = 0;

// 磁盘初始化
int init_disk(char *filename, int ncyl, int nsec, int ttd) {
    disk._ncyl = ncyl;
    disk._nsec = nsec;
    disk.ttd = ttd;
    // do some initialization...

    // open file
    disk.fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (disk.fd < 0) {
        printf("Error: Could not open file '%s'.\n", filename);
        exit(EXIT_FAILURE);
    }
    // stretch the file
    disk.FILESIZE = BLOCKSIZE * disk._ncyl * disk._nsec;
    // 原先的lseek方法操作会报错
    if (ftruncate(disk.fd, disk.FILESIZE) < 0) {
        Log("Error calling ftruncate() to stretch the file: %s", strerror(errno));
        close(disk.fd);
        return -1;
    }
    // mmap
    disk.diskfile = (char *) mmap(NULL, disk.FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, disk.fd, 0);
    if (disk.diskfile == MAP_FAILED) {
        close(disk.fd);
        Log("Could not map file: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    Log("Disk initialized: %s, %d Cylinders, %d Sectors per cylinder", filename, ncyl, nsec);
    return 0;
}

// 获取磁盘信息
int cmd_i(int *ncyl, int *nsec) {
    // 获取磁盘信息
    *ncyl = disk._ncyl;
    *nsec = disk._nsec;
    return 0;
}

// 读操作
int cmd_r(int cyl, int sec, char *buf) {
    // 合法性检查
    if (cyl >= disk._ncyl || sec >= disk._nsec || cyl < 0 || sec < 0) {
        Log("Invalid cylinder or sector: cyl=%d, sec=%d", cyl, sec);
        return 1;
    }

    // 寻道
    usleep(1000 * abs(cyl-cur_cyl) * disk.ttd); // 寻道时间
    cur_cyl = cyl; // 更新柱面

    // 读数据并更新日志
    memcpy(buf, &disk.diskfile[BLOCKSIZE * (cyl * disk._nsec + sec)], BLOCKSIZE);
    Log("Read sector: cyl=%d, sec=%d", cyl, sec);
    return 0;
}

// 写操作
int cmd_w(int cyl, int sec, int len, char *data) {
    // 合法性检查
    if (cyl >= disk._ncyl || sec >= disk._nsec || cyl < 0 || sec < 0) { // 地址必须有效
        Log("Invalid cylinder or sector: cyl=%d, sec=%d", cyl, sec);
        return 1;
    }
    if (len > BLOCKSIZE || len <= 0) { // 写入数据大小不能超过一个block
        Log("Invalid data length: %d (must be 1-512)", len);
        return 1;
    }

    // 寻道
    off_t offset = BLOCKSIZE * (cyl * disk._nsec + sec);
    usleep(1000 * abs(cyl-cur_cyl) * disk.ttd); // 寻道时间
    cur_cyl = cyl; // 更新柱面

    if (offset + BLOCKSIZE > disk.FILESIZE) {
        Log("Offset out of bound: offset=%ld, file size=%ld", offset, disk.FILESIZE);
        return 1;
    }

    // 写数据并更新日志
    memcpy(&disk.diskfile[offset], data, len);
    if (len < BLOCKSIZE) {
        memset(&disk.diskfile[offset + len], 0, BLOCKSIZE - len);
    }
    Log("Wrote sector: cyl:%d, sec=%d, len=%d", cyl, sec, len);
    return 0;
}

// 关闭磁盘
void close_disk() {
    // close the file
    if (disk.diskfile != MAP_FAILED)
        munmap(disk.diskfile, disk.FILESIZE);
    if (disk.fd >= 0)
        close(disk.fd);
}