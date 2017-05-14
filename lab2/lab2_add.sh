#!/bin/bash 
syncs="spinlock mutex compare-swap"
iters_s="10 20 40 80 100 1000 10000 100000"
threads_s="1 2 3 4 5 6 7 8 12"
for iters in $iters_s; do
    for threads in $threads_s; do 
        for sync in $syncs; do
            >&2 echo "add: Running test with $threads threads, $iters iterations and $sync syncronisation, yielding"
            ./lab2_add --threads=$threads --iterations=$iters --sync=$sync --yield            
            >&2 echo "add: Running test with $threads threads, $iters iterations and $sync syncronisation, no yielding"
            ./lab2_add --threads=$threads --iterations=$iters --sync=$sync
        done
        >&2 echo "add: Running test with $threads threads, $iters iterations and no syncronisation, yielding"
        ./lab2_add --threads=$threads --iterations=$iters --yield 
        >&2 echo "add: Running test with $threads threads, $iters iterations and no syncronisation, no yielding"
        ./lab2_add --threads=$threads --iterations=$iters
    done
done
