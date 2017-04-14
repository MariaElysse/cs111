#!/bin/bash
set -u
function test_exit {
    if [[ $? -ne $1 ]]; then 
        exit 1
    fi
}

echo "Testing for errors that should exist"
echo "Passed:"
./lab0 --segfault 2> /dev/null
test_exit 139
echo "Segfault test"

./lab0 --input 2> /dev/null 
test_exit 1
echo "Missing argument to --input"

./lab0 --output 2> /dev/null
test_exit 1
echo "Missing argument to --output"

./lab0 --catch --segfault 2> /dev/null 
test_exit 4
echo "Caught Segmentation Fault"

./lab0 --catch --segfault --input=Makefile 2> /dev/null 
test_exit 4
temp_file=$(mktemp)
rm $temp_file
./lab0 --catch --segfault --output="$temp_file"  2>/dev/null 
test_exit 4
#note: if the --catch/--segfault options are there we shouldn't even consider the requirements
#for the --input/output options; they are irrelevant
./lab0 --catch --segfault --output=Makefile 2>/dev/null #as such, this should execute the segfault uncomplainingly
test_exit 4
./lab0 --segfault --input=Makefile --catch 2>/dev/null
test_exit 4
echo "All other options ignored when segmentation fault requested"

echo "Testing for errors that shouldn't exist"
set -e
from_cat="$(cat Makefile)"
from_pgm="$(./lab0 --input=Makefile)"

if [[ "$from_cat" != "$from_pgm" ]]; then
    exit 1
fi

echo "Correctly reads from file to stdout" 
rand="$(od /dev/urandom -Ax -x -N 8 | head -n 1)"
tempfile="$(mktemp)"
rm $tempfile
./lab0 --output="$tempfile" <<< $rand
result="$(cat $tempfile)"
if [[ "$result" != "$rand" ]]; then 
    exit 1
fi
echo "Correctly reads from stdin to file"

temp_source="$(mktemp)"
temp_dest="$(mktemp)"
rm $temp_dest
od /dev/urandom -Ax -x -N 8 | head -n 1 > $temp_source
./lab0 --input=$temp_source --output=$temp_dest 
if ! cmp $temp_source $temp_dest; then 
    exit 1
fi
echo "Correctly reads from file to file"

rand="$(od /dev/urandom -Ax -x -N 8 | head -n 1)"
output="$(./lab0 <<< $rand)"
if [[ "$output" != "$rand" ]]; then 
    exit 1
fi
echo "Correctly reads from stdin to stdout"
exit 0
