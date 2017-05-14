#!/bin/bash 
syncs="spinlock mutex"
iters_s="1 10 100 1000 2 4 8 16 32 10000 20000"
threads_s="1 2 4 8 12 16 24"
yields="i d l id il dl idl"
for iters in $iters_s; do
    for threads in $threads_s; do 
        for sync in $syncs; do
            for yield in $yields; do 
                if [[ $iters -eq "10000" ]] || [[ $iters -eq "20000" ]] &&  [[ $sync == "spinlock" ]]; then 
                    continue;
                fi 
                >&2 echo "list: Running test with $threads threads, $iters iterations and $sync syncronisation, yielding"
                ./lab2_list --threads=$threads --iterations=$iters --sync=$sync --yield=$yield            
                >&2 echo "list: Running test with $threads threads, $iters iterations and $sync syncronisation, no yielding"
                ./lab2_list --threads=$threads --iterations=$iters --sync=$sync
            done
        done
        >&2 echo "list: Running test with $threads threads, $iters iterations and no syncronisation, yielding"
        ./lab2_list --threads=$threads --iterations=$iters --yield=$yield
        >&2 echo "list: Running test with $threads threads, $iters iterations and no syncronisation, no yielding"
        ./lab2_list --threads=$threads --iterations=$iters
    done
done  
