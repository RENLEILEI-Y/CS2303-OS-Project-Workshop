/* fs.c - 实现基本文件系统中的目录与文件内容操作 */

#include "fs.h"
#include "block.h"
#include "inode.h"
#include "common.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

#define FS_MAGIC 0x2303A514

int current_uid = 0;
struct superblock sb;
inode *cwd = NULL;  // 当前工作目录
static char current_path[256] = "/"; // 当前工作目录的绝对路径

// 加载超级块，初始化在cmd_f中实现
void sbinit() {
    uchar buf[BSIZE];
    read_block(0, buf);
    memcpy(&sb, buf, sizeof(sb));
    if (sb.magic != FS_MAGIC) Warn("sbinit: 发现未知或未格式化的磁盘");
}

// 辅助函数：目录查找项
int dir_lookup(inode *dp, const char *name, uint *inum_out) {
    entry e;
    for (uint off = 0; off + sizeof(e) <= dp->size; off += sizeof(e)) {
        readi(dp, (uchar *)&e, off, sizeof(e));
        if (strncmp(e.name, name, MAXNAME) == 0) {
            if (inum_out) *inum_out = e.inum;
            return e.type;
        }
    }
    return 0;
}

// 辅助函数：添加目录项
int dir_add(inode *dp, const char *name, short type, uint inum) {
    // 若重名则直接返回错误
    if (dir_lookup(dp, name, NULL)) return 1;

    // 组织目录项内容
    entry e;
    memset(&e, 0, sizeof(e));
    strncpy(e.name, name, MAXNAME);
    e.type = type;
    e.inum = inum;

    // 以当前文件大小作为写入偏移
    uint offset = dp->size;

    // 把目录项写入目录文件
    int written = writei(dp, (uchar *)&e, offset, sizeof(e));
    if (written != sizeof(e)) return 1;  // 写失败

    /* writei() 已经把 dp->size 更新为 offset + written
       这里**千万不要**再自增，否则会多算一次！ */

    // 把内存中的inode刷回磁盘，保证size等元数据持久化
    iupdate(dp);
    return 0;
}

// 辅助函数：删除目录项
int dir_remove(inode *dp, const char *name) {
    entry e;
    for (uint off = 0; off + sizeof(e) <= dp->size; off += sizeof(e)) {
        readi(dp, (uchar *)&e, off, sizeof(e));
        if (strncmp(e.name, name, MAXNAME) == 0) {
            memset(&e, 0, sizeof(e));
            writei(dp, (uchar *)&e, off, sizeof(e));
            return 0;
        }
    }
    // 目录文件大小没有改变，不用iupdate更新
    return -1;
}

// rmdir的辅助函数：递归删除目录或文件
static void recursive_delete(inode *ip) {
    if (ip->type == T_FILE) {
        ifree(ip);
        iput(ip);
        return;
    }

    // 是目录，遍历子项
    entry e;
    for (uint off = 0; off + sizeof(e) <= ip->size; off += sizeof(e)) {
        readi(ip, (uchar *)&e, off, sizeof(e));
        if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0 || e.name[0] == '\0')
            continue;

        inode *child = iget(e.inum);
        if (child) {
            recursive_delete(child);
        }
    }

    // 清空当前目录
    ifree(ip);
    iput(ip);
}

// ls的辅助函数：递归统计某目录下所有文件（包括子目录内）的大小之和
uint calc_total_file_size(inode *dir) {
    uint total = 0;
    entry e;
    for (uint off = 0; off + sizeof(e) <= dir->size; off += sizeof(e)) {
        readi(dir, (uchar *)&e, off, sizeof(e));
        if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0 || e.name[0] == '\0')
            continue;
        inode *child = iget(e.inum);
        if (!child) continue;

        if (child->type == T_FILE) {
            total += child->size;
        } else if (child->type == T_DIR) {
            total += calc_total_file_size(child);  // 递归统计子目录
        }
        iput(child);
    }
    return total;
}

/* 辅助函数：路径解析
解析路径字符串，返回路径对应的 inode 指针
如果 name_out 不为 NULL，将路径最后一级的名字写入其中 */
inode *resolve_path(const char *path, char *name_out) {
    inode *ip = NULL;

    // 如果路径为空，返回 NULL
    if (!path || !*path) return NULL;

    // 如果路径是绝对路径，从根目录 inode 开始；否则从当前目录 cwd 开始
    ip = (*path == '/') ? iget(0) : iget(cwd->inum);

    // 为了不破坏原始字符串，先拷贝一份路径
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));

    // 使用 strtok 将路径按 '/' 分割，获取第一个路径分量
    char *p = strtok(path_copy, "/");

    // 遍历每一级路径
    while (p && ip) {
        inode *next = NULL;  // 保存下一层目录 inode
        uint inum;           // 临时存储查到的 inode 编号

        if (strcmp(p, ".") == 0) {
            // "." 表示当前目录，什么都不做
        } else if (strcmp(p, "..") == 0) {
            // ".." 表示父目录，查找父目录的 inode 编号
            dir_lookup(ip, "..", &inum);
            next = iget(inum);   // 获取父目录 inode
            iput(ip);            // 释放当前目录 inode
            ip = next;           // 进入父目录
        } else {
            // 普通目录项，比如 "a", "b", "file.txt"
            int type = dir_lookup(ip, p, &inum);  // 查找该项
            if (!type) {
                iput(ip);       // 找不到就释放当前 inode 并返回 NULL
                return NULL;
            }
            next = iget(inum);  // 找到则加载该目录项 inode
            iput(ip);           // 释放旧的 inode
            ip = next;          // 进入子目录或文件
        }

        // 继续处理下一层路径分量
        p = strtok(NULL, "/");
    }

    // 如果需要获取路径最后一级的名字（比如用在创建文件时）
    if (name_out) strncpy(name_out, p ? p : "", MAXNAME);

    // 返回路径解析到的 inode 指针
    return ip;
}

// 辅助函数：判断当前用户是否有权限
int has_permission(inode *ip, int required) {
    // 所有者或管理员
    if (current_uid == 1 || current_uid == ip->owner) return 1;
    return ip->perm >= required;
}

int cmd_f(int ncyl, int nsec) {
    // 只有 uid 为 1 的用户（超级用户）才允许执行格式化操作
    if (!current_uid) return E_NOT_LOGGED_IN;
    if (current_uid != 1) return E_PERMISSION_DENIED;

    // 计算总块数（每个 cylinder 有 nsec 个 sector，每个 sector 就是一个 block）
    uint nblocks = (uint)ncyl * (uint)nsec;
    if (nblocks == 0) return E_ERROR;

    // 设置磁盘几何信息（保存到全局变量）
    _set_disk_geometry(ncyl, nsec);


    // 清空 superblock，并设置必要字段
    memset(&sb, 0, sizeof(sb));
    uint nbitmap = (nblocks + BPB - 1) / BPB; // 向上取整
    sb.magic = FS_MAGIC;          // 魔数，用于判断是否格式化
    sb.size = nblocks;            // 总块数
    sb.bmapstart = 1;             // 位图起始块（superblock 是 block 0）
    sb.datastart = sb.bmapstart + nbitmap; // 数据起始快
    sb.ninodeblock = 0;

    // 清空位图所在的所有块（从 block 1 开始）
    uchar buf[BSIZE] = {0};
    memset(buf, 0, BSIZE);
    for (uint i = 0; i < nbitmap; i++) write_block(sb.bmapstart + i, buf);

    // 把 bitmap 本身对应的块标记为“已使用”（避免被当作数据块分配）
    for (uint b = 0; b <= nbitmap; b++) {
        uint map_blk = BBLOCK(b);     // 找到 bitmap 的块
        read_block(map_blk, buf);     // 读出这个 bitmap 块

        int byte = (b % BPB) / 8;      // 计算 bitmap 中字节索引
        int bit  = (b % BPB) % 8;      // 计算该字节中的位索引
        buf[byte] |= 1 << bit;         // 设置该位为 1，表示已使用

        write_block(map_blk, buf);    // 写回修改后的 bitmap 块
    }

    // 创建根目录 inode，类型为 T_DIR
    inode *root = ialloc(T_DIR);

    // 给根目录添加 "." 和 ".." 两个特殊目录项（都指向自己）
    dir_add(root, ".", T_DIR, root->inum);
    dir_add(root, "..", T_DIR, root->inum);

    // 把 inode 写回磁盘
    iupdate(root);

    // 替换当前工作目录 cwd 为新的根目录
    iput(cwd);                  // 释放旧的 cwd（如果有）
    cwd = iget(root->inum);     // 获取新的 inode 作为 cwd
    iput(root);                 // 释放刚刚分配的 root inode

    // 写入 superblock（block 0）
    memcpy(buf, &sb, sizeof(sb));
    write_block(0, buf);

    // 工作目录回到根目录
    memcpy(current_path, "/", 2);

    return E_SUCCESS;
}

int cmd_login(int auid) {
    if (auid <= 0) return E_ERROR;
    if (current_uid > 0) return E_PERMISSION_DENIED;
    current_uid = auid;

    // 初始化根目录 inode
    cwd = iget(0);
    if (!cwd) return E_ERROR;

    // 为该用户创建 home 目录（例如 /2）
    char username[16];
    snprintf(username, sizeof(username), "%d", auid);
    if (!dir_lookup(cwd, username, NULL)) {
        // 暂时将
        cwd->perm = 2;
        short mode = 0b1111; // 默认权限
        cmd_mkdir(username, mode);
        cwd->perm = 1;
    }
    memcpy(current_path, "/", 2);
    Log("user %d logged in", current_uid);

    return E_SUCCESS;
}

int cmd_mk(char *name, short mode) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    if (!has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限
    if (dir_lookup(cwd, name, NULL)) {
        Warn("cmd_mk: name already exists");
        return E_ERROR;
    }

    inode *ip = ialloc(T_FILE);
    if (!ip) return E_ERROR;
    Log("New file inode #%d for '%s'\n", ip->inum, name);

    if (dir_add(cwd, name, T_FILE, ip->inum))
        Warn("cmd_mk: failed to add file entry");
    iupdate(cwd);
    iput(ip);
    return E_SUCCESS;
}

int cmd_mkdir(char *name, short mode) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    if (!has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (dir_lookup(cwd, name, NULL)) {
        Warn("cmd_mkdir: name already exists");
        return E_ERROR;
    }

    inode *ip = ialloc(T_DIR);
    if (!ip) return E_ERROR;
    Log("New dir inode #%d for '%s'\n", ip->inum, name);

    // 初始化 "." 和 ".." 目录项
    dir_add(ip, ".", T_DIR, ip->inum);
    dir_add(ip, "..", T_DIR, cwd->inum);
    iupdate(ip);

    // 将新目录添加到当前工作目录
    if (dir_add(cwd, name, T_DIR, ip->inum))
        Warn("cmd_mkdir: failed to add directory entry");

    iupdate(cwd);
    iput(ip);
    return E_SUCCESS;
}

int cmd_cd(char *name) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    inode *ip = resolve_path(name, NULL);
    if (!ip || ip->type != T_DIR) return E_ERROR;
    if (!has_permission(ip, 1)) return E_PERMISSION_DENIED; // 检查权限

    // 修改cwd指针
    iput(cwd);
    cwd = ip;

    // 更新路径字符串
    if (name[0] == '/') {
        // 绝对路径
        strncpy(current_path, name, sizeof(current_path));
    } else {
        // 相对路径：拼接
        if (strcmp(name, "..") == 0) {
            // 删除末尾一层
            char *last = strrchr(current_path, '/');
            if (last != NULL && last != current_path) {
                *last = '\0';
            } else {
                strcpy(current_path, "/");
            }
        } else if (strcmp(name, ".") != 0) {
            // 追加子目录
            if (strcmp(current_path, "/") != 0)
                strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);
            strncat(current_path, name, sizeof(current_path) - strlen(current_path) - 1);
        }
    }
    return E_SUCCESS;
}

char* get_path() {
    size_t n = strlen(current_path) + 20;
    char *rep = (char *)malloc(n * sizeof(char));
    rep[0] = 0;
    snprintf(rep, n, "user_%d:%s$", current_uid, current_path);
    return rep;
}

int cmd_ls(entry **e, int *n) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;

    int raw_cnt = cwd->size / sizeof(entry);
    entry *all = malloc(raw_cnt * sizeof(entry));
    int cnt = 0;

    for (int i = 0; i < raw_cnt; i++) {
        entry tmp;
        readi(cwd, (uchar *)&tmp, i * sizeof(entry), sizeof(entry));
        if (tmp.name[0] == '\0' || strcmp(tmp.name, ".") == 0 || strcmp(tmp.name, "..") == 0)
            continue;
        inode *ip = iget(tmp.inum);
        if (ip) {
            all[cnt] = tmp;
            if (ip->type == T_DIR) {
                // 使用递归计算该目录下所有文件大小之和
                all[cnt].size = calc_total_file_size(ip);
            } else {
                all[cnt].size = ip->size;
            }
            all[cnt].mtime = ip->mtime;
            all[cnt].ctime = ip->ctime;
            all[cnt].owner = ip->owner;
            all[cnt].perm = ip->perm;
            iput(ip);
            cnt++;
        }
    }

    *e = malloc(cnt * sizeof(entry));
    memcpy(*e, all, cnt * sizeof(entry));
    free(all);

    *n = cnt;
    return E_SUCCESS;
}

int cmd_rm(char *name) {
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;
    inode *ip = iget(inum);
    
    if (!has_permission(ip, 2) || !has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_FILE) {
        iput(ip);
        return E_ERROR;
    }
    dir_remove(cwd, name);
    iupdate(cwd);
    ifree(ip);
    iput(ip);
    return E_SUCCESS;
}

int cmd_rmdir(char *name) {
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;

    inode *ip = iget(inum);
    if (!has_permission(ip, 2) || !has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_DIR) {
        iput(ip);
        return E_ERROR;
    }

    // 调用递归删除
    recursive_delete(ip);

    // 从父目录移除项
    dir_remove(cwd, name);
    iupdate(cwd);
    return E_SUCCESS;
}

int cmd_cat(char *name, uchar **buf, uint *len) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;
    inode *ip = iget(inum);
    if (!has_permission(ip, 1)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_FILE) {
        iput(ip);
        return E_ERROR;
    }
    *len = ip->size;
    *buf = malloc(ip->size);
    readi(ip, *buf, 0, ip->size);
    iput(ip);
    return E_SUCCESS;
}

int cmd_w(char *name, uint l, const char *data) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;
    inode *ip = iget(inum);
    if (!has_permission(ip, 2) || !has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_FILE) {
        iput(ip);
        return E_ERROR;
    }
    writei(ip, (uchar *)data, 0, l);
    iput(ip);
    return E_SUCCESS;
}

int cmd_i(char *name, uint p, uint l, const char *data) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;
    inode *ip = iget(inum);
    if (!has_permission(ip, 2) || !has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_FILE) {
        iput(ip);
        return E_ERROR;
    }
    // 如果pos比文件还大，直接加在文件末尾
    if (p > ip->size)
        p = ip->size;
    uchar *tmp = malloc(ip->size + l);
    readi(ip, tmp, 0, p);
    memcpy(tmp + p, data, l);
    readi(ip, tmp + p + l, p, ip->size - p);
    writei(ip, tmp, 0, ip->size + l);
    free(tmp);
    iput(ip);
    return E_SUCCESS;
}

int cmd_d(char *name, uint p, uint l) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;

    inode *ip = iget(inum);
    if (!has_permission(ip, 2) || !has_permission(cwd, 2)) return E_PERMISSION_DENIED; // 检查权限

    if (ip->type != T_FILE) {
        iput(ip);
        return E_ERROR;
    }

    if (p >= ip->size) {
        iput(ip);
        return E_SUCCESS;  // 删除范围超出文件末尾，直接返回成功
    }

    uint old_size = ip->size;  // 保存原始大小
    uint actual_del = min(l, old_size - p);  // 实际删除字节数
    uint new_len = old_size - actual_del;

    uchar *tmp = malloc(new_len);

    // 拷贝删除前的前半段数据
    readi(ip, tmp, 0, p);

    // 拷贝删除后的后半段数据
    readi(ip, tmp + p, p + actual_del, old_size - p - actual_del);

    // 写回数据
    writei(ip, tmp, 0, new_len);

    // 修改文件大小
    ip->size = new_len;
    iupdate(ip);

    free(tmp);
    iput(ip);
    return E_SUCCESS;
}

int cmd_chmod(char *name, int perm, int kernel) {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;
    if (!cwd || perm < 0 || perm > 2) return E_ERROR;

    uint inum;
    if (!dir_lookup(cwd, name, &inum)) return E_ERROR;

    inode *ip = iget(inum);
    if (!ip) return E_ERROR;

    // 只有管理员或 owner 可修改权限，工作在内核模式下也可以修改权限
    if (!kernel && ip->owner != current_uid && current_uid != 1) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }
        ip->perm = (ushort)perm;

    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}

int cmd_logout() {
    if (!cwd) return E_NOT_LOGGED_IN;
    if (current_uid == 1) return E_PERMISSION_DENIED;
    if (sb.magic != FS_MAGIC) return E_NOT_FORMATTED;

    // 用户目录名就是 UID 字符串
    char username[16];
    snprintf(username, sizeof(username), "%d", current_uid);

    // 找到根目录
    inode *root = iget(0);
    if (!root) return E_ERROR;

    // 查找该用户的 home 目录 inode
    uint inum;
    if (!dir_lookup(root, username, &inum)) {
        iput(root);
        return E_ERROR;
    }

    inode *user_dir = iget(inum);
    if (!user_dir || user_dir->type != T_DIR) {
        iput(root);
        if (user_dir) iput(user_dir);
        return E_ERROR;
    }

    // 删除整个目录树
    recursive_delete(user_dir);

    // 从根目录中移除此用户目录项
    dir_remove(root, username);
    iupdate(root);

    // 清理工作目录与UID
    iput(cwd);
    cwd = NULL;
    current_uid = 0;
    strcpy(current_path, "/");

    iput(root);
    return E_SUCCESS;
}