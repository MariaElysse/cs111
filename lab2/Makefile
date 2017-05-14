##
# Name: Maria E. V. Neptune
# Email:jerenept@gmail.com
# Id: 	004-665-056
.PHONY: tests graphs dist clean
build: lab2_add lab2_list 
tests: dist #test all
	./P2A_check.sh 004-665-056 
graphs: lab2_add.csv lab2_list.csv 
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp 
make_csvs:  
	./sample.sh 
lab2_add.csv: make_csvs
lab2_list.csv: make_csvs  
dist: graphs #make the archive
	tar -cvzf lab2a-004-665-056.tar.gz sample.sh lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png lab2_list.c lab2_add.c add_utils.c add_utils.h list_utils.c list_utils.h lab2_list.sh lab2_add.sh SortedList.c SortedList.h lab2_add.gp lab2_list.gp Makefile README lab2_rpi.tar.gz P2A_check.sh 
clean:
	rm -f lab2_list lab2_add lab2_list.csv lab2_add.csv lab2a-004-665-056.tar.gz
lab2_add: lab2_add.c add_utils.c
	gcc -o lab2_add -g -Og -pthread -lprofiler lab2_add.c add_utils.c
lab2_list: lab2_list.c list_utils.c SortedList.c
	gcc -o lab2_list -g -O0 -lprofiler -pthread lab2_list.c list_utils.c SortedList.c 
