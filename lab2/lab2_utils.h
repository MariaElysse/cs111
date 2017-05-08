//
// Created by maria on 5/5/17.
//

#ifndef LAB2_LAB2_UTILS_H
#define LAB2_LAB2_UTILS_H
#include <pthread.h>
#include <stdbool.h>

#define print_err() do {if (errno) {fprintf(stderr, "Internal Error: %s", strerror(errno)); exit(EXIT_FAILURE)}}
#define abs_diff(x, y) x < y ? y-x : x-y


enum SYNC_MODE {NONE, MUTEX, SPINLOCK, COMPARE_SWAP};
struct behavior {
    unsigned n_threads;
    unsigned iterations;
    enum SYNC_MODE sync_mode;
    bool yield;
};
struct iter_data {
    pthread_mutex_t *mutex;
    unsigned iters;
    long long* var;
    enum SYNC_MODE sync_mode;
    bool yield;
    volatile int *spin_lock;
};
//enum
struct behavior* argparse(int argc, char** argv);
pthread_t* manufacture_threads(unsigned n_threads, void* (*func)(void *), void* func_arg);
void join_threads(unsigned n_threads, pthread_t* threads, struct behavior* behavior);
void* add(void *info);
#endif //LAB2_LAB2_UTILS_H
