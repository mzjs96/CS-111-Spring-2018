default: build

build: lab4b

lab4b: lab4b.c
	gcc -Wall -Wextra -std=gnu99 -g -lm -o lab4b lab4b.c -lmraa

clean:
	rm -f lab4b-804663317.tar.gz lab4b

dist: lab4b
	tar -cvzf lab4b-804663317.tar.gz lab4b.c Makefile README

check: lab4b
	echo "SCALE=F\nPERIOD=0\nSCALE=C\nSTART\nSTART\nSTOP\nSTOP\nSTART\nLOG test\nLOG test\nLOG test\nOFF" | ./lab4b --period=2 --scale=C --log="test.txt"; \
	EXIT_CODE=$$?;\
    echo "command exited with $$EXIT_CODE";\
    exit $$EXIT_CODE