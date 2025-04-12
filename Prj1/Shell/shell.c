// 实现了服务器功能的shell
// ./shell <port>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int BACKLOG = 10; // 请求队列长度
const int MAX_LINE = 10; // 一行指令的最大参数量
const int  ARGU_SIZE = 10; // 每个参数的最大长度
const int BUFFER_SIZE = 100; // 路径存储器的长度
const int MAX_SOCKET_SIZE = 1000; // socket传输数据的长度

void mysend(const int client_fd, const char* msg);
void myrecv(const int client_fd, char *line);
int execute_normal(char *line, const int client_fd); // 处理没有管道的命令

int execute(char *line, const int client_fd) {
    int pipes[MAX_LINE-1][2]; // MAX_LINE-1个管道
    char *command_array[MAX_LINE * ARGU_SIZE + ARGU_SIZE]; // 储存用'|'分割的命令
    char *arguments[MAX_LINE][ARGU_SIZE]; // 储存在command_array的基础上分割成的单个参数
    int counter = 0;
    char *command, *argument;
    int i, j;
    pid_t pid;

    if(strchr(line, '|') == NULL) {
        execute_normal(line, client_fd);
        counter = 1;
    }
    
    // 复制输入行，否则strtok()会改变原字符串
    else {
        char *new_line = strdup(line);

        // 解析按管道符号分割的命令
        command = strtok(new_line, "|");
        while(command && counter < MAX_LINE) {
            command_array[counter++] = command;
            command = strtok(NULL, "|");
        }
        // 解析每个命令的参数
        for(i = 0; i < counter; i++) {
            j = 0;
            argument = strtok(command_array[i], " ");
            while(argument != NULL && j < ARGU_SIZE - 1) {
                arguments[i][j++] = argument;
                argument= strtok(NULL, " ");
            }
            arguments[i][j] = NULL; // 将每段命令的末尾参数设置为NULL
        }
        // 创建管道与进程
        for(i = 0; i < counter - 1; i++) {
            if(pipe(pipes[i]) < 0) {
                perror("pipe failed");
                free(new_line);
                return -1;
            }
        }
        for(i = 0; i < counter; i++) {
            pid = fork();
            if(pid < 0) {
                perror("fork failed");
                free(new_line);
                return -1;
            }
            // 子进程
            else if(pid == 0) {
                if(i > 0)
                    if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                        perror("dup2 failed");
                        exit(EXIT_FAILURE);
                    }
                if(i < counter - 1)
                    if(dup2(pipes[i][1], STDOUT_FILENO) < 0 || dup2(pipes[i][1], STDERR_FILENO) < 0) {
                        perror("dup2 failed");
                        exit(EXIT_FAILURE);
                    }
                if(i == counter -1)
                    if(dup2(client_fd, STDOUT_FILENO) < 1 || dup2(client_fd, STDOUT_FILENO) < 0) {
                        perror("dup2 failed");
                        exit(EXIT_FAILURE);
                    }
                for(j = 0; j < counter - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                // 处理命令
                if(execvp(arguments[i][0], arguments[i]) < 0) {
                    fprintf(stderr, "Error: running command: '%s'\n", arguments[i][0]);
                    exit(EXIT_FAILURE);
                }
            }
        }
        // 父进程
        for (i=0; i<counter-1; i++) { // 关闭所有管道
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        for (i=0; i <counter; i++) // 等待所有子进程
            wait(NULL);
    }
    return counter;
}

int execute_normal(char *line, const int client_fd) {
    int counter = 0;
    char *newline = (char *)malloc(MAX_SOCKET_SIZE);
    char *command_array[ARGU_SIZE];
    pid_t pid;

    strcpy(newline, line);
    char *p = strtok(newline, " ");
    while(p) {
        command_array[counter] = p;
        counter++;
        p = strtok(NULL, " ");
    }
    command_array[counter] = NULL;

    // 单独处理cd
    if(strcmp(command_array[0], "cd") == 0) {
        if (chdir(command_array[1]) != 0) {
            if(dup2(client_fd, STDERR_FILENO) < 0)
                perror("dup failed");
            fprintf(stderr, "no such file '%s' in dictionary\n", command_array[1]);
        }
        return 0;
    }

    pid = fork();
    if(pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0) {
        if(dup2(client_fd, STDOUT_FILENO) < 0 || dup2(client_fd, STDERR_FILENO) < 0)
            perror("dup failed");

    // 处理其它命令
    if(execvp(command_array[0], command_array) == -1) {
            fprintf(stderr, "error: running command: '%s'\n", line);
            exit(EXIT_FAILURE);
        }
    }
    else
        wait(NULL);
    return 0;
}

void mysend(const int client_fd, const char* msg) {
    int sendbytes;
    if ((sendbytes = send(client_fd, msg, strlen(msg), 0)) == -1) {
        perror("server send error");
        exit(EXIT_FAILURE);
    }
}

void myrecv(const int client_fd, char *line) {
    int recvbytes;
    if ((recvbytes = recv(client_fd, line, MAX_SOCKET_SIZE, 0)) == -1) {
        perror("server receive error");
        exit(EXIT_FAILURE);
    }
}

void clear(char * line) {
    for(int i=0; i<MAX_SOCKET_SIZE; i++)
        line[i] = '\0';
}

// 用于手动关闭服务器
void abort_(int signal) {
    if (signal == SIGINT) {
        printf("\nserver shut down\n");
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    // 检查输入参数
    if(argc != 2) {
        perror("argument error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_sockaddr, client_sockaddr;
    int sin_size;
    int sockfd, client_fd;
    int client_id = 0;
    const int server_port = atoi(argv[1]);

    // 创建服务器端的socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    printf("socket success!,sockfd=%d\n", sockfd);
    // 绑定socket
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(server_port);
    server_sockaddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_sockaddr.sin_zero), 8);
    if((bind(sockfd, (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr))) == -1) {
        perror("bind failed");
        exit(-1);
    }
    printf("bind success!\n");

    while(1) {
        // 监听
        if(listen(sockfd,BACKLOG) == -1) {
            perror("listen");
            exit(1);
        }
        printf("listening ... \n");

        signal(SIGINT, abort_); // 手动关闭服务器

        sin_size = sizeof(struct sockaddr_in);
        if((client_fd = accept(sockfd, (struct sockaddr *)&client_sockaddr, (socklen_t *)&sin_size)) == -1) {
            perror("accept failed");
            close(client_fd);
        }
        printf("accept success!\n");
        client_id++;

        pid_t pid = fork();
        if(pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        // 子进程
        else if(pid == 0) {
            close(sockfd); //子进程不需要监听socket
            char line[MAX_SOCKET_SIZE];
            char locbuf[BUFFER_SIZE];

            mysend(client_fd, "Welcome to my shell!(type 'exit' to quit)\n");
            while(1) {
                // 获取并输出所在路径
                if (getcwd(locbuf, BUFFER_SIZE) == NULL) {
                    perror("getcwd failed");
                    continue;
                }
                strcat(locbuf, " >>");
                mysend(client_fd, locbuf);

                // 从client端获取指令
                clear(line);
                myrecv(client_fd, line);
                printf("client %d command: %s", client_id, line);

                // 解析并执行指令
                char *p = strchr(line, '\r');
                *p = '\0'; // 去掉换行符

                // client退出
                if(strcmp(line, "exit") == 0) {
                    mysend(client_fd, "Have a niec day!\n");
                    printf("client %d leaving\n", client_id);
                    break;
                }

                if (strlen(line) == 0)
                    continue;
                execute(line, client_fd);
            }
            close(client_fd);
            exit(EXIT_SUCCESS);
        }
        else
            close(client_fd); // 父进程不需要子进程的client
    }
    return 0;
}