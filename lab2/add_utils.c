//
// Created by maria on 5/5/17.
//

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "add_utils.h"
#include <string.h>
#include <getopt.h>
#include <sched.h>
#include <stdatomic.h>

void usage(){
    fprintf(stderr, "--yield=YIELD\tYield execution during addition and subtraction\n");
    fprintf(stderr, "--iterations=ITERS\tIterate ITERS times\n");
    fprintf(stderr, "--sync=SYNCOPT\tUse the synchronisation option specified\n");
    fprintf(stderr, "\t'm'\tUse pthread_mutex\n");
    fprintf(stderr, "\t's'\tUse test-and-set spinlocks\n");
    fprintf(stderr, "\t'c'\tUse Compare-and-swap to add\n");
    fprintf(stderr, "--threads=THREADS\tUse THREADS threads\n");
    exit(EXIT_FAILURE); //todo
}
struct behavior* argparse(int argc, char** argv){
    struct behavior* ret = malloc(sizeof(struct behavior));
    ret->yield = false;
    ret->sync_mode = NONE;
    int getopt_status, index;
    ret->iterations = 1;
    ret->n_threads = 1;
    struct option options[] = {
            {"threads", required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"yield", no_argument, NULL, 'y'},
            {"sync", required_argument, NULL, 's'},
            {0, 0, 0, 0},
    };
    while ((getopt_status = getopt_long(argc, argv, ":t:i:y:s", options, &index)) != -1){
        switch (getopt_status) {
            case -1:
            case 0:
                break;
            case 't':
                if (sscanf(optarg, "%i", &(ret->n_threads))!=1 || (int) ret->n_threads < 1){
                    fprintf(stderr, "Invalid number of threads, %s\n", optarg);
                    usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if (sscanf(optarg, "%i", &(ret->iterations))!=1 || (int) ret->iterations < 1){
                    fprintf(stderr, "Invalid number of iterations, %s\n", optarg);
                    usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case 'y':
                ret->yield = true;
                break;
            case 's':
                if (optarg[0] == 'm'){
                    ret->sync_mode = MUTEX;
                    break;
                }
                if (optarg[0] == 's'){
                    ret->sync_mode = SPINLOCK;
                    break;
                }
                if (optarg[0] == 'c'){
                    ret->sync_mode = COMPARE_SWAP;
                    break;
                }
                //if we're here, someone fucked up
                fprintf(stderr, "Unknown synchronization system: %s\n", optarg);
                exit(EXIT_FAILURE);
            case '?':
                fprintf(stderr, "Unrecognized option: %s\n", argv[index+1]);
                usage();
                exit(EXIT_FAILURE);
            case ':':
                fprintf(stderr, "Unrecognized argument to option %s: %s\n", argv[index+1], optarg);
                usage();
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Very wrong arguments and options given (getopt status code: %x", getopt_status);
                usage();
                exit(EXIT_FAILURE);
        }
    }
    return ret;
}
void* add(void *v_info){
    const struct iter_data *info =  (struct iter_data*) v_info;
    //const so the compiler won't flush it from cache (it's used a lot and never changes anyway)
    int i = 0;
    long long sum, prev_sum;
    for (; i<info->iters; ++i){
        switch (info->sync_mode){
            case MUTEX:
                pthread_mutex_lock(info->mutex);
                break;
            case SPINLOCK:
                while (__sync_lock_test_and_set(info->spin_lock, 1) != 0){
                    ;
                }
                break;
            case COMPARE_SWAP:
                do {
                    prev_sum = *(info->var);
                    sum = *(info->var) + 1;
                } while (__sync_val_compare_and_swap(info->var, prev_sum, sum) != prev_sum);
                break;
            case NONE:
                break;
        }
        if (info->sync_mode != COMPARE_SWAP) {
            sum = *(info->var) + 1;
            if (info->yield) {
                sched_yield();
            }
            *(info->var) = sum;
        }
        switch (info->sync_mode){
            case MUTEX:
                pthread_mutex_unlock(info->mutex);
                break;
            case SPINLOCK:
                if (__sync_lock_test_and_set(info->spin_lock, 0) != 1){
                    fprintf(stderr, "builtin test-and-set bugged out\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case COMPARE_SWAP:
                break;
            case NONE:
                break;
        }
    }
    for (; i>0; --i){
        switch (info->sync_mode){
            case MUTEX:
                pthread_mutex_lock(info->mutex);
                break;
            case SPINLOCK:
                while (__sync_lock_test_and_set(info->spin_lock, 1) != 0){
                    ;
                }
                break;
            case COMPARE_SWAP:
                do {
                    prev_sum = *(info->var);
                    sum = *(info->var) - 1;
                } while (__sync_val_compare_and_swap(info->var, prev_sum, sum) != prev_sum);
                break;
            case NONE:
                break;
        }
        if (info->sync_mode != COMPARE_SWAP) {
            sum = *(info->var) - 1;
            if (info->yield) {
                sched_yield();
            }
            *(info->var) = sum;
        }
        switch (info->sync_mode){
            case MUTEX:
                pthread_mutex_unlock(info->mutex);
                break;
            case SPINLOCK:
                if (__sync_lock_test_and_set(info->spin_lock, 0) != 1){
                    fprintf(stderr, "builtin test-and-set bugged out\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case COMPARE_SWAP:
                break;
            case NONE:
                break;

        }
    }
    return NULL; //so pthread will quit complaining about mismatched pointers
}
pthread_t* manufacture_threads(unsigned n_threads, void* (*func)(void *), void* func_arg){
    if (n_threads < 1){
        fprintf(stderr, "Number of threads, %u, is not valid\n", n_threads);
        exit(EXIT_FAILURE);
    }
    pthread_t* threads = malloc(n_threads* sizeof(pthread_t));
    for (int i=0; i<n_threads; ++i){
        if (pthread_create(threads+i, NULL, func, func_arg)){
            fprintf(stderr, "Creating thread failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return threads;
}

void join_threads(unsigned n_threads, pthread_t* threads, struct behavior* behavior){
    if (n_threads < 1){
        fprintf(stderr, "Number of threads, %u, is not valid\n", n_threads);
        exit(EXIT_FAILURE);
    }
    for (int i=0; i<n_threads; ++i){
        if (pthread_join((threads[i]), NULL)) {
            fprintf(stderr, "Unable to join threads: %s\n", strerror(errno));
            exit((EXIT_FAILURE));
        }
    }
    fprintf(stdout, "add");
    if (behavior->yield){
        fprintf(stdout, "-yield");
    }
    switch (behavior->sync_mode){
        case SPINLOCK:
            fprintf(stdout, "-s");
            break;
        case MUTEX:
            fprintf(stdout, "-m");
            break;
        case COMPARE_SWAP:
            fprintf(stdout, "-c");
            break;
        case NONE:
            fprintf(stdout, "-none");
            break;
    }
}

