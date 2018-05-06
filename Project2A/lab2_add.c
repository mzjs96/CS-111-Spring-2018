/*
	MICHAEL ZHOU
	UID: 804663317
	MZJS96@GMAIL.COM
*/



#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

int opt_yield = 0;
int num_iter = 1;
int num_thread = 1;

char * test_name = NULL;

enum sync_type {NO_LOCK, MUTEX, SPIN_LOCK, COMPARE_SWAP};

enum sync_type opt_sync = NO_LOCK;
pthread_mutex_t my_mutex;
int my_spin_lock;


void add(long long *pointer, long long value) 
{
	long long sum = *pointer + value;
	if (opt_yield)
		sched_yield();
	*pointer = sum;
}

void add_compare_swap(long long *pointer, long long value)
{
	long long curr;
	long long new;
	do{
		curr = *pointer;
		new = curr + value;
		if(opt_yield)
			sched_yield();
	} while (__sync_val_compare_and_swap(pointer, curr, new) != curr);
}

void clean_up()
{
	if (opt_sync == MUTEX)
	{
		pthread_mutex_destroy(&my_mutex);
	}
}

void* thread_function_to_run_test (void* counter_ptr)
{
	long long * pointer = (long long *) counter_ptr;
	int i;
	for (i = 0; i < num_iter; ++i)
	{
		// first perform number of iteration +1
		switch(opt_sync)
		{
			case NO_LOCK:
			{
				add(pointer, 1);
				break;
			}
			case MUTEX:
			{
				pthread_mutex_lock(&my_mutex);
				add(pointer,1);
				pthread_mutex_unlock(&my_mutex);
				break;
			}
			case SPIN_LOCK:
			{
				while(__sync_lock_test_and_set(&my_spin_lock, 1));
				add(pointer,1);
				__sync_lock_release(&my_spin_lock);
				break;
			}
			case COMPARE_SWAP:
			{	
				add_compare_swap(pointer,1);
				break;
			}
		}
	}
	// second perform number of iteration -1
	for (i = 0; i < num_iter; ++i)
	{		
		switch(opt_sync)
		{
			case NO_LOCK:
			{
				add(pointer, -1);
				break;
			}
			case MUTEX:
			{
				pthread_mutex_lock(&my_mutex);
				add(pointer, -1);
				pthread_mutex_unlock(&my_mutex);
				break;
			}
			case SPIN_LOCK:
			{
				while(__sync_lock_test_and_set(&my_spin_lock, 1));
				add(pointer, -1);
				__sync_lock_release(&my_spin_lock);
				break;
			}
			case COMPARE_SWAP:
			{	
				add_compare_swap(pointer, -1);
				break;
			}
		}
	}
	return NULL;
}

int main(int argc, char** argv)
{
	long long counter = 0;

	if (atexit(clean_up) <0)
	{
		fprintf(stderr, "Atexit clean_up() error: %s\n", strerror(errno));
	}

	struct option options[] = 
	{
		{"yield", 0, NULL, 'y'},
		{"iterations", 1, NULL, 'i'},
		{"threads", 1, NULL, 't'},
		{"sync", 1, NULL, 's'}
	};

	int o;
	while((o = getopt_long(argc, argv, "i:o:sc", options, NULL)) != -1)
	{
		switch(o)
		{
			case 'y':
				opt_yield = 1;
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
						pthread_mutex_init(&my_mutex, NULL);
						break;
					case 's':
						opt_sync = SPIN_LOCK;
						break;
					case 'c':
						opt_sync = COMPARE_SWAP;
						break;
					default:
						fprintf(stderr, "usage: lab2_add [--yield] [--iterations=#] [--threads=#] [--sync=[msc]]\n");
						exit(1);
				}
				break;
			default:
				fprintf(stderr, "usage: lab2_add [--yield] [--iterations=#] [--threads=#] [--sync=[msc]]\n");
				exit(1);
		}
	}

	if(opt_yield)
	{
		switch(opt_sync)
		{
			case NO_LOCK:
				test_name = "add-yield-none";
				break;
			case MUTEX:
				test_name = "add-yield-m";
				break;
			case SPIN_LOCK:
				test_name = "add-yield-s";
				break;
			case COMPARE_SWAP:
				test_name = "add-yield-c";
				break;
		}
	}
	else
	{
		switch(opt_sync)
		{
			case NO_LOCK:
				test_name = "add-none";
				break;
			case MUTEX:
				test_name = "add-m";
				break;
			case SPIN_LOCK:
				test_name = "add-s";
				break;
			case COMPARE_SWAP:
				test_name = "add-c";
				break;
		}
	}

	pthread_t threads[num_thread];

	//long long counter = 0;

	struct timespec my_start_time;

	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &my_start_time);
	if(ret < 0)
	{
		fprintf(stderr, "Error clock_gettime(): %s\n", strerror(errno));
		exit(1);
	}

	int i;
	for (i = 0; i < num_thread; ++i)
	{
		ret = pthread_create(&threads[i], NULL, thread_function_to_run_test, &counter);
		if(ret < 0)
		{
			fprintf(stderr, "Error pthread_create(): %s\n", strerror(errno));
			exit(1);
		}
	}

	for (i = 0; i < num_thread; ++i)
	{
		ret = pthread_join(threads[i], NULL);
		if(ret < 0)
		{
			fprintf(stderr, "Error pthread_join(): %s\n", strerror(errno));
			exit(1);
		}
	}

	struct timespec my_end_time;

	ret = clock_gettime(CLOCK_MONOTONIC, &my_end_time);
	if(ret < 0)
	{
		fprintf(stderr, "Error clock_gettime(): %s\n", strerror(errno));
		exit(1);
	}

	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec)* 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	int num_ops = 2 * num_thread * num_iter;
	long long avg_run_time = my_elapsed_time_in_ns/num_ops;

	fprintf(stdout, "%s,%d,%d,%d,%lld,%lld,%lld\n", test_name, num_thread, num_iter, num_ops, my_elapsed_time_in_ns, avg_run_time, counter);

	exit(0);
}