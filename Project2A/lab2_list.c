/*
	NAME: MICHAEL ZHOU
	EMAIL: mzjs96@gmail.com
	UID:804663317
*/


#include "SortedList.h"

#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

int opt_yield = 0;
int num_iter = 1;
int num_thread = 1;
int num_element = 1;
static int key_length = 5;
static int lock = 0;
char key_candidate[53]= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

SortedList_t* my_list = NULL;
SortedListElement_t* element_list = NULL;

char* yield_opt = NULL;

pthread_mutex_t mutex;

enum sync_type {NO_LOCK, MUTEX, SPIN_LOCK};

enum sync_type opt_sync = NO_LOCK;

void cleanup()
{
	if(opt_sync == MUTEX)
		pthread_mutex_destroy(&mutex);
	if(my_list)
		free(my_list);
	if(element_list)
		free(element_list);
}

void sig_handler(int signum)
{
	if(signum == SIGSEGV)
	{
		fprintf(stderr, "Error. Signal %d SIGSEGV Detected: %s\n", signum, strerror(errno));
		exit(2);
	}
}

char* random_key() {
    //  http://stackoverflow.com/questions/33464816/c-generate-random-string-of-max-length
    srand((unsigned int)time(NULL));
    char* my_key = malloc((key_length + 1) * (sizeof(char)));
    int i;
    for (i = 0; i < key_length; i++) {
        my_key[i] = key_candidate[rand() % 52];
    }
    my_key[key_length] = '\0';
    return my_key;
}

void* thread_func(void* arg)
{
	int thread_id = *((int*) arg);
	int i;
	int ret;
	for (i = thread_id; i < num_element; i=i + num_thread)
	{
		switch(opt_sync)
		{
			case NO_LOCK:
				SortedList_insert(my_list, &element_list[i]);
				break;
			case MUTEX:
				pthread_mutex_lock(&mutex);
				SortedList_insert(my_list, &element_list[i]);
				pthread_mutex_unlock(&mutex);
				break;
			case SPIN_LOCK:
				while(__sync_lock_test_and_set(&lock, 1) == 1);
				SortedList_insert(my_list, &element_list[i]);
				__sync_lock_release(&lock);
				break;
		}
	}

	switch(opt_sync)
	{
		case NO_LOCK:
			ret = SortedList_length(my_list);
			if(ret < 0)
			{
				fprintf(stderr, "Error SortedList_length(). SortedList corruption detected. : %s\n", strerror(errno));
				exit(2);
			}
			break;
		case MUTEX:
			pthread_mutex_lock(&mutex);
			ret = SortedList_length(my_list);
			if(ret < 0)
			{
				fprintf(stderr, "Error SortedList_length(). SortedList corruption detected. : %s\n", strerror(errno));
				exit(2);
			}
			pthread_mutex_unlock(&mutex);
			break;
		case SPIN_LOCK:
			while(__sync_lock_test_and_set(&lock, 1) == 1);
			ret = SortedList_length(my_list);
			if(ret < 0)
			{
				fprintf(stderr, "Error SortedList_length(). SortedList corruption detected. : %s\n", strerror(errno));
				exit(2);
			}
			__sync_lock_release(&lock);
			break;
	}

	SortedListElement_t* retrieved;

	for (i = thread_id; i < num_element; i = i + num_thread)
	{
		switch(opt_sync)
		{
			case NO_LOCK:
				retrieved = SortedList_lookup(my_list, element_list[i].key);
				if (retrieved == NULL)
				{
					fprintf(stderr, "Error SortedList_lookup(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				ret = SortedList_delete(retrieved);
				if (ret == 1)
				{
					fprintf(stderr, "Error SortedList_delete(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				break;
			case MUTEX:
				pthread_mutex_lock(&mutex);
				retrieved = SortedList_lookup(my_list, element_list[i].key);
				if (retrieved == NULL)
				{
					fprintf(stderr, "Error SortedList_lookup(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				ret = SortedList_delete(retrieved);
				if (ret == 1)
				{
					fprintf(stderr, "Error SortedList_delete(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				pthread_mutex_unlock(&mutex);
				break;
			case SPIN_LOCK:
				while(__sync_lock_test_and_set(&lock, 1) == 1);
				retrieved = SortedList_lookup(my_list, element_list[i].key);
				if (retrieved == NULL)
				{
					fprintf(stderr, "Error SortedList_lookup(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				ret = SortedList_delete(retrieved);
				if (ret == 1)
				{
					fprintf(stderr, "Error SortedList_delete(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				__sync_lock_release(&lock);
				break;
		}
	}
	return NULL;
}

int main(int argc, char ** argv)
{
	//atexit clean up
	if(atexit(cleanup) == -1)
	{
		fprintf(stderr, "Error atexit cleanup(): %s\n", strerror(errno));
		exit(1);
	}
	//sigsegv handler
	if(signal(SIGSEGV, sig_handler) == SIG_ERR)
	{
		fprintf(stderr, "Error signal handler: %s\n", strerror(errno));
		exit(1);
	}
	//get options
	struct option options[] = 
	{
		{"yield", 1, NULL, 'y'},
		{"iterations", 1, NULL, 'i'},
		{"threads", 1, NULL, 't'},
		{"sync", 1, NULL, 's'},
	};

	int o;
	int i;
	int ret = 0;
	int y_opt_len;
	while ((o = getopt_long(argc, argv, "i:o:sc", options, NULL)) != -1)
	{
		switch(o)
		{
			case 'i':
				num_iter = (int)atoi(optarg);
				break;
			case 't':
				num_thread = (int)atoi(optarg);
				break;
			case 's':
				switch(*optarg)
				{
					case'm':
						opt_sync = MUTEX;
						pthread_mutex_init(&mutex, NULL);
						break;
					case 's':
						opt_sync = SPIN_LOCK;
						break;
					default:
						fprintf(stderr, "usage: lab2_add [--yield] [--iterations=#] [--threads=#] [--sync=[msc]]\n");
						exit(1);
				}
				break;
			case 'y':
				y_opt_len = strlen(optarg);
				if(y_opt_len >3)
				{	
					fprintf(stderr, "Error: Bad arguments are encountered. \n");
                    exit(1);
                }
				for (i = 0; i < y_opt_len; i++)
				{
					switch(optarg[i])
					{
						case 'i':
							opt_yield = opt_yield | INSERT_YIELD;
							break;
						case 'd':
							opt_yield = opt_yield | DELETE_YIELD;
							break;
						case 'l':
							opt_yield = opt_yield | LOOKUP_YIELD;
							break;
						default:
							fprintf(stderr, "usage: lab2_list [--yield=[idl]] [--iterations=#] [--threads=#] [--sync=[ms]]\n");
							exit(1);
					}
				}
				break;
			default:
				fprintf(stderr, "usage: lab2_list [--yield=[idl]] [--iterations=#] [--threads=#] [--sync=[ms]]\n");
				exit(1);
		}
	}
	char* base = "list-";
	char test_name[15];
	strcpy(test_name, base);
	if(opt_yield ==0)
	{
		strcat(test_name, "none");
	}
	else{
		if(opt_yield & INSERT_YIELD)
			strcat(test_name, "i");
		if(opt_yield & DELETE_YIELD)
			strcat(test_name, "d");
		if(opt_yield & LOOKUP_YIELD)
			strcat(test_name, "l");
	}

	if (opt_sync == MUTEX)
	{
		strcat(test_name, "-m");
	}
	else if(opt_sync == SPIN_LOCK)
	{
		strcat(test_name, "-s");
	}
	else //if(opt_sync == NO_LOCK)
	{
		strcat(test_name, "-none");
	}

	num_element = num_thread * num_iter;

	my_list = malloc(sizeof(SortedList_t));
	my_list->prev = my_list;
	my_list->next = my_list;
	my_list->key = NULL;

	//create and aloocate size of the memory required for number of elements.
	element_list = malloc(num_element * sizeof(SortedListElement_t));

	//generate random key for element list
	for (i = 0; i < num_element; ++i)
	{
		element_list[i].key = random_key();
	}

	pthread_t threads[num_thread];
	int thread_id[num_thread];

	struct timespec my_start_time;
	ret = clock_gettime(CLOCK_MONOTONIC, &my_start_time);
	if (ret <0) 
	{
		fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
		exit(1);	
	}

	for (i = 0; i < num_thread; ++i)
	{	
		thread_id[i]= i;
		ret = pthread_create(&threads[i], NULL, thread_func, &thread_id[i]);
		if (ret < 0)
		{
			fprintf(stderr, "Error pthread_create: %s\n", strerror(errno));
			exit(2);
		}
	}

	for (i = 0; i < num_thread; ++i)
	{
		ret = pthread_join(threads[i], NULL);
		if (ret < 0)
		{
			fprintf(stderr, "Error pthread_create: %s\n", strerror(errno));
			exit(2);
		}
	}

	struct timespec my_end_time;
	ret = clock_gettime(CLOCK_MONOTONIC, &my_end_time);
	if (ret < 0)
	{
		fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
		exit(1);
	}

	if (SortedList_length(my_list) != 0)
	{
		fprintf(stderr, "Error: Corrupted list. Final list length is not 0.\n");
        exit(2);
	}

	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec)* 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	int num_ops = 3 * num_thread * num_iter;
	long long avg_run_time = my_elapsed_time_in_ns/num_ops;

	int num_list = 1;

	fprintf(stdout, "%s,%d,%d,%d,%d,%lld,%lld\n", test_name, num_thread, num_iter, num_list, num_ops, my_elapsed_time_in_ns, avg_run_time);
	exit(0);

}