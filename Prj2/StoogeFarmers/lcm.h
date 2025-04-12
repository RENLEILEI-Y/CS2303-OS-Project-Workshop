#ifndef __LCM__H__
#define __LCM__H__

static const int N = 5; // number of holes to dig
static const int MAX = 3; // maximum number of unfilled holes

// Don't modify the following lines.

typedef enum { LARRY = 0, MOE = 1, CURLY = 2 } Farmer;
char *get_name(Farmer f);

// thread functions
void *larry();
void *moe();
void *curly();

// functions called in main
void init();
void destroy();

// functions called in threads
void get_shovel(Farmer f);
void drop_shovel(Farmer f);
void dig(Farmer f, int id);
void fill(Farmer f, int id);
void plant(Farmer f, int id);

#endif
