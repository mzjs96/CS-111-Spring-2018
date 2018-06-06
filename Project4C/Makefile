default:
	gcc -Wall -Wextra -std=gnu99 -g -lmraa -lssl -lcrypto -lm -o lab4c_tcp lab4c.c
	gcc -Wall -Wextra -std=gnu99 -g -lmraa -lssl -lcrypto -lm -o lab4c_tls lab4c.c 
clean:
	rm -f lab4c-804663317.tar.gz lab4c_tcp lab4c_tls
dist:
	tar -czf lab4c-804663317.tar.gz README Makefile lab4c.c 