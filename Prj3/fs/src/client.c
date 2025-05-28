#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]);
    tcp_client client = client_init("localhost", port);
    static char buf[4096];
    while (1) {
        // 获取工作路径并打印
        client_send(client, "p\n", 3);
        int n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        if (strlen(buf))
            printf("%s", buf);

        // 发送指令，获取回复
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        client_send(client, buf, strlen(buf) + 1);
        n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        printf("%s\n", buf);
        // 处理退出命令返回
        if (strcmp(buf, "Bye!") == 0 || strcmp(buf, "Logged out and directory deleted") == 0)
            break;
    }
    client_destroy(client);
}
