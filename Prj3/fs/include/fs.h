#ifndef __FS_H__
#define __FS_H__

#include "common.h"
#include "inode.h"
#include "stdlib.h"

// used for cmd_ls
typedef struct {
    char name[MAXNAME];  // 目录项名称
    short type;          // 目录项类型
    uint inum;           // inode号
    uint size;
    uint mtime;
    uint ctime;
    uint owner;
    ushort perm;
} entry;

void sbinit();

int cmd_f(int ncyl, int nsec);

int cmd_mk(char *name, short mode);
int cmd_mkdir(char *name, short mode);
int cmd_rm(char *name);
int cmd_rmdir(char *name);

int cmd_cd(char *name);
int cmd_ls(entry **entries, int *n);

int cmd_cat(char *name, uchar **buf, uint *len);
int cmd_w(char *name, uint len, const char *data);
int cmd_i(char *name, uint pos, uint len, const char *data);
int cmd_d(char *name, uint pos, uint len);

int cmd_login(int auid);
int cmd_chmod(char *name, int perm, int kernel);
int cmd_logout();

char* get_path();

#endif