##
# NAME: Maria E.V. Neptune
# ID:	004-665-056
# EMAIL:jerenept@gmail.com
lab1a: lab1a.c
	gcc lab1a.c -o lab1a -g -Og 
testing: lab1a.c #debug 
	gcc lab1a.c -o lab1a -g -Og -D DEBUG_PIPES
check: testing_build 
#	./test.sh
release: 
	gcc lab1a.c -o lab1a 
clean:
	rm lab1a
	rm -f lab1a-004-665-056.tar.gz
dist: check release README 
	tar -cvzf lab1a-004-665-056.tar.gz Makefile README lab1a.c test.sh 
debug: lab1a
	gdb lab1a 
run:
	./lab1a
testing_build: 
	gcc -D TESTING_CS111_MARIA -o lab1a-testing test.c lab1a.c -Wall
	./lab1a-testing
	@rm lab1a-testing 
.PHONY: clean check release dist debug run testing_build testing
