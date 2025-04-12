#ifndef __PARTY__H__
#define __PARTY__H__

static const int A = 1;  // number of frame producers
static const int B = 1;  // number of wheel producers
static const int C = 1;  // number of assemblers
static const int N = 3; // number of slots in the box
static const int M = 5;  // number of bicycles to be assembled

// Don't modify the following lines.

typedef struct {
    int n_frames;
    int n_wheels;
} box_t;

// thread functions
void *frame_producer(void *arg);
void *wheel_producer(void *arg);
void *assembler(void *arg);

// functions called in main
void init();
void destroy();

// functions called in threads
void produce_frame(int id);
void place_frame(int id, box_t *box);

void produce_wheel(int id);
void place_wheel(int id, box_t *box);

void get_frame(int id, box_t *box);
void get_wheels(int id, box_t *box);
void assemble(int id);

#endif
