#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disk.h"
#include "log.h"
#include "tcp_utils.h"

static const int BLOCKSIZE = 512;

int handle_i(tcp_buffer *wb, char *args, int len) {
    int ncyl, nsec;
    cmd_i(&ncyl, &nsec);
    static char buf[64];
    sprintf(buf, "Yes %d %d", ncyl, nsec);

    // including the null terminator
    reply(wb, buf, strlen(buf) + 1);
    return 0;
}

int handle_r(tcp_buffer *wb, char *args, int len) {
    int cyl;
    int sec;
    char buf[512];

    // 解析参数
    if (sscanf(args, "%d %d", &cyl, &sec) != 2) {
        Log("Invalid command format for READ: %s", args);
        reply_with_no(wb, NULL, 0);
        return 0;
    }
    
    // 调用读取，回复消息
    if (cmd_r(cyl, sec, buf) == 0)
        reply_with_yes(wb, buf, 512);
    else
        reply_with_no(wb, "No", 3);
    return 0;
}

// 以十六进制格式输出扇区数据
int handle_rx(tcp_buffer *wb, char *args, int len) {
    int cyl;
    int sec;
    char buf[512];

    // 解析参数
    if (sscanf(args, "%d %d", &cyl, &sec) != 2) {
        Log("Invalid command format for READ: %s", args);
        reply_with_no(wb, NULL, 0);
        return 0;
    }

    // 调用读取，回复消息
    if (cmd_r(cyl, sec, buf) == 0) {
        // 构造回复消息：将字节数组转换为十六进制字符串
        char response[512 * 2 + 1];  // 加1留给null terminator
        char *ptr = response;

        for (int i = 0; i < 512; i++) {
            ptr += sprintf(ptr, "%02x", (unsigned char)buf[i]);
        }

        // 安全地返回字符串，长度为实际字符长度（1024）
        reply_with_yes(wb, response, strlen(response));
    } else {
        reply_with_no(wb, NULL, 0);
    }

    return 0;
}


int handle_w(tcp_buffer *wb, char *args, int len) {
    int cyl;
    int sec;
    int datalen;
    char *data;

    // 解析参数，检验合法性
    if(sscanf(args, "%d %d %d", &cyl, &sec, &datalen) != 3) {
        Log("Invalid command format for WRITE: %s", args);
        reply(wb, "No", 3);
        return 0;
    }
    if (datalen < 0 || datalen > BLOCKSIZE) {
        Log("Invalid data length: %d (must be 1-512)", datalen);
        reply_with_no(wb, NULL, 0);
        return 0;
    }
    data = args;
    for (int i = 0; i < 3; i++)
        data = strchr(data, ' ') + 1;

    // 调用写入，回复消息
    if (cmd_w(cyl, sec, datalen, data) == 0)
        reply(wb, "Yes", 4);
    else
        reply_with_no(wb, NULL, 0);
    return 0;
}

int handle_e(tcp_buffer *wb, char *args, int len) {
    const char *msg = "Bye!";
    reply(wb, msg, strlen(msg) + 1);
    return -1;
}

static struct {
    const char *name;
    int (*handler)(tcp_buffer *wb, char *, int);
} cmd_table[] = {
    {"I", handle_i},
    {"R", handle_r},
    {"X", handle_rx},
    {"W", handle_w},
    {"E", handle_e},
};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void on_connection(int id) {
    // some code that are executed when a new client is connected
    // you don't need this now
}

int on_recv(int id, tcp_buffer *wb, char *msg, int len) {
    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (p && strcmp(p, cmd_table[i].name) == 0) {
            ret = cmd_table[i].handler(wb, p + strlen(p) + 1, len - strlen(p) - 1);
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

void cleanup(int id) {
    // some code that are executed when a client is disconnected
    // you don't need this now
}

FILE *log_file;

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,
                "Usage: %s <disk file name> <cylinders> <sector per cylinder> "
                "<track-to-track delay> <port>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    // args
    char *filename = argv[1];
    int ncyl = atoi(argv[2]);
    int nsec = atoi(argv[3]);
    int ttd = atoi(argv[4]);  // ms
    int port = atoi(argv[5]);

    log_init("disk.log");

    int ret = init_disk(filename, ncyl, nsec, ttd);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize disk\n");
        exit(EXIT_FAILURE);
    }

    // command
    tcp_server server = server_init(port, 1, on_connection, on_recv, cleanup);
    server_run(server);

    // never reached
    close_disk();
    log_close();
}