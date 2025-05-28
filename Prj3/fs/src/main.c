
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"

// global variables
int ncyl, nsec;

#define ReplyYes()       \
    do {                 \
        printf("Yes\n"); \
        Log("Success");  \
    } while (0)
#define ReplyNo(x)      \
    do {                \
        printf("No\n"); \
        Warn(x);        \
    } while (0)

// return a negative value to exit
int handle_f(char *args) {
    if (cmd_f(ncyl, nsec) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to format");
    }
    return 0;
}

int handle_mk(char *args) {
    char name[MAXNAME];
    short mode = 0b1111; // 默认权限
    if (sscanf(args, "%s", name) == 1 && cmd_mk(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create file");
    }
    return 0;
}

int handle_mkdir(char *args) {
    char name[MAXNAME];
    short mode = 0b1111;
    if (sscanf(args, "%s", name) == 1 && cmd_mkdir(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create directory");
    }
    return 0;
}

int handle_rm(char *args) {
    char name[MAXNAME];
    if (sscanf(args, "%s", name) == 1 && cmd_rm(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove file");
    }
    return 0;
}

int handle_cd(char *args) {
    char name[256];
    if (sscanf(args, "%s", name) == 1 && cmd_cd(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to change directory");
    }
    return 0;
}

int handle_rmdir(char *args) {
    char name[MAXNAME];
    if (sscanf(args, "%s", name) == 1 && cmd_rmdir(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove directory");
    }
    return 0;
}


int handle_ls(char *args) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(&entries, &n) != E_SUCCESS) {
        ReplyNo("Failed to list files");
        return 0;
    }
    ReplyYes();
    free(entries);
    return 0;
}

int handle_cat(char *args) {
    char name[MAXNAME];
    uchar *buf = NULL;
    uint len;

    if (sscanf(args, "%s", name) == 1 && cmd_cat(name, &buf, &len) == E_SUCCESS) {
        ReplyYes();
        fwrite(buf, 1, len, stdout);
        printf("\n");
        free(buf);
    } else {
        ReplyNo("Failed to read file");
    }
    return 0;
}

int handle_w(char *args) {
    char name[MAXNAME];
    uint len;
    char *data;

    if (sscanf(args, "%s %u", name, &len) == 2) {
        data = strchr(args, ' ');
        data = strchr(data + 1, ' ');
        if (data && strlen(data + 1) >= len && cmd_w(name, len, data + 1) == E_SUCCESS) {
            ReplyYes();
            return 0;
        }
    }
    ReplyNo("Failed to write file");
    return 0;
}

int handle_i(char *args) {
    char name[MAXNAME];
    uint pos, len;
    char *data;

    if (sscanf(args, "%s %u %u", name, &pos, &len) == 3) {
        data = strchr(args, ' ');
        data = strchr(data + 1, ' ');
        data = strchr(data + 1, ' ');
        if (data && strlen(data + 1) >= len && cmd_i(name, pos, len, data + 1) == E_SUCCESS) {
            ReplyYes();
            return 0;
        }
    }
    ReplyNo("Failed to insert to file");
    return 0;
}

int handle_d(char *args) {
    char name[MAXNAME];
    uint pos, len;
    if (sscanf(args, "%s %u %u", name, &pos, &len) == 3 && cmd_d(name, pos, len) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to delete content");
    }
    return 0;
}

int handle_e(char *args) {
    printf("Bye!\n");
    Log("Exit");
    return -1;
}

int handle_login(char *args) {
    int uid;
    if (sscanf(args, "%d", &uid) == 1 && cmd_login(uid) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to login");
    }
    return 0;
}


static struct {
    const char *name;
    int (*handler)(char *);
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

FILE *log_file;

int main(int argc, char *argv[]) {
    log_init("fs.log");

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <DiskServerAddr> <Port>\n", argv[0]);
        exit(1);
    }

    const char *disk_addr = argv[1];
    int disk_port = atoi(argv[2]);
    init_disk_client(disk_addr, disk_port);

    assert(BSIZE % sizeof(dinode) == 0);

    // get disk info and store in global variables
    get_disk_info(&ncyl, &nsec);

    // read the superblock
    sbinit();

    static char buf[4096];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        buf[strlen(buf) - 1] = 0;
        Log("Use command: %s", buf);
        char *p = strtok(buf, " ");
        int ret = 1;
        for (int i = 0; i < NCMD; i++)
            if (p && strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(p + strlen(p) + 1);
                break;
            }
        if (ret == 1) {
            Log("No such command");
            printf("No\n");
        }
        if (ret < 0) break;
    }

    log_close();
}
