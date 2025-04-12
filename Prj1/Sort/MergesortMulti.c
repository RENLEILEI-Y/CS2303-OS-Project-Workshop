// 归并排序的多线程实现
// ./MergesortMulti <MaxThreads>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

int max_depth;

typedef struct {
    int *arr;
    int len;
    int *sorted;
    int depth;
}thread_args;

void set_arguments(thread_args *args, int *arr, int len, int depth) {
    args->arr = arr;
    args->len = len;
    args->depth = depth;
}

int* merge(const int *arr1, const int *arr2 , const int l1, const int l2) {
    if(l1 == 0 && l2 == 0) return NULL;

    const int len = l1 + l2;
    int *sorted = (int *)malloc(sizeof(int) * (long unsigned int )len);
    if(!sorted) {
        perror("malloc failed in merge");
        exit(EXIT_FAILURE);
    }
    int i = 0, j = 0, k = 0;
    while(i < l1 && j < l2) {
        if(arr1[i] < arr2[j]) {
            sorted[k] = arr1[i];
            i++;
        }
        else {
            sorted[k] = arr2[j];
            j++;
        }
        k++;
    }
    if(i < l1)
        for(; i < l1; i++, k++)
            sorted[k] = arr1[i];
    else if(j < l2)
        for(; j < l2; j++, k++)
            sorted[k] = arr2[j];

    return sorted;
}

void* mergeSort(void *args) {
    thread_args *inp = (thread_args *)args;
    int *arr = inp->arr;
    int len = inp->len;
    int depth = inp->depth;
    inp->sorted = NULL;

    if(len == 1) {
        int *ret = (int *)malloc(sizeof(int)); // 确保返回的数组可被释放
        if(!ret) {
            perror("malloc failed in mergesort");
            exit(EXIT_FAILURE);
        }
        *ret = *arr;
        inp->sorted = ret;
        return NULL;
    }
    int mid = len / 2;

    // 线程参数
    pthread_t tid1, tid2;
    pthread_attr_t attr1, attr2;
    thread_args args1, args2;
    int rc; // 用来接收各种system call的返回值
    int t1_created = 0, t2_created = 0; // 标识符，指示是否创建新的thread

    // 线程1
    pthread_attr_init(&attr1);
    set_arguments(&args1, arr, mid, depth + 1);
    if(depth <= max_depth) {
        rc = pthread_create(&tid1, &attr1, mergeSort, &args1);
        if(rc) {
            fprintf(stderr, "create pthread failed: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
        t1_created = 1; 
    }
    if(!t1_created)
        mergeSort(&args1); // 若未创建子线程，则直接递归调用

    // 线程2
    pthread_attr_init(&attr2);
    set_arguments(&args2, arr + mid, len - mid, depth + 1);
    if(depth <= max_depth) {
        rc = pthread_create(&tid2, &attr2, mergeSort, &args2);
        if(rc) {
            fprintf(stderr, "create pthread failed: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
        t2_created = 1; 
    }
    if(!t2_created) // 若未创建子线程，则直接递归调用
        mergeSort(&args2);
    
    // 若创建了子线程，则等待子线程执行
    if(t1_created) {
        rc = pthread_join(tid1, NULL);
        if(rc) {
            perror("pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
    if(t2_created) {
        rc = pthread_join(tid2, NULL);
        if(rc) {
            perror("pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }

    // 对子线程的返回值进行归并
    inp->sorted = merge(args1.sorted, args2.sorted, mid, len - mid);
    // 释放内存
    if(args1.sorted) free(args1.sorted);
    if(args2.sorted) free(args2.sorted);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // 输入数据，设置参数
    if(argc != 2) {
        perror("argument error");
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    int *array = (int *)malloc(sizeof(int) * n);
    int *sorted;
    for(int i = atoi(argv[1]); i != 0; i/=2, max_depth++);

    // 读入数组
    scanf("%d", &n);
    for(int i = 0; i < n; i++)
        scanf("%d", array + i);

    // 进行归并排序
    // 线程变量
    thread_args args;
    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    set_arguments(&args, array, n, 1);
    
    // 创建线程
    if(max_depth > 0) {
        int rc = pthread_create(&tid, &attr, mergeSort, &args);
        if(rc) {
            fprintf(stderr, "create pthread failed: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
        // 等待线程执行
        rc = pthread_join(tid, NULL);
        if(rc) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
    else
        mergeSort(&args);

    // 输出排序结果
    sorted = args.sorted;
    for(int i = 0; i < n; i++)
    printf("%d ", sorted[i]);
    printf("\n");

    free(args.sorted);
    free(array);

    return 0;
}