// Don't modify this file, except for usleep() calls.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bicycle.h"

int main() {
    printf("N = %d, M = %d\n", N, M);
    printf("A = %d, B = %d, C = %d\n", A, B, C);
    init();

    pthread_t a_t[A], b_t[B], c_t[C];
    int a_id[A], b_id[B], c_id[C];

    for (int i = 0; i < A; i++) {
        a_id[i] = i;
        pthread_create(&a_t[i], NULL, frame_producer, &a_id[i]);
    }
    for (int i = 0; i < B; i++) {
        b_id[i] = i;
        pthread_create(&b_t[i], NULL, wheel_producer, &b_id[i]);
    }
    for (int i = 0; i < C; i++) {
        c_id[i] = i;
        pthread_create(&c_t[i], NULL, assembler, &c_id[i]);
    }

    for (int i = 0; i < A; i++) {
        pthread_join(a_t[i], NULL);
    }
    for (int i = 0; i < B; i++) {
        pthread_join(b_t[i], NULL);
    }
    for (int i = 0; i < C; i++) {
        pthread_join(c_t[i], NULL);
    }

    destroy();
    printf("End.\n");
}

void produce_frame(int id) {
    usleep(rand() % 1000000 + 500000);
    printf("Frame producer %d produced a frame.\n", id);
}

void place_frame(int id, box_t *box) {
    if (box->n_frames < N) {
        box->n_frames++;
        printf("Frame producer %d placed a frame. { %d frames, %d wheels }\n", id, box->n_frames, box->n_wheels);
    } else {
        printf("Box is full. Frame producer %d cannot place a frame.\n", id);
    }
}

void produce_wheel(int id) {
    usleep(rand() % 1000000 + 500000);
    printf("Wheel producer %d produced a wheel.\n", id);
}

void place_wheel(int id, box_t *box) {
    if (box->n_wheels < N) {
        box->n_wheels++;
        printf("Wheel producer %d placed a wheel. { %d frames, %d wheels }\n", id, box->n_frames, box->n_wheels);
    } else {
        printf("Box is full. Wheel producer %d cannot place a wheel.\n", id);
    }
}

void get_frame(int id, box_t *box) {
    if (box->n_frames > 0) {
        box->n_frames--;
        printf("Assembler %d took a frame. { %d frames, %d wheels }\n", id, box->n_frames, box->n_wheels);
    } else {
        printf("No frames. Assembler %d cannot take a frame.\n", id);
    }
}

void get_wheels(int id, box_t *box) {
    if (box->n_wheels > 1) {
        box->n_wheels -= 2;
        printf("Assembler %d took two wheels. { %d frames, %d wheels }\n", id, box->n_frames, box->n_wheels);
    } else {
        printf("No enough wheels. Assembler %d cannot take wheels.\n", id);
    }
}

void assemble(int id) {
    usleep(rand() % 1000000 + 500000);
    printf("Assembler %d assembled a bicycle.\n", id);
}