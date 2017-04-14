##NAME:		Maria E.V. Neptune
##EMAIL:	jerenept@gmail.com
##ID:		004-665-056
##Copyright:GNU GPL v3 (http://gnu.org/licenses)

lab0: lab0.c
	gcc lab0.c -o lab0 -g -Og
check: lab0 test.sh
	@./test.sh 2>/dev/null #my test harness 
release: lab0.c
	gcc lab0.c -o lab0 
clean: 
	rm lab0 
	rm -f lab0-004-665-056.tar.gz
edit: 
	vim lab0.c
dist: check release backtrace.png breakpoint.png README test.sh P0_check.sh 
	tar -cvzf lab0-004-665-056.tar.gz P0_check.sh Makefile backtrace.png breakpoint.png README lab0.c test.sh
	./P0_check.sh "004-665-056" #run instructor tests 
debug: lab0 
	gdb ./lab0 
run: lab0 
	./lab0 
.PHONY: clean check release edit debug run dist 
# Resources used to create this Makefile: 
# GNU Make Manual (https://gnu.org/software/make/manual/make.html)
