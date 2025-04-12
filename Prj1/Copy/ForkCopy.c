// 使用子进程的文件复制，缓冲区大小为1000B
// ./ForkCopy <source file> <destination file>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    __pid_t pid;

    // 检查参数数量是否正确
    if(argc != 3) {
        perror("argument error");
        exit(-1);
    }
    
    pid = fork();
    if(pid == -1) {
        perror("Fork failed\n");
        exit(-1);
    }
    if(pid == 0) {
        fflush(stdout);
        if(execl("./MyCopy", argv[0], argv[1], argv[2], NULL) == -1) {
            perror("Exec failed\n");
            exit(-1);
        }
    }
    else {
        wait(NULL);
        fflush(stdout);
    }
    return 0;
}