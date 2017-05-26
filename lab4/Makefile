## NAME: Maria E. V. Neptune
#  EMAIL:jerenept@gmail.com
#  ID:	 004-665-056 	

EDISON_IP = "131.179.37.106"

lab4b: lab4b.c lab4-utils.c lab4-utils.h
	gcc -o lab4b -Og -g -lm -lmraa lab4-utils.c lab4b.c 
check:
	./P4B_check.sh 004-665-056 
clean:
	rm -f lab4b-004-665-056.tar.gz lab4b log*.txt
deploy:
	./lab4_deploy.sh $(EDISON_IP) 
dist:
	tar -cvf lab4b-004-665-056.tar.gz lab4b.c lab4-utils.c Makefile README lab4_deploy.sh lab4-utils.h P4B_check.sh 
.PHONY: deploy test 
