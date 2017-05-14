//
// Created by maria on 5/8/17.
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "list_utils.h"
//extern char* g_list;
const char* KEY =  "neutron"; //no requirement for randomness
//https://piazza.com/class/j119yaxomjl2qb?cid=318
int opt_yield=0;
void handler(int sig){
    fprintf(stderr, "ERROR: %i caught\n", sig);
    if (sig==SIGSEGV){
        exit(EXIT_LIST_FAILURE);
    }
}
void usage(){
    fprintf(stderr, "--yield=YIELD\tYield execution at one or more of the times specified\n");
    fprintf(stderr, "\t'i'\tduring inserts\n");
    fprintf(stderr, "\t'd'\tduring deletes\n");
    fprintf(stderr, "\t'l'\tduring lookups\n");
    fprintf(stderr, "--iterations=ITERS\tIterate ITERS times\n");
    fprintf(stderr, "--sync=SYNCOPT\tUse the synchronisation option specified\n");
    fprintf(stderr, "\t'm'\tUse pthread_mutex\n");
    fprintf(stderr, "\t's'Use test-and-set spinlocks\n");
    fprintf(stderr, "--threads=THREADS\tUse THREADS threads\n");
    exit(EXIT_FAILURE); //todo
}
void fill_list(const struct behavior* behavior){

    for (int i=0; i<behavior->iterations; ++i){
        SortedListElement_t *temp = malloc(sizeof(SortedListElement_t));
        temp->key = behavior->key_list[i];
        //got lock
        switch (behavior->sync_mode){
            case MUTEX:
                pthread_mutex_lock(behavior->mutex);
                break;
            case SPINLOCK:
                while (__sync_lock_test_and_set(behavior->spin_lock, 1) != 0){
                    ;
                }
                break;
            case NONE:
                break;
        }
        SortedList_insert(behavior->list, temp);
        //release lock
        switch (behavior->sync_mode){
            case MUTEX:
                pthread_mutex_unlock(behavior->mutex);
                break;
            case SPINLOCK:
                if (__sync_lock_test_and_set(behavior->spin_lock, 0) != 1){
                    fprintf(stderr, "builtin test-and-set bugged out\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case NONE:
                break;
        }
    }

}
void empty_list(const struct behavior* behavior){
    switch (behavior->sync_mode){
        case MUTEX:
            pthread_mutex_lock(behavior->mutex);
            break;
        case SPINLOCK:
            while (__sync_lock_test_and_set(behavior->spin_lock, 1) != 0){
                ;
            }
            break;
        case NONE:
            break;
    }
    //got a lock
    for (int i =0; i< behavior->iterations; ++i){
        //fetch one then delete it
        SortedListElement_t* temp = SortedList_lookup(behavior->list, behavior->key_list[i]);
        if (memcmp(behavior->key_list[i], temp->key, KEY_LEN) != 0){
            exit(EXIT_LIST_FAILURE);
        }
        SortedList_delete(temp);
    }
    //release lock
    switch (behavior->sync_mode){
        case MUTEX:
            pthread_mutex_unlock(behavior->mutex);
            break;
        case SPINLOCK:
            if (__sync_lock_test_and_set(behavior->spin_lock, 0) != 1){
                fprintf(stderr, "builtin test-and-set bugged out\n");
                exit(EXIT_FAILURE);
            }
            break;
        case NONE:
            break;
    }
}
void verify_list(const struct behavior* behavior){
    for(int i=0; i<behavior->iterations; ++i){
        SortedListElement_t* temp = SortedList_lookup(behavior->list, behavior->key_list[i]);
        if (!temp){
            exit(EXIT_LIST_FAILURE);
        }
        if (memcmp(behavior->key_list[i], temp->key, KEY_LEN) != 0){ //i did not know I could do this
            exit(EXIT_LIST_FAILURE);
        }
    }
}

char** manufacture_list(struct behavior* behavior){
    char** array = calloc(behavior->iterations, sizeof(char*));
    print_err();
    for (int i=0; i<behavior->iterations; ++i){
        for (int j=0; j<KEY_LEN; ++j){
            array[i] = calloc(KEY_LEN, sizeof(char));
            array[i][j] = (char) rand();
        }
    }
    return array;
}

struct behavior* argparse(int argc, char** argv) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handler;
    sigaction(SIGSEGV, &sa, NULL);
    print_err();
    //signal(SIGSEGV, handler);
    struct behavior *ret = calloc(sizeof(struct behavior), 1);
    ret->sync_mode = NONE;
    int getopt_status, index;
    ret->iterations = 1;
    ret->n_threads = 1;
    ret->list = calloc(sizeof(SortedList_t), 1);
    ret->spin_lock = malloc(sizeof(volatile int));
    ret->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ret->mutex, NULL);
    ret->opt_yield = 0;
    struct option options[] = {
            {"threads",    required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"yield",      required_argument, NULL, 'y'},
            {"sync",       required_argument, NULL, 's'},
            {0, 0, 0,                               0},
    };
    while ((getopt_status = getopt_long(argc, argv, ":t:i:y:s", options, &index)) != -1) {
        switch (getopt_status) {
            case 't':
                if (sscanf(optarg, "%i", &(ret->n_threads)) != 1 || (int) ret->n_threads < 1) {
                    fprintf(stderr, "Invalid number of threads, %s\n", optarg);
                    usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if (sscanf(optarg, "%i", &(ret->iterations)) != 1 || (int) ret->iterations < 1) {
                    fprintf(stderr, "Invalid number of iterations, %s\n", optarg);
                    usage();
                    exit(EXIT_FAILURE);
                }
                break;
            case 'y':
                if (strchr(optarg, 'i')) {
                    ret->opt_yield |= INSERT_YIELD;
                    strcat(ret->yieldopts, "i");
                }
                if (strchr(optarg, 'd')) {
                    ret->opt_yield = DELETE_YIELD;
                    strcat(ret->yieldopts, "d");
                }
                if (strchr(optarg, 'l')) {
                    ret->opt_yield |= LOOKUP_YIELD;
                    strcat(ret->yieldopts, "l");
                } //Israeli Defense League?
                break;
            case 's':
                if (strchr(optarg, 'm')) { //mutex doesnt have an 's'
                    ret->sync_mode = MUTEX;
                    break;
                } else if (strchr(optarg, 's')) { //spin lock doesnt have an 'm'
                    ret->sync_mode = SPINLOCK;
                    break;
                }
                //again, someone fucked up if we're here
                fprintf(stderr, "Unknown synchronization system: %s\n", optarg);
                exit(EXIT_FAILURE);
            case '?':
                fprintf(stderr, "Unrecognized option:\n");
                usage();
                exit(EXIT_FAILURE);
            case ':':
                fprintf(stderr, "Unrecognized argument to option %s: %s\n", argv[index + 1], optarg);
                usage();
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Very wrong arguments and options given (getopt status code: %x", getopt_status);
                usage();
                exit(EXIT_FAILURE);
        }
    }
    opt_yield = ret->opt_yield;
    return ret;
}

void* list(void* v_behavior){
    struct behavior* behavior = (struct behavior* ) v_behavior;
    clock_gettime(CLOCK_MONOTONIC_RAW, &behavior->start);
    fill_list(behavior);
    verify_list(behavior);
    empty_list(behavior);
    fill_list(behavior);
    verify_list(behavior);
    empty_list(behavior);
    clock_gettime(CLOCK_MONOTONIC_RAW, &behavior->end);
    pthread_exit(NULL);
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

void join_threads(pthread_t* threads, struct behavior* behavior){
    if (behavior->n_threads < 1){
        fprintf(stderr, "Number of threads, %u, is not valid\n", behavior->n_threads);
        exit(EXIT_FAILURE);
    }
    for (int i=0; i<behavior->n_threads; ++i){
        if (pthread_join((threads[i]), NULL)) {
            fprintf(stderr, "Unable to join threads: %s\n", strerror(errno));
            exit((EXIT_FAILURE));
        }
    }
    fprintf(stdout, "list");
    if (behavior->yieldopts[0]){
        fprintf(stdout, "-%s", behavior->yieldopts);
    } else {
        fprintf(stdout, "-none");
    }
    switch (behavior->sync_mode){
        case SPINLOCK:
            fprintf(stdout, "-s");
            break;
        case MUTEX:
            fprintf(stdout, "-m");
            break;
        case NONE:
            fprintf(stdout, "-none");
            break;
    }
    pthread_mutex_destroy(behavior->mutex);
    long run_time = (behavior->end.tv_sec*1000) - (behavior->start.tv_sec*1000);//abs_diff(endtime.tv_nsec, starttime.tv_nsec);
    run_time*=1000000000;
    run_time += behavior->end.tv_nsec;
    run_time -= behavior->start.tv_nsec;
    unsigned opers = 3 * behavior->n_threads * behavior->iterations;
    long avg_runtime = run_time / opers;
    fprintf(stdout, ",%u,%u,%u,%i,%li,%li\n", behavior->n_threads, behavior->iterations, 1, opers, run_time, avg_runtime);
}