// Don't modify this file.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lcm.h"

int main() {
    init();
    printf("N = %d.\nMAX = %d.\n", N, MAX);

    pthread_t t[3];
    pthread_create(&t[0], NULL, larry, NULL);
    pthread_create(&t[1], NULL, moe, NULL);
    pthread_create(&t[2], NULL, curly, NULL);

    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_join(t[2], NULL);

    destroy();
    printf("End.\n");
}

char *get_name(Farmer f) {
    switch (f) {
        case LARRY:
            return "Larry";
        case MOE:
            return "Moe";
        case CURLY:
            return "Curly";
    }
    return "Unknown";
}

void get_shovel(Farmer f) { printf("%s gets the shovel.\n", get_name(f)); }

void drop_shovel(Farmer f) { printf("%s drops the shovel.\n", get_name(f)); }

void dig(Farmer f, int id) {
    printf("%s digs another hole #%d.\n", get_name(f), id);
}

void fill(Farmer f, int id) {
    printf("%s fills a planted hole #%d.\n", get_name(f), id);
}

void plant(Farmer f, int id) {
    printf("%s plants a seed in hole #%d.\n", get_name(f), id);
}
