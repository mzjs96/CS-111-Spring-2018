.SILENT:

default: server client

server:
	gcc -Wall -Wextra lab1b-server.c -o lab1b-server -lz -std=c99

client:
	gcc -Wall -Wextra lab1b-client.c -o lab1b-client -lz

dist:
	tar -czvf lab1b-804663317.tar.gz lab1b-server.c lab1b-client.c README Makefile  

clean: 
	rm -f lab1b-server lab1b-client lab1b-804663317.tar.gz 