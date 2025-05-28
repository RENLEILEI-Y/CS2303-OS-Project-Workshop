#include "block.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"
#include "log.h"
#include "tcp_utils.h"

static tcp_client disk_client;
static CacheEntry cache[CACHE_CAPACITY];
static CacheEntry *head = NULL, *tail = NULL;
static int cache_hits = 0;      // 命中次数
static int cache_accesses = 0;  // 总访问次数
// static int g_port = 0;
static int g_ncyl = 0;
static int g_nsec = 0;

// 初始化 disk server 连接
void init_disk_client(const char *addr, int port) {
    printf("addr: %s, port: %d\n", addr, port);
    disk_client = client_init(addr, port);

    // 请求几何信息
    client_send(disk_client, "I", 2);

    char buf[64];
    int n = client_recv(disk_client, buf, sizeof(buf));
    buf[n] = 0;

    if (strncmp(buf, "Yes ", 4) == 0) {
        sscanf(buf + 4, "%d %d", &g_ncyl, &g_nsec);
    } else {
        Warn("Failed to initialize disk geometry");
        g_ncyl = g_nsec = 0;
        exit(EXIT_FAILURE);
    }
}

/*--------------- 基本块 I/O 接口 ----------------*/
// 将号码为blockno的块的数据（512bits）读入buf中
void read_block(int blockno, uchar *buf) {
    // 先在缓存中查找
    cache_accesses++;
    CacheEntry *entry = find_in_cache(blockno);
    if (entry) {
        memcpy(buf, entry->data, BSIZE);
        cache_hits++;
        move_to_front(entry);
        Log("Cache hit for block %d (Block search: %d, Hit rate: %.2f%%)", blockno, cache_accesses, 100.0 * cache_hits / cache_accesses);
        return;
    }

    // 若未命中，则从磁盘读取
    // 通过block号确定柱面号和扇区号    
    int cyl = blockno / g_nsec;
    int sec = blockno % g_nsec;

    // 向磁盘服务器发送请求
    char cmd[64];
    sprintf(cmd, "R %d %d", cyl, sec);
    client_send(disk_client, cmd, strlen(cmd) + 1);

    // 获取磁盘服务器的回复
    char response[BSIZE + 10];
    client_recv(disk_client, response, sizeof(response));
    if (strncmp(response, "Yes ", 4) == 0) {
        Log("read_block: succeeded to read block %d", blockno);
        memcpy(buf, response + 4, BSIZE);
        evict_and_insert(blockno, buf);  // 加入缓存
    } else {
        memset(buf, 0, BSIZE);
        Warn("read_block: failed to read block %d", blockno);
    }
    Log("Cache miss for block %d (Block search: %d, Hit rate: %.2f%%)", blockno, cache_accesses, 100.0 * cache_hits / cache_accesses);
}

// 将buf中的数据写入号码为blockno的块中
void write_block(int blockno, uchar *buf) {
    // 通过block号确定柱面号和扇区号
    int cyl = blockno / g_nsec;
    int sec = blockno % g_nsec;

    // 向磁盘服务器发送请求
    char header[64];
    int len = BSIZE;
    sprintf(header, "W %d %d %d ", cyl, sec, len);
    int hlen = strlen(header);

    // 构造完整消息
    char *msg = malloc(hlen + BSIZE);
    memcpy(msg, header, hlen);
    memcpy(msg + hlen, buf, BSIZE);
    client_send(disk_client, msg, hlen + BSIZE);
    free(msg);

    // 获取磁盘服务器的回复
    char response[64];
    int n = client_recv(disk_client, response, sizeof(response));
    response[n] = 0;
    if (strcmp(response, "Yes") != 0) {
        Warn("write_block: failed to write block %d", blockno);
    }
    else { // 更新缓存
        CacheEntry *entry = find_in_cache(blockno);
        if (entry) {
            memcpy(entry->data, buf, BSIZE);
            move_to_front(entry);
        }
        else
            evict_and_insert(blockno, buf);  // 加入缓存
    }
}

// 清空号码为bno的块的数据（全部置零）
void zero_block(uint bno) {
    uchar zero[BSIZE] = {0};
    write_block(bno, zero);
}

/*--------------- 位图分配器 ---------------------*/
// 返回空闲的块号码
uint allocate_block() {
    uchar buf[BSIZE];
    for (uint b = sb.datastart; b < sb.size; b++) {
        uint bmap_blk = BBLOCK(b); // 计算b块的位向量信息储存在哪个位图块中
        read_block(bmap_blk, buf); // 读取相应位图块
        // 计算相应字节在位图块中的偏移
        int byte = (b % BPB) / 8;
        int bit  = (b % BPB) % 8;
        
        if (!(buf[byte] & (1 << bit))) {         // 找到空闲位
            buf[byte] |= 1 << bit;               // 占用
            write_block(bmap_blk, buf);          // 写回修改后的位图块 
            zero_block(b);                       // 清零后返回
            return b;
        }
    }
    Warn("allocate_block: disk used up");
    return 0;   // 约定 0 代表失败
}

void free_block(uint bno) {
    // 合法性检查
    if (bno == 0 || bno >= sb.size) {
        Warn("free_block: 非法块号 %u", bno);
        return;
    }
    // 清除块数据
    zero_block(bno);
    // 修改位向量
    uchar buf[BSIZE];
    uint bmap_blk = BBLOCK(bno);
    read_block(bmap_blk, buf);

    int byte = (bno % BPB) / 8;
    int bit  = (bno % BPB) % 8;
    buf[byte] &= ~(1 << bit); // 清空该位
    Log("Free block: %d\n", bno);
    write_block(bmap_blk, buf);
}

/*--------------- 几何信息 -----------------------*/
// 初始化时将磁盘的集合信息保存到本地
void _set_disk_geometry(int ncyl, int nsec) {
    g_ncyl = ncyl;
    g_nsec = nsec;
}

// 供程序获取磁盘的几何信息
void get_disk_info(int *ncyl, int *nsec) {
    if (ncyl) *ncyl = g_ncyl;
    if (nsec) *nsec = g_nsec;
}

/*------------------ 缓存模块 --------------------*/
// 初始化缓存
void init_block_cache() {
    for (int i = 0; i < CACHE_CAPACITY; ++i) {
        cache[i].blockno = -1;
        cache[i].valid = 0;
        cache[i].prev = NULL;
        cache[i].next = NULL;
    }
    head = tail = NULL;
}

// 查找缓存
CacheEntry* find_in_cache(int blockno) {
    for (int i = 0; i < CACHE_CAPACITY; ++i) {
        if (cache[i].valid && cache[i].blockno == blockno) {
            return &cache[i];
        }
    }
    return NULL;
}

// 移动到缓存头
void move_to_front(CacheEntry *entry) {
    if (entry == head) return;

    // 从链表中移除
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == tail) tail = entry->prev;

    // 插入到队头
    entry->prev = NULL;
    entry->next = head;
    if (head) head->prev = entry;
    head = entry;
    if (!tail) tail = entry;
}

// 淘汰尾部并插入新块
void evict_and_insert(int blockno, uchar *data) {
    CacheEntry *entry = NULL;

    // 查找空位
    for (int i = 0; i < CACHE_CAPACITY; ++i) {
        if (!cache[i].valid) {
            entry = &cache[i];
            break;
        }
    }

    // 无空位则替换尾部
    if (!entry) {
        Log("Cache for block %d evicted", tail->blockno);
        entry = tail;
        if (tail->prev) tail->prev->next = NULL;
        tail = tail->prev;
    }

    entry->blockno = blockno;
    entry->valid = 1;
    memcpy(entry->data, data, BSIZE);
    Log("Cache for block %d inserted", blockno);

    move_to_front(entry);
}

void clear_block_cache() {
    for (int i = 0; i < CACHE_CAPACITY; ++i) {
        cache[i].blockno = -1;
        cache[i].valid = 0;
        cache[i].prev = NULL;
        cache[i].next = NULL;
    }
    head = tail = NULL;
    cache_hits = 0;
    cache_accesses = 0;
    Log("Block cache cleared");
    printf("Block cache cleared.\n");
}
