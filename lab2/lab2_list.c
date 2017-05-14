//
// Created by maria on 5/10/17.
//
#include <sys/types.h>
#include "list_utils.h"
int main(int argc, char** argv){
    struct behavior* behavior = argparse(argc, argv);
    behavior->key_list = manufacture_list(behavior);
    pthread_t* threads = manufacture_threads(behavior->n_threads, list, behavior);
    join_threads(threads, behavior);
}