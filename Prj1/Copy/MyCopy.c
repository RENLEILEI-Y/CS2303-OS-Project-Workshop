// 基础的文件复制程序，缓冲区大小为1000B
// ./MyCopy <source file> <destination file>
#include <stdio.h>
#include <stdlib.h>

// 清空缓存区，为了防止最后一次读入没有覆写整个缓存区导致错误输出
static inline int clear_buffer(char *buffer, const size_t buffer_size) {
    for(int i=0; i<buffer_size; i++)
        buffer[i] = '\0';
    return 1;
}

int copy(char *argv[]) {
    FILE *src, *dest;
    const size_t buffer_size = 1000;
    const size_t block_size = 100;
    const size_t block_count = 10;
    char* buffer = malloc(buffer_size * sizeof(char));

    src = fopen(argv[1],"r");
    dest = fopen(argv[2],"w");

    if(src == NULL) {
        fprintf(stderr, "Can't open file '%s'.\n",argv[1]);
        exit(-1);
    }
    if(dest == NULL) {
        fprintf(stderr, "Can't open file '%s'.\n",argv[2]);
        exit(-1);
    }

    while(clear_buffer(buffer, buffer_size)) {
        if(fread(buffer, block_size, block_count, src) == block_count) {    // 检测所有数据块是否完整读入
            if(fwrite(buffer, block_size, block_count, dest) != block_count) {      // 若完整读入则正常将缓冲区完整输出
                // 若输出的数据块数小于预期，报告复制输出错误
                fprintf(stderr, "Copy write error\n");
                exit(-1);
            }
        }
        else {  // 数据块没有全部完整读入
            if(feof(src)) {     // 若是因为到达文件末尾，则正常输出剩余的部分
                int i = 0;
                // 避免在文件末尾写入一堆空字符
                while(buffer[i] != '\0') {
                    putc(buffer[i], dest);
                    i++;
                }
                break;
            }
            else {      // 若未达到文件末尾，则报告复制输入错误
                fprintf(stderr, "Copy read error\n");
                exit(-1);
            }
        }

    }

    free(buffer);
    fclose(src);
    fclose(dest);
    
    return 0;
}

int main(int argc, char *argv[]) {
    // 检查参数数量是否正确
    if(argc != 3) {
        perror("argument error");
        exit(-1);
    }
    
    copy(argv);
    return 0;
}