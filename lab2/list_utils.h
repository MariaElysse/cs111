//
// Created by maria on 5/8/17.
//

#ifndef LAB2_LIST_UTILS_H
#define LAB2_LIST_UTILS_H
#include "SortedList.h"
#define print_err() do {if (errno) {fprintf(stderr, "Internal Error: %s", strerror(errno)); exit(EXIT_FAILURE);}} while (0)
enum SYNC_MODE { NONE, SPINLOCK, MUTEX};
struct behavior {
    int opt_yield;
    char yieldopts[4];
    enum SYNC_MODE sync_mode;
    unsigned iterations;
    unsigned n_threads;
    pthread_mutex_t* mutex;
    volatile int* spin_lock;
    SortedList_t* list;
    //SortedListElement_t* elelist;
    struct timespec start, end;
    char** key_list;
};
extern int opt_yield;
#define KEY_LEN 8
#define EXIT_LIST_FAILURE 2
void usage();
struct behavior* argparse(int argc, char** argv);
void fill_list(const struct behavior* behavior);
void empty_list(const struct behavior* behavior);
char** manufacture_list(struct behavior* behavior);
void join_threads(pthread_t* threads, struct behavior* behavior);
//void init_randlist(struct behavior* behavior);
pthread_t* manufacture_threads(unsigned n_threads, void* (*func)(void *), void* func_arg);
void* list(void* v_behavior);
#endif //LAB2_LIST_UTILS_H
