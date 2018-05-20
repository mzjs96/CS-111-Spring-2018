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
#define BUFFER_SIZE = 256;

int shutdown = 0;
int period = 0;
int enabled = 0;
char scale = 'F';	//default scale as Faraheight
int logfile = -1;

const int B = 4275;                 // B value of the thermistor
const float R0 = 100000.0;            // R0 = 100k


void do_when_interrupted()
{
	shutdown = 1;
}

void read_temp(char scale)
{
	int reading = mraa_aio_read(temp_sensor);

	float R = 1023.0/((float) reading) - 1.0;
	R = 100000.0*R

	float temp_c = 1.0/(log(R/100000.0)/B + 1/298.15) - 273.15;
	float temp_f = (temp_c * 9)/5 + 32 ;

	if (scale == 'F') 
		return temp_f;
	else if(scale == 'C')
		return temp_c;
}

void do_command(char* cmd)
{
	if((strncmp(cmd, "SCALE=", 6*sizeof(char))) == 0)
	{
		if (cmd[6] == 'C')
			scale = 'C';
		else if(cmd[6] == 'F')
			scale = 'F';
		else
		{
			fprintf(stderr, "Error: Invalid command: %s\n", strerror(errno));
			exit(1);
		}
	}
	else if((strncmp(cmd, "PERIOD=", 7*sizeof(char))) == 0)
	{
		period = (int)atoi(cmd[7]);
	}
	else if (strcmp(cmd, "START"))
	{
		enabled = 1;
	}
	else if (strcmp(cmd, "STOP"))
	{
		enabled = 0;
	}
	else if ((strncmp(cmd, "LOG", 3*sizeof(char))) == 0)
	{
		if(logfile == 1)
		{
			int ret = dprintf(logfile, "%s\n", cmd);
			if (ret < 0)
			{
				fprintf(stderr, "Error dprintf logfile: %s\n", strerror(errno));
				exit(1);
			}
		}
	}
	else	//if there is an invalid command
	{
		fprintf(stderr, "Error: Invalid command: %s\n", strerror(errno));
		exit(1);
	}
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
				period = atoi(optarg);
				break;
			case 's':
				scale = optarg;
				break;
			case 'l':
				logfile = creat(optarg, 0666);
				if (logfile < 0)
				{
					fprintf(stderr, "Error creating file descriptor: %s\n", strerror(errno));
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

	time_t next_due = 0;

	struct pollin poll_in = {0, POLLIN, 0};

	char read_buf[BUFFER_SIZE];
	char cmd_buf[BUFFER_SIZE];

	memset(read_buf, 0, BUFFER_SIZE);
	memset(cmd_buf, 0, BUFFER_SIZE);

	whie(!shutdown)
	{
		/* To see what time it is.*/
		gettimeologfileay(&clock, 0);

		if( enabled && clock.tv_sec >= next_due)
		{
			/*get the temperature*/
			float temp = read_temp(scale);
			int t = temp * 10;

			/*report the time and temperature*/
			now = localtime(&(clock.tv_sec));
			sprintf(outbuf, "%02d:%02d:%02d %d.%1d\n", now->tm_hour, now->tm_min, now->tm_sec, t/10, t%10);
			fputs(outbuf, stdout);
			if (logfile)
				fpots(outbuf, logfile);
			/*schedule the next report*/
			next_due = clock.tv_sec + period;
		}
	}
	// see if we have received a command
	poll_in.revents = 0;
	int ret = poll(&poll_in, 1, SLEEP);

	int index = 0;

	if(ret < 0)
	{
		fprintf(stderr, "Error poll(): %s\n", strerror(errno));
	}
	else if(ret > 0)
	{
		if (poll_in.revents & POLLIN)
		{
			int num_read = read(0, read_buf, sizeof(read_buf));
			if(num_read <0)
			{
				fprintf(stderr, "Error read(): %s\n", strerror(errno));
			}
			for (int i = 0; i < num_read && index < BUFFER_SIZE; i++)
			{
				if(read_buf[i] == '\n')
					do_command(cmd_buf);
					index = 0;
					memset(cmd_buf, 0, BUFFER_SIZE);
				else
					cmd_buf[index] = read_buf[i];
					index++;
			}
		}
	}
	/*	//see if we have a button-push
	if (mraa_gpio_read(button))
	{
		shutdown = 1;
	}
	*/
	//log the shut-down message
	now = localtime(&(clock.tv_sec));
	//sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
	//int fputs(const char * str, FILE * stream);
	//fputs (outbuf, logfile);
	dprintf(logfile, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);

	//shutdown
	mraa_aio_close(temp_sensor);
	mraa_gpio_close(button);
	exit(0);
}








