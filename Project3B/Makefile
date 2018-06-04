default:
	rm -rf lab3b
	echo '#!/usr/local/cs/bin/python3' > lab3b
	cat lab3b.py >> lab3b
	chmod +x lab3b

dist: default
	tar -czvf lab3b-804663317.tar.gz README Makefile lab3b.py

clean: 
	rm -rf lab3b lab3b-804663317.tar.gz
