default: build

build: lab2_list lab2_add

lab2_list: lab2_list.c SortedList.c SortedList.h
	gcc -Wall -Wextra -pthread SortedList.c lab2_list.c -o lab2_list -g -std=c99

lab2_add: lab2_add.c
	gcc -Wall -Wextra -pthread lab2_add.c -o lab2_add -g

clean:
	rm -rf lab2_add lab2_list lab2a-804663317.tar.gz *.o  

dist: build tests graphs
	tar -cvzf lab2a-804663317.tar.gz lab2_list.c lab2_add.c SortedList.c SortedList.h README Makefile *.gp *.png *.csv test.sh

tests: build test.sh
	@chmod +x test.sh
	-./test.sh

graphs: tests
	@chmod +x ./lab2_add.gp
	@chmod +x ./lab2_list.gp
	./lab2_add.gp
	./lab2_list.gp
