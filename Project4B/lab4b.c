#include <mraa.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/type.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#define AIO_PIN_1 = 1;
#define GPIO_PIN_1 = 60;

int shutdown = 0;
char scale = 'F';	//default scale as Faraheight
int fd = 0;

void do_when_interrupted()
{
	shutdown = 1;
}

int main (int argc, char ** argv)
{
	static struct option long_options ={
	{"period",1, NULL,'p'},
	{"scale",1, NULL,'s'},
	{"log",1, NULL, 'l'},
	{0,0,0,0}
	}
	int o
	while( (o = getopt_long(argc, argv, "p:s:l:", long_options, NULL)) != -1)
	{
		switch(0){
			case 'p':
				period = atoi (optarg);
				break;
			case 's':
				scale = optarg;
				break;
			case 'l':
				fd = creat(optarg, 0666);
				if (fd < 0)
				{
					fprintf(stderr, "Error creating file descriptor: %s\n", strerr(errno));
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "usage: lab4b [--period=#] [--scale=[CF]] [--log=FILENAME]\n");
				exit(1);
		}
	}

	mraa_aio_context temp_sensor;
	mraa_gpio_context button;
	temp_sensor = mraa_aio_init(AIO_PIN_1);
	button = mraa_gpio_init(GPIO_PIN_1);
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted, NULL);

	struct timeval clock
	struct tm* now;

	time_t next_sample = 0;

	struct pollfd poll_fd = {0, POLLIN, 0};


}







