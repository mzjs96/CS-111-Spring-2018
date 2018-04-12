.SILENT:

default:
	gcc -g -o lab0 lab0.c -Wall -Wextra

check: default creatTest writeTest segfaultTest trivialTest exit0test cmpTest
	if [[ ! -s log.txt ]]; then \
		echo "Congratulations, Passed all tests!"; \
	else \
		cat log.txt; \
		rm -f *.txt; \
	fi;
clean:
	rm -f lab0 *.txt *.tar.gz
dist:
	tar -cvzf lab0-804663317.tar.gz Makefile lab0.c backtrace.png breakpoint.png README

#smoke tests
creatTest:
	echo "This is the content for test1.txt" > test1.txt; \
	chmod u-r test1.txt; \
	./lab0 --input=test1.txt --output=temp.txt 1&>/dev/null; \
	if [[ $$? -ne 2 ]]; then \
		echo "fail to open a file and did not exit 2" >> log.txt; \
	fi; \
	rm -f test1.txt temp.txt
writeTest:
	echo "This is the content for test2.txt" > test2.txt; \
	echo "This is the content for test3.txt" > test3.txt; \
	chmod u-w test3.txt;
	./lab0 --input=test2.txt --output=test3.txt 1&> /dev/null; \
	if [[ $$? -eq 3 ]]; then \
		:; \
	else \
		echo "Failure to write to file and did not exit(3)." > log.txt; \
	fi; \
	rm -f test2.txt test3.txt
segfaultTest:
	./lab0 --catch --segfault 1&>/dev/null; \
	if [[ $$? -ne 4 ]]; then \
		echo "Segfault design is wrong and the program did not exit(4)." >> log.txt; \
	fi;
trivialTest:
	./lab0 --trivial 1&>/dev/null; \
	if [[ $$? -ne 1 ]]; then \
		echo "Unknown argument not handled and did not exit(1)." >> log.txt; \
	fi;
exit0test:
	echo "This is the content for test4.txt" >>test4.txt; \
	echo "This is the content for test5.txt" >>test5.txt; \
	if [[ $$? -ne 0 ]]; then \
		echo "The read and write did not exit(0)." >> log.txt; \
	fi; \
	rm -f test4.txt test5.txt

cmpTest:
	echo "This is the content for test6.txt" >> test6.txt;
	./lab0 --input=test6.txt --output=test7.txt 1&>/dev/null; \
	cmp test6.txt test7.txt; \
	if [[ $$? -ne 0 ]]; then \
		echo "Contents from input and output files do not match, there is no specified exit code." >> log.txt; \
	fi; \
	rm -f test6.txt test7.txt
