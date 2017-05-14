##
# Name:	Maria E. V. Neptune
# Email:jerenept@gmail.com
# Id: 	004-665-056
#
lab1b: lab1b-client lab1b-server 

lab1b-client : lab1b-client.c
	gcc lab1b-client.c lab1b.c -o lab1b-client -g -Og -lmcrypt -Wall
lab1b-server: lab1b-server.c
	gcc lab1b-server.c lab1b.c -o lab1b-server -g -Og -DSERVER -lmcrypt -Wall
clean: 
	rm lab1b-server lab1b-client 
	rm -f lab1b-004-665-056.tar.gz

dist: lab1b 
	tar -cvzf lab1b-004-665-056.tar.gz lab1b-client.c lab1b.c lab1b.h lab1b-server.c Makefile README my.key
.PHONY: clean check lab1b 
