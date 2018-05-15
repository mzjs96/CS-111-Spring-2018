default: build

build: lab2_list

lab2_list: lab2_list.c SortedList.h SortedList.c
	gcc -Wall -Wextra -pthread -L /lib64 -lprofiler SortedList.c lab2_list.c -o lab2_list -g -std=c99

clean:
	rm -f lab2_list *.tar.gz

dist: build tests
	tar -cvzf lab2b-804663317.tar.gz lab2_list.c SortedList.c SortedList.h README Makefile lab2_list.gp *.png lab2b_list.csv test.sh profile.out

profile: build
	-rm -f ./raw.gperf profile.out
	LD_PRELOAD=/u/ee/ugrad/jiashu/lib/libprofiler.so.0 
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	@pprof --text ./lab2_list ./raw.gperf > profile.out
	@pprof --list=thread_func ./lab2_list ./raw.gperf >> profile.out
	-rm -f ./raw.gperf

tests: build test.sh
	rm -rf *.csv
	@chmod +x test.sh
	-./test.sh

graphs: tests 
	@chmod +x ./lab2_list.gp
	./lab2_list.gp
