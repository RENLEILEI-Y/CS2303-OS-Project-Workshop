#include "inode.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "block.h"
#include "log.h"

extern uint current_uid;
extern struct superblock sb;
#define INODES_PER_BLOCK (BSIZE / sizeof(dinode))
#define IBLOCK(i) (sb.inodeblock[(i) / INODES_PER_BLOCK])  // inode 所在 block
#define IOFFSET(i) ((i) % INODES_PER_BLOCK)               // inode 在 block 中的偏移

// 获取编号为inum的inode
inode *iget(uint inum) {
    uchar buf[BSIZE];
    if (inum / INODES_PER_BLOCK >= sb.ninodeblock) {
        Warn("Invalid inode number");
        return NULL;
    }
    read_block(IBLOCK(inum), buf);

    dinode *dip = ((dinode *)buf) + IOFFSET(inum); // 找到对应的dinode
    if (dip->type == 0) return NULL;  // 未分配

    // 将储存在磁盘中的dinode读到内存中，成为inode
    inode *ip = malloc(sizeof(inode));
    ip->inum = inum;
    ip->type = dip->type;
    ip->size = dip->size;
    ip->blocks = dip->blocks;
    ip->mtime = dip->mtime;
    ip->ctime = dip->ctime;
    ip->owner = dip->owner;
    ip->perm = dip->perm;

    memcpy(ip->addrs, dip->addrs, sizeof(ip->addrs)); // 对于数组要单独用复制操作，否则只会复制指针
    return ip;
}

// 释放 inode（当前直接释放内存）
void iput(inode *ip) { free(ip); }

// 清空inode，以再之后重用
void ifree(inode *ip) {
    uchar buf[BSIZE];
    read_block(IBLOCK(ip->inum), buf);
    dinode *dip = (dinode *)buf + IOFFSET(ip->inum);
    memset(dip, 0, sizeof(dinode));
    write_block(IBLOCK(ip->inum), buf);
}

// 分配一个新的 inode，设置类型，初始化其内容
inode *ialloc(short type) {
    uchar buf[BSIZE];
    uint current_block = -1;

    uint max_inodes = sizeof(sb.inodeblock) / sizeof(uint) * INODES_PER_BLOCK; // 最大 inode 数
    for (uint inum = 0; inum < max_inodes; inum++) {
        // 如果检索的inode号超过已分配的inode块，则要新分配一个inode块
        if (inum / INODES_PER_BLOCK == sb.ninodeblock) {
            sb.inodeblock[sb.ninodeblock] = allocate_block();
            sb.ninodeblock++;
        }
        uint blkno = IBLOCK(inum);          // inode 所在 block
        if (blkno != current_block) {       // 如果这次检索的块与上一次的不同则重新读取，否则沿用上一次的
            read_block(blkno, buf);
            current_block = blkno;
        }
        dinode *dip = ((dinode *)buf) + IOFFSET(inum);  // inode 在块中的位置

        if (dip->type == 0) { // 如果这个编号（位置）的inode没有被占用，则用它
            dip->type = type;
            dip->size = 0;
            dip->blocks = 0;
            dip->ctime = dip->mtime = (uint)time(NULL);
            dip->owner = current_uid;
            dip->perm = 1;

            memset(dip->addrs, 0, sizeof(dip->addrs));

            write_block(blkno, buf);
            Log("[ialloc] Allocated inode #%d for type %d\n", inum, type);
            return iget(inum); // 返回储存在内存里的inode信息
        }
    }

    Error("ialloc: no free inode available");
    return NULL;
}

// 将内存中的 inode 内容写回磁盘
void iupdate(inode *ip) {
    // 找到内存inode对应dinode的位置，并读到内存中
    uchar buf[BSIZE];
    read_block(IBLOCK(ip->inum), buf);
    dinode *dip = ((dinode *)buf) + IOFFSET(ip->inum);

    // 在内存中修改数据
    dip->type = ip->type;
    dip->size = ip->size;
    dip->blocks = ip->blocks;
    dip->mtime = ip->mtime;
    dip->ctime = ip->ctime;
    dip->owner = ip->owner;
    dip->perm = ip->perm;
    memcpy(dip->addrs, ip->addrs, sizeof(dip->addrs));

    // 将修改的数据写回磁盘中
    write_block(IBLOCK(ip->inum), buf);
}

// 获取逻辑块号对应的物理块地址，必要时分配
static uint get_data_block(inode *ip, uint lbn, int alloc) {
    // 在直接块里放得下
    if (lbn < NDIRECT) {
        if (ip->addrs[lbn] == 0 && alloc) {
            ip->addrs[lbn] = allocate_block();
            zero_block(ip->addrs[lbn]);
            ip->blocks++;
        }
        return ip->addrs[lbn];
    }

    // 在直接块里放不下
    lbn -= NDIRECT;
    if (lbn < APB) {
        if (ip->addrs[NDIRECT] == 0 && alloc) { // 如果没有一级间接块且允许分配则先分配一级间接块
            ip->addrs[NDIRECT] = allocate_block();
            zero_block(ip->addrs[NDIRECT]);
            ip->blocks++;
        }
        else if (ip->addrs[NDIRECT] == 0 && !alloc) return 0; // 如果没有一级间接块且不允许分配，则直接退出

        // 有一级间接块，则使用
        uchar indirect[BSIZE];
        read_block(ip->addrs[NDIRECT], indirect); // 读入一级间接块
        uint *table = (uint *)indirect;
        if (table[lbn] == 0 && alloc) { // 如果间接块中对应的逻辑块未分配且允许分配则分配
            table[lbn] = allocate_block();
            zero_block(table[lbn]); // 对新分配的块置零
            write_block(ip->addrs[NDIRECT], indirect); // 将更新后的间接块写回
            ip->blocks++;
        }
        return table[lbn];
    }

    // 暂不支持二级间接块，可后续扩展
    return 0;
}

// 从inode索引的文件中读取数据到dst中，起始偏移量为off，读取字节数为n
int readi(inode *ip, uchar *dst, uint off, uint n) {
    if (off >= ip->size) return 0; // 如果偏移量超过文件大小，则直接返回0
    if (off + n > ip->size) n = ip->size - off; // 如果offset + read_len超过文件范围，则将read_len截断为实际可读字节数

    uint total = 0; // 已读取的字节数
    while (total < n) {
        uint lbn = (off + total) / BSIZE; // 计算要读的数据所在的逻辑块
        uint blockoff = (off + total) % BSIZE; // 计算块内偏移
        uint to_read = min(BSIZE - blockoff, n - total); // 计算当前最多读取多少，要么是当前块里剩下的，要么是剩下要读取的
        // 获取逻辑块号对应的物理块号
        uint bno = get_data_block(ip, lbn, 0);
        if (bno == 0) break; // 如果读到没有无效的块则中止
        // 读取磁盘并放入目标缓冲区
        uchar buf[BSIZE];
        read_block(bno, buf);
        memcpy(dst + total, buf + blockoff, to_read);
        // 更新已经读取的字节数
        total += to_read;
    }
    return total;
}

// 向inode索引的文件写入数据，起始偏移量为off，写入字节数为n
int writei(inode *ip, uchar *src, uint off, uint n) {
    uint total = 0; // 已写入的字节数
    while (total < n) {
        uint lbn = (off + total) / BSIZE; // 计算要写的数据所在的逻辑块
        uint blockoff = (off + total) % BSIZE; // 计算块内偏移
        uint to_write = min(BSIZE - blockoff, n - total); // 计算当前最多写多少，要么是当前块里剩下的，要么是剩下要写的
        // 获取逻辑块号对应的物理块号
        uint bno = get_data_block(ip, lbn, 1); // 允许新分配
        if (bno == 0) break;
        // 将块读入内存，修改后写入内存
        uchar buf[BSIZE];
        read_block(bno, buf);
        memcpy(buf + blockoff, src + total, to_write);
        write_block(bno, buf);

        total += to_write;
    }
    // 如果写入后文件大小增加，则更新dinode
    if (off + total > ip->size) ip->size = off + total;
    ip->mtime = (uint)time(NULL);  // 更新时间
    iupdate(ip);
    return total;
}
