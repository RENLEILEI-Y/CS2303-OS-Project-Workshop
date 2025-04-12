// 使用管道通信的文件复制，缓冲区大小为1000B
// ./PipeCopy <source file> <destination file>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

// 清空缓存区，为了防止最后一次读入没有覆写整个缓存区导致错误输出
static inline int clear_buffer(char *buffer, const size_t buffer_size) {
    for(int i=0; i<buffer_size; i++)
        buffer[i] = '\0';
    return 1;
}

int pipecopy(char *argv[]) {
    int mypipe[2];
    __pid_t pid1, pid2;
    size_t buffer_size = 1000;
    size_t block_size = 100;
    size_t block_count = 10;
    char *buffer = malloc(buffer_size * sizeof(char));

    if(pipe(mypipe)) {
        fprintf(stderr,"Pipe failed\n");
        exit(-1);
    }

    pid1 = fork();
    if(pid1 < 0) {
        fprintf(stderr,"Fork failed\n");
        exit(-1);
    }
    else if(pid1 == 0){
        close(mypipe[0]);
        FILE *src;
        src = fopen(argv[1],"r");
        if(src == NULL) {
            fprintf(stderr, "Can't open file '%s'.\n", argv[1]);
            exit(-1);
        }
        while(clear_buffer(buffer, buffer_size)) {
            if(fread(buffer, block_size, block_count, src) == block_count) {
                if(write(mypipe[1], buffer, buffer_size) != buffer_size) {
                    fprintf(stderr, "Pipe error\n");
                    exit(-1);
                }
            }
            else {
                if(feof(src)) {
                    size_t i;
                    for(i=0; buffer[i]!='\0'; i++);
                    if(write(mypipe[1], buffer, i) != i) {
                        fprintf(stderr, "Pipe error\n");
                        exit(-1);
                    }
                    break;
                }
                else {
                    fprintf(stderr, "Copy read error\n");
                    exit(-1);
                }
            }
            
        }
        fclose(src);
        close(mypipe[1]);
        exit(EXIT_SUCCESS);
        printf("Copy completed successfully!\n");
    }

    pid2 = fork();
    if(pid2 < 0) {
        fprintf(stderr, "Fork failed");
        exit(-1);
    }
    else if(pid2 == 0){
        close(mypipe[1]);
        FILE *dest;
        dest = fopen(argv[2],"w");
        if(dest == NULL) {
            fprintf(stderr, "Can't open file '%s'.\n", argv[1]);
            exit(-1);
        }
        while(1) {
            size_t count = read(mypipe[0], buffer, buffer_size);
            if(count == buffer_size) {
                if(fwrite(buffer, block_size, block_count, dest) != block_count)
                    fprintf(stderr, "Copy write error\n");
            }
            else {
                for(size_t i=0; i<count; i++) putc(buffer[i], dest);
                break;
            }
        }
        fclose(dest);
        close(mypipe[0]);
        exit(EXIT_SUCCESS);
    }

    close(mypipe[0]);
    close(mypipe[1]);
    wait(NULL);
    wait(NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    // 检查参数数量是否正确
    if(argc != 3) {
        perror("argument error");
        exit(-1);
    }
    pipecopy(argv);
}