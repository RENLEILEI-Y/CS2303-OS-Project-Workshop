// 归并排序的基础
// ./MergesortSingle
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int* merge(const int *arr1, const int *arr2 , const int l1, const int l2) {
    const int len = l1 + l2;
    int *sorted = (int *)malloc(sizeof(int) * (long unsigned int )len);
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

int* mergeSort(const int * arr, const int len) {
    if(len == 1) {
        int *ret = (int *)malloc(sizeof(int)); // 确保返回的数组可被释放
        if(!ret) {
            perror("malloc failed in mergesort");
            exit(EXIT_FAILURE);
        }
        *ret = *arr;
        return ret;
    }

    int mid = len / 2;
    int *arr1 = mergeSort(arr, mid);
    int *arr2 = mergeSort(arr + mid, len-mid);
    
    int *sorted = merge(arr1, arr2, mid, len - mid);
    free(arr1);
    free(arr2);
    return sorted;
}

int main() {
    int n;
    scanf("%d", &n);
    int *array = (int *)malloc(sizeof(int) * n);
    int *sorted;

    // 读入数组
    for(int i = 0; i < n; i++)
        scanf("%d", array + i);
    
    // 执行递归排序
    sorted = mergeSort(array, n);

    // 输出排序结果
    for(int i = 0; i < n; i++)
        printf("%d ", sorted[i]);
    printf("\n");

    // 释放内存
    free(array);
    free(sorted);

    return 0;
}
// 数组长度