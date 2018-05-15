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
#include <math.h>

int opt_yield = 0;
int num_iter = 1;
int num_thread = 1;
int num_element = 1;
static int key_length = 5;
char key_candidate[53]= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
int num_list = 1;
char* yield_opt = NULL;
enum sync_type {NO_LOCK, MUTEX, SPIN_LOCK};
enum sync_type opt_sync = NO_LOCK;

typedef struct SubList
{	
	SortedList_t list;
	pthread_mutex_t mutex;
	int lock;
}SubList_t;

SubList_t* my_list;
SortedListElement_t* element_list;

long long* waittime_list = NULL;

void cleanup()
{
	if(opt_sync == MUTEX)
		for (int i = 0; i < num_list; ++i)
		{
			pthread_mutex_destroy(&my_list[i].mutex);
		}
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
		exit(1);
	}
}

unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)>0)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void* thread_func(void* arg)
{
	int thread_id = *((int*) arg);
	int i;
	int ret;
	struct timespec func_start_time;
	struct timespec func_end_time;
	//instering list elements into according hash function mapping
	for (i = thread_id; i < num_element; i=i + num_thread)
	{	
		int hash_ret = hash(element_list[i].key);
		int bucket = abs(hash_ret % num_list);
		switch(opt_sync)
		{
			case NO_LOCK:
				SortedList_insert(&my_list[bucket].list, &element_list[i]);
				break;

			case MUTEX:
				ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}

				pthread_mutex_lock(&my_list[bucket].mutex);

				ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);
				SortedList_insert(&my_list[bucket].list, &element_list[i]);
				pthread_mutex_unlock(&my_list[bucket].mutex);
				break;

			case SPIN_LOCK:
				ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				while(__sync_lock_test_and_set(&my_list[bucket].lock, 1) == 1);
				ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);
				SortedList_insert(&my_list[bucket].list, &element_list[i]);
				__sync_lock_release(&my_list[bucket].lock);
				break;
		}
	}

	int list_length =0;
	int sublist_length = 0;
	//getting the sublist length and sum up
	switch(opt_sync)
	{
		case NO_LOCK:

			for(i = 0; i < num_list; i++)
			{
				sublist_length = SortedList_length(&my_list[i].list);
				if(sublist_length < 0)
				{
					fprintf(stderr, "Error SortedList_length(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				list_length += sublist_length;
			}
			if(list_length<0)
			{
				fprintf(stderr, "Error list length is negative. : %s\n", strerror(errno));
				exit(2);
			}
			break;

		case MUTEX:
			//grab all the locks
			ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
			if (ret < 0)
			{
				fprintf(stderr, "Error clock_gettime func_start_time: %s\n", strerror(errno));
				exit(1);
			}
			for(i = 0; i < num_list; i++)
			{
				pthread_mutex_lock(&my_list[i].mutex);
			}

			ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
			if (ret < 0)
			{
				fprintf(stderr, "Error clock_gettime func_end_time: %s\n", strerror(errno));
				exit(1);
			}
			waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);

			for(i = 0; i < num_list; i++)
			{
				sublist_length = SortedList_length(&my_list[i].list);
				if(sublist_length < 0)
				{
					fprintf(stderr, "Error SortedList_length() in MUTEX option. SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				list_length += sublist_length;
			}
			if(list_length<0)
			{
				fprintf(stderr, "Error list length is negative. : %s\n", strerror(errno));
				exit(2);
			}

			for(i = 0; i < num_list; i++)
			{
				pthread_mutex_unlock(&my_list[i].mutex);
			}
			break;

		case SPIN_LOCK:
			ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
			if (ret < 0)
			{
				fprintf(stderr, "Error clock_gettime func_start_time: %s\n", strerror(errno));
				exit(1);
			}
			for (i = 0; i < num_list; ++i)
			{
				while(__sync_lock_test_and_set(&my_list[i].lock, 1) == 1);
			}

			ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
			if (ret < 0)
			{
				fprintf(stderr, "Error clock_gettime func_end_time: %s\n", strerror(errno));
				exit(1);
			}
			waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);

			for(i = 0; i < num_list; i++)
			{
				sublist_length = SortedList_length(&my_list[i].list);
				if(sublist_length < 0)
				{
					fprintf(stderr, "Error SortedList_length(). SortedList corruption detected. : %s\n", strerror(errno));
					exit(2);
				}
				list_length += sublist_length;
			}

			if(list_length<0)
			{
				fprintf(stderr, "Error list length is negative. : %s\n", strerror(errno));
				exit(2);
			}
			for (i = 0; i < num_list; ++i)
			{
				__sync_lock_release(&my_list[i].lock);			
			}
			break;
	}

	SortedListElement_t* retrieved;
	for (i = thread_id; i < num_element; i=i + num_thread)
	{
		int hash_ret = hash(element_list[i].key);		
		int bucket = abs(hash_ret % num_list);
		switch(opt_sync)
		{
			case NO_LOCK:
				retrieved = SortedList_lookup(&my_list[bucket].list, element_list[i].key);
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
				ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}

				pthread_mutex_lock(&my_list[bucket].mutex);
				ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);
				retrieved = SortedList_lookup(&my_list[bucket].list, element_list[i].key);
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
				pthread_mutex_unlock(&my_list[bucket].mutex);
				break;

			case SPIN_LOCK:
				ret = clock_gettime(CLOCK_MONOTONIC, &func_start_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				while(__sync_lock_test_and_set(&my_list[bucket].lock, 1) == 1);
				ret = clock_gettime(CLOCK_MONOTONIC, &func_end_time);
				if (ret < 0)
				{
					fprintf(stderr, "Error clock_gettime: %s\n", strerror(errno));
					exit(1);
				}
				waittime_list[thread_id] += (func_end_time.tv_sec - func_start_time.tv_sec)*1000000000 + (func_end_time.tv_nsec - func_start_time.tv_nsec);
				retrieved = SortedList_lookup(&my_list[bucket].list, element_list[i].key);
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
				__sync_lock_release(&my_list[bucket].lock);
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
	//option as a struct and get the different types of options
	struct option options[] = 
	{
		{"yield", 1, NULL, 'y'},
		{"iterations", 1, NULL, 'i'},
		{"threads", 1, NULL, 't'},
		{"sync", 1, NULL, 's'},
		{"lists",1,NULL, 'l'}
	};

	int o;
	int i;
	int ret = 0;
	int y_opt_len;
	while ((o = getopt_long(argc, argv, "t:i:y:s:l", options, NULL)) != -1)
	{
		switch(o)
		{
			case 'l':
				num_list = (int)atoi(optarg);
				break;
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
						//pthread_mutex_init(&mutex, NULL);
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
	else
	{
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
	waittime_list = (long long *) malloc(num_thread * sizeof(long long));
	//create and aloocate size of the memory required for number of elements.
	element_list = malloc(num_element * sizeof(SortedListElement_t));
	//generate random key for element list

	srand((unsigned int) time(NULL));
	for (int i = 0; i < num_element; i++)
	{
		char* my_key = malloc((key_length + 1) * (sizeof(char)));
		for (int i = 0; i < key_length; i++) 
		{
			my_key[i] = key_candidate[rand() % 52];
		}
		my_key[key_length] = '\0';
		element_list[i].key = my_key;
	}
		
    my_list = (SubList_t*) malloc (sizeof(SubList_t) * num_list);

	for (i = 0; i < num_list; ++i)
	{
		my_list[i].list.prev = &my_list[i].list;
		my_list[i].list.next = &my_list[i].list;
		my_list[i].list.key = NULL;

		if (opt_sync == MUTEX)
			pthread_mutex_init(&my_list[i].mutex, NULL);
		if (opt_sync ==SPIN_LOCK)
			my_list[i].lock = 0;
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
	//pthread create
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
	//pthread join
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

	int total_length = 0;
	int sub_length = 0;

	for (i = 0; i < num_list; i++)
	{	
		sub_length = SortedList_length(&my_list[i].list);
		if (sub_length < 0)
		{
			fprintf(stderr, "Error while getting sub_length. One sublist is corrupted: %s\n", strerror(errno));
			exit(2);
		}
		total_length += sub_length;
	}

	if (total_length != 0)
	{
		fprintf(stderr, "Error: Corrupted list. Final list length return is not 0.\n");
        exit(2);
	}

	long long total_wait_time = 0;

	for (int i = 0; i < num_thread; i++)
	{
		total_wait_time += waittime_list[i];
	}

	int num_lock = (2 *	num_iter + 1) * num_thread;
	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec)* 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	int num_ops = 3 * num_thread * num_iter;

	long long avg_run_time = my_elapsed_time_in_ns/num_ops;
	long long wait_for_lock_time = total_wait_time/num_lock;
	fprintf(stdout, "%s,%d,%d,%d,%d,%lld,%lld,%lld\n", test_name, num_thread, num_iter, num_list, num_ops, my_elapsed_time_in_ns, avg_run_time, wait_for_lock_time);
	exit(0);
}

