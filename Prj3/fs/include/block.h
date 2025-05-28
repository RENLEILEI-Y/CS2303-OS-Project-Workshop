/*======================  block.h  新增 / 补全  ======================*/
#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "common.h"      /* 提供 uint / uchar 等类型别名 */

/* ------------ 与块大小相关的常量 ------------ */
#define BSIZE 512               /* 每块字节数（模板一般已有，可留一份） */
#define BPB (BSIZE * 8)         /* Bits‑Per‑Block：一个数据块能容纳的位数 */

/*------------------------------------------------------------
 *  超级块 Superblock
 *    描述整个文件系统的宏观布局；磁盘第 0 块保存此结构
 *-----------------------------------------------------------*/
struct superblock {
    uint magic;         /* 魔数，用于校验磁盘是否已格式化 */
    uint size;          /* 磁盘总块数 */
    uint bmapstart;     /* 位图起始块号（连续若干块存储位向量） */
    /*  后续实现 inode 数据区时，可在此追加字段 */
    uint datastart;     /* 数据区（包括间接块和inode块）起始号 */
    uint ninodeblock;
    uint inodeblock[123];
};

/* 全局唯一的超级块实例；这里只是“声明”，真正的定义放在 fs.c */
extern struct superblock sb;

/*------------- 位图相关辅助宏 --------------*/
/* 给定逻辑块号 b，计算它位于哪一个“位图块” */
#define BBLOCK(b)  ((b) / BPB + sb.bmapstart)

/*--------------- 各种函数 -----------------*/
void zero_block(uint bno);
uint allocate_block();
void free_block(uint bno);

void get_disk_info(int *ncyl, int *nsec);
void read_block(int blockno, uchar *buf);
void write_block(int blockno, uchar *buf);
void _set_disk_geometry(int ncyl, int nsec);

void init_disk_client(const char*, int);

/*------------- 缓存机制 --------------*/
#define CACHE_CAPACITY 2  // 缓存大小

typedef struct CacheEntry {
    int blockno;                // 缓存的是哪个块
    uchar data[BSIZE];          // 缓存的数据
    int valid;                  // 是否有效
    struct CacheEntry *prev;
    struct CacheEntry *next;
} CacheEntry;

void init_block_cache();
void move_to_front(CacheEntry *entry);
CacheEntry* find_in_cache(int blockno);
void evict_and_insert(int blockno, uchar *data);
void clear_block_cache();

#endif /* _BLOCK_H_ */
