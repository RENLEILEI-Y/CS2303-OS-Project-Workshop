#include "lcm.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// semaphores
sem_t shovel;
sem_t holes_ahead;  // Larry领先Curly的洞
sem_t holes_unplanted; // 没有放种子的洞
sem_t holes_planted; // 放了种子的洞

void *larry() {
    // some code goes here
    int id = 0;
    while (id < N) {
        sem_wait(&holes_ahead); // 若领先的洞达到了阈值，则等待
        sem_wait(&shovel);

        get_shovel(LARRY);
        if (id >= N) {
            sem_post(&holes_ahead);
            sem_post(&shovel);
            break;
        }
        dig(LARRY, ++id);
        drop_shovel(LARRY);

        sem_post(&holes_unplanted); // 增加一个没有放种子的洞
        sem_post(&shovel);

        // usleep(rand()%100000); // 实验用
    }
    pthread_exit(0);
}

void *moe() {
    // some code goes here
    int id = 0;
    while (id < N) {
        sem_wait(&holes_unplanted); // 等待一个没放种子的洞
        if (id >= N) {
            sem_post(&holes_unplanted);
            break;
        }
        plant(MOE, ++id);
        sem_post(&holes_planted); // 增加一个放了种子的洞

        // usleep(rand()%100000); // 实验用
    }
    pthread_exit(0);
}

void *curly() {
    // some code goes here
    int id = 0;
    while (id < N) {
        sem_wait(&holes_planted); // 等待一个放了种子的洞
        sem_wait(&shovel);

        get_shovel(CURLY);
        if (id >= N) {
            sem_post(&holes_planted);
            sem_post(&shovel);
            break;
        }
        fill(CURLY, ++id);
        drop_shovel(CURLY);

        sem_post(&holes_ahead); // 追上一个洞
        sem_post(&shovel);
        
        // usleep(rand()%100000); // 实验用
    }
    pthread_exit(0);
}

void init() {
    // some code goes here
    sem_init(&shovel, 0, 1);
    sem_init(&holes_ahead, 0, MAX);
    sem_init(&holes_planted, 0, 0);
    sem_init(&holes_planted, 0, 0);
    srand((unsigned)time(NULL));
}

void destroy() {
    // some code goes here
    sem_destroy(&shovel);
    sem_destroy(&holes_ahead);
    sem_destroy(&holes_planted);
    sem_destroy(&holes_unplanted);
}
