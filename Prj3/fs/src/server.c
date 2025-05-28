
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"
#include "tcp_utils.h"

// global variables
int ncyl, nsec;
extern inode *cwd;
extern int current_uid;

static void server_reply(tcp_buffer *wb, const char* rep) {
    reply(wb, rep, strlen(rep) + 1);
}

// return a negative value to exit
int handle_f(tcp_buffer *wb, char *args) {
    int ret = cmd_f(ncyl, nsec);
    switch (ret) {
        case E_SUCCESS:
            server_reply(wb, "Format Successfully");
            break;
        case E_ERROR:
            server_reply(wb, "Failed to format");
            break;
        case E_NOT_LOGGED_IN:
            server_reply(wb, "Please login first");
            break;
        case E_PERMISSION_DENIED:
            server_reply(wb, "Permission denied");
            break;
        default:
            server_reply(wb, "Unexpected reply");
    }
    return 0;
}

int handle_mk(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "mk: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "Invalid arguments");
    else {
        short mode = 0b1111; // 默认权限
        int ret = cmd_mk(name, mode);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "File created successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to create file");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_mkdir(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "mkdir: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "mkdir: Invalid arguments");
    else {
        short mode = 0b1111; // 默认权限
        int ret = cmd_mkdir(name, mode);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "Directory created successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to create directory");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_rm(tcp_buffer *wb, char *args) {
    if (!args) {
        char *rep = "rm: Invalid arguments";
        reply_with_no(wb, rep, strlen(rep) + 1);
        return 0;
    }
    char name[MAXNAME];
    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "rm: Invalid arguments");
    else {
        int ret = cmd_rm(name);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "File removed successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to remove file");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_cd(tcp_buffer *wb, char *args) {
    if (!args) {
        char *rep = "cd: Invalid arguments";
        reply_with_no(wb, rep, strlen(rep) + 1);
        return 0;
    }
    char name[256];
    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "cd: Invalid arguments");
    else {
        int ret = cmd_cd(name);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "Directory changed successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to change directory");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_rmdir(tcp_buffer *wb, char *args) {
    if (!args) {
        char *rep = "rmdir: Invalid arguments";
        reply_with_no(wb, rep, strlen(rep) + 1);
        return 0;
    }
    char name[MAXNAME];
    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "rmdir: Invalid arguments");
    else {
        int ret = cmd_rmdir(name);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "Directory removed successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to remove directory");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

const char *perm_str(int perm) { // handle_ls辅助函数
    switch (perm) {
        case 0: return "---";
        case 1: return "r--";
        case 2: return "rw-";
        default: return "???";
    }
}

int handle_ls(tcp_buffer *wb, char *args) {
    entry *entries = NULL;
    int n = 0;
    int ret = cmd_ls(&entries, &n);
    if (ret != E_SUCCESS) {
        switch (ret) {
            case E_ERROR:
                server_reply(wb, "Failed to list");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
        return 0;
    }
    size_t rep_size = 100 * (n + 1);
    char *rep = malloc(rep_size);
    rep[0] = 0;
    int len = snprintf(rep, rep_size, "%-12s %-6s %-6s %-6s %s  %s          %s\n", "name", "type", "owner", "perm", "size(B)", "last modify", "create time");
    if (len >= rep_size) {
        Warn("Output out of bound");
        rep[rep_size - 1] = '\0';
    }
    for (int i = 0; i < n; i++) {
        const char *type_str = (entries[i].type == T_DIR) ? "DIR" : (entries[i].type == T_FILE) ? "FILE" : "UNKNOWN";

        char timebuf[32], ctimebuf[32];
        time_t mtime = entries[i].mtime;
        time_t ctime = entries[i].ctime;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&mtime));
        strftime(ctimebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&ctime));

        char add[100] = {0};
        int len = snprintf(add, sizeof(add), "%-12s %-6s %-6u %-4s   %-6u   %s  %s\n", entries[i].name, type_str, entries[i].owner, perm_str(entries[i].perm), entries[i].size, timebuf, ctimebuf);
        if (len > sizeof(add)) {
            Warn("Output out of bound");
            add[99] = '\0';
        }
        strcat(rep, add);
    }
    rep[strlen(rep) - 1] = '\0';
    reply(wb, rep, strlen(rep) + 1);
    free(entries);
    free(rep);
    return 0;
}

int handle_cat(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "cat: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    uchar *buf = NULL;
    uint len;

    if (sscanf(args, "%s", name) != 1)
        server_reply(wb, "cat: Invalid arguments");
    else {
        int ret = cmd_cat(name, &buf, &len);
        switch (ret) {
            case E_SUCCESS:
                reply(wb, (char *)buf, len);
                // reply_with_yes(wb, NULL, 0);
                free(buf);
                break;
            case E_ERROR:
                server_reply(wb, "Failed to read file");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_w(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "w: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    uint len;
    char *data;

    if (sscanf(args, "%s %u", name, &len) == 2) {
        data = strchr(args, ' ');
        data = strchr(data + 1, ' ');
        if (data && strlen(data + 1) >= len) {
            int ret = cmd_w(name, len, data + 1);
            switch (ret) {
                case E_SUCCESS:
                    server_reply(wb, "Write file successfully");
                    break;
                case E_ERROR:
                    server_reply(wb, "Failed to write file");
                    break;
                case E_NOT_LOGGED_IN:
                    server_reply(wb, "Please login first");
                    break;
                case E_PERMISSION_DENIED:
                    server_reply(wb, "Permission denied");
                    break;
                case E_NOT_FORMATTED:
                    server_reply(wb, "Not formatted");
                    break;
                default:
                    server_reply(wb, "Unexpected reply");
            }
            return 0;
        }
    }
    server_reply(wb, "w: Invalid arguments");
    return 0;
}

int handle_i(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "i: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    uint pos, len;
    char *data;

    if (sscanf(args, "%s %u %u", name, &pos, &len) == 3) {
        data = strchr(args, ' ');
        data = strchr(data + 1, ' ');
        data = strchr(data + 1, ' ');
        if (data && strlen(data + 1) >= len) {
            int ret = cmd_i(name, pos, len, data + 1);
            switch (ret) {
                case E_SUCCESS:
                    server_reply(wb, "Insert file successfully");
                    break;
                case E_ERROR:
                    server_reply(wb, "Failed to insert file");
                    break;
                case E_NOT_LOGGED_IN:
                    server_reply(wb, "Please login first");
                    break;
                case E_PERMISSION_DENIED:
                    server_reply(wb, "Permission denied");
                    break;
                case E_NOT_FORMATTED:
                    server_reply(wb, "Not formatted");
                    break;
                default:
                    server_reply(wb, "Unexpected reply");
            }
            return 0;
        }
    }
    server_reply(wb, "i: Invalid arguments");
    return 0;
}

int handle_d(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "d: Invalid arguments");
        return 0;
    }
    char name[MAXNAME];
    uint pos, len;
    if (sscanf(args, "%s %u %u", name, &pos, &len) == 3) {
        int ret = cmd_d(name, pos, len);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "Delete file successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to delete file");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    } else
        server_reply(wb, "d: Invalid arguments");
    return 0;
}

int handle_e(tcp_buffer *wb, char *args) {
    reply(wb, "Bye!", 5);
    Log("Exit");
    return -1;
}

int handle_login(tcp_buffer *wb, char *args) {
    if (!args) {
        server_reply(wb, "Failed to login");
        return 0;
    }
    int uid;
    if (sscanf(args, "%d", &uid) != 1)
        server_reply(wb, "Invalid argument");
    else {
        int ret = cmd_login(uid);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "User login");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to login");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "User already logged in");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    }
    return 0;
}

int handle_path(tcp_buffer *wb, char *args) {
    if (cwd && current_uid != 0) {
        char *rep = get_path();
        reply(wb, rep, strlen(rep) + 1);
        free(rep);
    }
    else
        reply(wb, NULL, 0);
    return 0;
}

int handle_logout(tcp_buffer *wb, char *args) {
    int ret = cmd_logout();
    switch (ret) {
        case E_SUCCESS:
            server_reply(wb, "User logout and directory deleted");
            return -1; // 强制断开连接
            break;
        case E_ERROR:
            server_reply(wb, "Failed to logout");
            break;
        case E_PERMISSION_DENIED:
            server_reply(wb, "Superuser cannot logout");
            break;
        case E_NOT_LOGGED_IN:
            server_reply(wb, "Please login first");
            break;
        default:
            server_reply(wb, "Unexpected reply");
    }
    return 0;
}

int handle_chmod(tcp_buffer *wb, char *args) {
    char name[MAXNAME];
    int perm;
    if (sscanf(args, "%s %d", name, &perm) == 2) {
        int ret = cmd_chmod(name, perm, 0);
        switch (ret) {
            case E_SUCCESS:
                server_reply(wb, "Change permission successfully");
                break;
            case E_ERROR:
                server_reply(wb, "Failed to change permission");
                break;
            case E_NOT_LOGGED_IN:
                server_reply(wb, "Please login first");
                break;
            case E_PERMISSION_DENIED:
                server_reply(wb, "Permission denied");
                break;
            case E_NOT_FORMATTED:
                server_reply(wb, "Not formatted");
                break;
            default:
                server_reply(wb, "Unexpected reply");
        }
    } else
        server_reply(wb, "chmod: Invalid arguments");
    return 0;
}

int handle_clearcache(tcp_buffer *wb, char *args) {
    clear_block_cache();
    reply_with_yes(wb, "Cache cleared", 14);
    return 0;
}

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static struct {
    const char *name;
    int (*handler)(tcp_buffer *, char *);
} cmd_table[] = {{"f", handle_f},         {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},       {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},         {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}, {"p", handle_path},      {"chmod", handle_chmod}, {"logout", handle_logout},
                 {"clearcache", handle_clearcache}};

void on_connection(int id) {
    Log("client connecting");
};
void clean_up(int id) {
    Log("client leaving: user %d", current_uid);
    current_uid = 0;
};

int on_recv(int id, tcp_buffer *wb, char *msg, int len) {
    char dupmsg[strlen(msg) + 1];
    memcpy(dupmsg, msg, strlen(msg) + 1);
    char *p = strtok(dupmsg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (p && strcmp(p, cmd_table[i].name) == 0) {
            char *inp = strchr(msg, ' ');
            if (!inp && (strcmp(msg, "ls") == 0 || strcmp(msg, "logout") || strcmp(msg, "clearcache"))) inp = msg;
            else if (inp) inp = inp + 1;
            ret = cmd_table[i].handler(wb, inp);
            break;
        }
    if (ret == 1) {
        static char unk[] = "Unknown command";
        buffer_append(wb, unk, sizeof(unk));
    }
    if (ret < 0) {
        return -1;
    }
    return 0;
}

FILE *log_file;

int main(int argc, char *argv[]) {
    log_init("fs.log");

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <DiskServerAddr> <DisckServerPort> <FileSystemServerPort>\n", argv[0]);
        exit(1);
    }

    const char *disk_addr = argv[1];
    int disk_port = atoi(argv[2]);
    int fs_port = atoi(argv[3]);
    init_disk_client(disk_addr, disk_port);

    assert(BSIZE % sizeof(dinode) == 0);

    // get disk info and store in global variables
    get_disk_info(&ncyl, &nsec);

    // read the superblock
    sbinit();

    init_block_cache(); // 初始化缓存
    tcp_server server = server_init(fs_port, 1, on_connection, on_recv, clean_up);
    server_run(server);

    log_close();
}
