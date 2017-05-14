//
// Created by maria on 5/5/17.
//

#include "add_utils.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main (int argc, char** argv) {
    //setting up the run
    long long shared_variable = 0;
    struct iter_data info;
    struct behavior* behavior = argparse(argc, argv);
    info.iters = behavior->iterations;
    info.var = &shared_variable;
    info.yield = behavior->yield;
    info.sync_mode = behavior->sync_mode;
    info.mutex = malloc(sizeof(pthread_mutex_t));

    info.spin_lock = malloc(sizeof(volatile int));
    pthread_mutex_init(info.mutex, NULL);

    //running and timing the threads
    struct timespec starttime, endtime;
    clock_gettime(CLOCK_MONOTONIC, &starttime);

    pthread_t* threads = manufacture_threads(behavior->n_threads, add, &info);
    join_threads(behavior->n_threads, threads, behavior);

    clock_gettime(CLOCK_MONOTONIC, &endtime);

    //ending the run, reporting statistics
    pthread_mutex_destroy(info.mutex);
    long run_time = (endtime.tv_sec*1000) - (starttime.tv_sec*1000);//abs_diff(endtime.tv_nsec, starttime.tv_nsec);
    run_time*=1000000000;
    run_time += endtime.tv_nsec;
    run_time -= starttime.tv_nsec;
    unsigned opers = 2 * behavior->n_threads * behavior->iterations;
    long avg_runtime = run_time / opers;
    fprintf(stdout, ",%u,%u,%u,%lu,%li,%lli\n", behavior->n_threads, behavior->iterations, opers, run_time, avg_runtime, shared_variable);
}