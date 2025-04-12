#include "bicycle.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

box_t box = {0, 0};

sem_t box_mutex; // 仓库的互斥锁
sem_t frame_producer_muxtex; // 车架制造者的互斥锁
sem_t wheel_producer_mutex; // 车轮制造者的互斥锁

sem_t empty_slot; // 仓库剩余容量

sem_t true_frame_lim; // 车架数量的理论上限
sem_t true_wheel_lim; // 车轮数量的理论上限

sem_t frame_available; // 仓库中可用车架
sem_t wheel_available; // 仓库中可用车轮

int bicycles_assembled = 0; // 组装好的自行车数量

int frame_got = 0; // 组装者是否拿了车架
int two_wheels_got = 0; // 组装者是否拿了车轮

int frame_produced = 0; // 生产的车架数量
int wheel_produced = 0; // 生产的车轮数量
const int frame_need = M; // 需要的车架数量
const int wheel_need = 2 * M; // 需要的车轮数量

int odd_wheel = 0; // 记录是否放置了了奇数个车轮，用于每放2个车轮释放1个信号量

void *frame_producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        // 制造车架
        sem_wait(&frame_producer_muxtex); // 制造的车架足够就不用制造了
        if (frame_produced >= frame_need) {
            sem_post(&frame_producer_muxtex);
            break;
        }
        else {
            frame_produced++;
            produce_frame(id);
            sem_post(&frame_producer_muxtex);
            // usleep(rand()%100000); // 实验用
        }

        // 存放车架
        sem_wait(&true_frame_lim);
        sem_wait(&empty_slot);
        sem_wait(&box_mutex);

        place_frame(id, &box); // 放入车架，+1操作在函数中执行
        sem_post(&frame_available); // 通知有车架

        sem_post(&box_mutex);
        // usleep(rand()%100000); // 实验用
    }
    pthread_exit(0);
}

void *wheel_producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        // 制造车轮
        sem_wait(&wheel_producer_mutex);
        if (wheel_produced >= wheel_need) { // 制造的车轮足够就不用制造了
            sem_post(&wheel_producer_mutex);
            break;
        }
        else {
            wheel_produced++;
            produce_wheel(id);
            sem_post(&wheel_producer_mutex);
            // usleep(rand()%100000); // 实验用
        }

        // 存放车轮
        sem_wait(&true_wheel_lim);
        sem_wait(&empty_slot);
        sem_wait(&box_mutex);

        place_wheel(id, &box); // 放入车轮，+1操作在函数中执行
        // 每制造两个车轮，通知一次
        if (odd_wheel == 1) {
            sem_post(&wheel_available);
            odd_wheel = 0;
        }
        else
            odd_wheel = 1;

        sem_post(&box_mutex);
        // usleep(rand()%100000); // 实验用
    }
    pthread_exit(0);
}

void *assembler(void *arg) {
    int id = *(int *)arg;
    while (1) {
        // 索要车架
        if(frame_got == 0) {
            if (sem_trywait(&frame_available) == 0) { // 使用非阻塞式等待
                sem_wait(&box_mutex);

                get_frame(id, &box); //拿走一个车架，-1操作在函数中进行
                sem_post(&empty_slot); // 释放一个仓库空间
                frame_got = 1; // 拿到了车架

                sem_post(&box_mutex);
                // usleep(rand()%100000); // 实验用
            }
        }
        // 索要车轮
        if (two_wheels_got == 0) {
            if (sem_trywait(&wheel_available) == 0) { // 使用非阻塞式等待
                sem_wait(&box_mutex);

                get_wheels(id, &box); //拿走一个车轮，-2操作在函数中进行
                sem_post(&empty_slot); // 以下两行释放了两个仓库空间
                sem_post(&empty_slot);
                two_wheels_got = 1; // 拿到了车轮

                sem_post(&box_mutex);
                // usleep(rand()%100000); // 实验用
            }
        }
        // 组装自行车
        if (frame_got && two_wheels_got) {
            // 我们把组装者的容量等效地纳入仓库的容量，故此时才释放对车架和车轮的理论数量限制
            sem_post(&true_frame_lim);
            sem_post(&true_wheel_lim);
            sem_post(&true_wheel_lim);
            frame_got = 0;
            two_wheels_got = 0;
            assemble(id); //组装自行车
            bicycles_assembled++;
            // usleep(rand()%100000); // 实验用
            if (bicycles_assembled == M)
                pthread_exit(NULL);
        }
    }
    pthread_exit(0);
}

void init() {
    // 组装者可以存放1个车架和2个车轮。我们把组装者的容量等效地纳入仓库的容量。
    sem_init(&true_frame_lim, 0, N - 1);  // 最多允许N-1个车架
    sem_init(&true_wheel_lim, 0, N + 1);  // 最多允许N+1个轮子

    sem_init(&box_mutex, 0, 1);
    sem_init(&empty_slot, 0, N);
    
    sem_init(&frame_available, 0, 0);
    sem_init(&wheel_available, 0, 0);

    sem_init(&wheel_producer_mutex, 0, 1);
    sem_init(&frame_producer_muxtex, 0, 1);

    // srand((unsigned)time(NULL));
}

void destroy() {
    sem_destroy(&empty_slot);
    sem_destroy(&frame_available);
    sem_destroy(&wheel_available);
    sem_destroy(&true_frame_lim);
    sem_destroy(&true_wheel_lim);
    sem_destroy(&box_mutex);
    sem_destroy(&wheel_producer_mutex);
    sem_destroy(&frame_producer_muxtex);
}
