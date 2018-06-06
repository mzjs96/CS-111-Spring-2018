/*
	NAME: MICHAEL ZHOU
	EMAIL: mzjs96@gmail.com
	UID:804663317
*/

#include <mraa.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include<time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>




#define BUFFER_SIZE 256

int AIO_PIN_1 = 1;
int GPIO_PIN_1 = 60;

int shut_down = 0;
int period = 1;
int enabled = 1;
char scale = 'F';	//default scale as Faraheight
int logfile = -1;
int is_tls= 0;
int port_no = -1;

static int socket_fd = -1;


static char ssl_buffer[BUFFER_SIZE];

static char* id = NULL;
static char* host = NULL;

const int B = 4275;                 // B value of the thermistor
const float R0 = 100000.0;            // R0 = 100k

mraa_aio_context temp_sensor;
mraa_gpio_context button;

SSL* ssl_setup(int socket_fd)
{
	SSL* ssl_client = 0;
	int ret =  SSL_library_init();
	if(ret < 0)
	{
		fprintf(stderr, "ssl_library_init() failed! Error: %s\n", strerror(errno));
		exit(1);
	}

	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	SSL_CTX* context = SSL_CTX_new(TLSv1_client_method());
	if (context == NULL)
	{
		fprintf(stderr, "SSL_CTX_new failed! Error: %s\n", strerror(errno));
		exit(1);
	}

	ssl_client = SSL_new(context);
	if (ssl_client == NULL)
	{
		fprintf(stderr, "SSL_new failed! Error: %s\n", strerror(errno));
		exit(1);
	}
	if (!SSL_set_fd(ssl_client, socket_fd))
	{
		fprintf(stderr, "SSL_set_fd failed! Error: %s\n", strerror(errno));
		exit(1);
	}
	if (SSL_connect(ssl_client) != 1) 
	{
		fprintf(stderr, "SSL_connect failed! Error: %s\n", strerror(errno));
		exit(1);
	}
	return ssl_client;
}

int socket_setup(int port_number)
{	
	int socket_fd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	socket_fd = socket (AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		fprintf(stderr, "Error: socket(). %s\n", strerror(errno));
		exit(1);
	}

	server = gethostbyname(host);
	if (server == NULL)
	{
		fprintf(stderr, "Error: gethostbyname(). %s\n", strerror(errno));
		exit(1);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port_number);

	int is_connect = -1;
	is_connect = connect(socket_fd, (struct sockaddr *) &serv_addr , sizeof(serv_addr));

	if (is_connect <0) 
	{
		fprintf(stderr, "Error connecting to the server: %s\n", strerror(errno));
		exit(1);
	}
	return socket_fd;
}

float read_temp(char scale)
{   
	int reading = mraa_aio_read(temp_sensor);

	float R = 1023.0/((float) reading) - 1.0;
	R = 100000.0*R;

	float temp_c = 1.0/(log(R/100000.0)/B + 1/298.15) - 273.15;

	float temp_f = (temp_c * 9)/5 + 32 ;

	if (scale == 'F') 
		return temp_f;
	else if(scale == 'C')
		return temp_c;
	else
	{
		fprintf(stderr, "Error scale in read temp. %s\n", strerror(errno));
		exit(2);
	}

    // This is just for the testing purpuse 
    //(without a real temperature sensor, I can hardcode the temperature value to pass sanity test)
    /*
    if (scale == 'F')
        return 70;
    else if(scale == 'C')
        return 25;
    else
        return -1;
    */
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
		period = (int)atoi(cmd+7);
	}
	else if (strcmp(cmd, "START") == 0)
	{
		enabled = 1;
	}
	else if (strcmp(cmd, "STOP") == 0)
	{
		enabled = 0;
	}
	else if (strcmp(cmd, "OFF") == 0)
	{
		shut_down = 1;
	}
	else if ((strncmp(cmd, "LOG", 3*sizeof(char))) == 0)
	{
		//if received LOG do nothing but logging
	}
	else	//if there is an invalid command
	{
		fprintf(stderr, "Error: Invalid command: %s\n", strerror(errno));
		exit(2);
	}

	if(logfile!= -1)
	{
		int ret = dprintf(logfile, "%s\n", cmd);
		if (ret < 0)
		{
			fprintf(stderr, "Error dprintf logfile: %s\n", strerror(errno));
			exit(1);
		}
	}
}

int main(int argc, char ** argv)
{
	if(strstr(argv[0], "tls") != NULL)
    {
        is_tls = 1;
    }
	char* exec_name = argv[0];

	static struct option long_options[] ={
	{"period",1, NULL,'p'},
	{"scale",1, NULL,'s'},
	{"log",1, NULL, 'l'},
	{"id",1,NULL,'i'},
	{"host",1,NULL,'h'},
	{0,0,0,0}
	};

	int o;
	while( (o = getopt_long(argc, argv, "p:s:l:", long_options, NULL)) != -1)
	{
		switch(o){
			case 'p':
				period = atoi(optarg);
				break;
			case 's':
			if(*optarg == 'C' || *optarg =='F')
			{
				scale = *optarg;
				break;
			}
			else
			{
				fprintf(stderr, "usage: %s [--period=#] [--scale=[CF]] --log=FILENAME --id=ID_NUMBER --host=HOSTNAME portnumber\n", exec_name);
				exit(1);
			}
			case 'l':
				logfile = creat(optarg, 0666);
				if (logfile < 0)
				{
					fprintf(stderr, "Error creating file descriptor: %s\n", strerror(errno));
					exit(1);
				}
				break;
			case 'i':
				id = optarg;
				if (strlen(id) != 9)
				{
					fprintf(stderr, "The id length must be 9! Error: %s\n", strerror(errno));
					exit(1);
				}
				break;
			case 'h':
				host = optarg;
				break;

			default:
				fprintf(stderr, "usage: %s [--period=#] [--scale=[CF]] --log=FILENAME --id=ID_NUMBER --host=HOSTNAME portnumber\n", exec_name);
				exit(1);
		}
	}

	if (!logfile || !host || !id)
	{
		fprintf(stderr, "usage: %s [--period=#] [--scale=[CF]] --log=FILENAME --id=ID_NUMBER --host=HOSTNAME portnumber\n", exec_name);
		exit(1);
	}

	if (optind < argc )
		//optind is less that argc means all the optargs have been processed (i am not sure about this)
	{
		port_no = (int)atoi(argv[optind]);
		if(port_no < 0)
		{
			fprintf(stderr, "usage: %s [--period=#] [--scale=[CF]] --log=FILENAME --id=ID_NUMBER --host=HOSTNAME portnumber\n", exec_name);
			exit(1);
		}
	}
	else
	{
		fprintf(stderr, "usage: %s [--period=#] [--scale=[CF]] --log=FILENAME --id=ID_NUMBER --host=HOSTNAME portnumber\n", exec_name);
		exit(1);
	}

    socket_fd = socket_setup(port_no);
	SSL* ssl_client = 0;


	if (is_tls)
	{	//TLS option setup:
		ssl_client = ssl_setup(socket_fd);
		sprintf(ssl_buffer, "ID=%s\n", id);
		SSL_write(ssl_client, ssl_buffer, strlen(ssl_buffer));
	}
	else
		//TCP option setup:
		dprintf(socket_fd, "ID=%s\n", id);

	dprintf(logfile, "ID=%s\n", id);
	
	temp_sensor = mraa_aio_init(AIO_PIN_1);
	button = mraa_gpio_init(GPIO_PIN_1);
	
	//mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted, NULL);

	struct timeval clock;
	struct tm* now;

	time_t next_due = 0;

	struct pollfd poll_fd = {socket_fd, POLLIN, 0};

	char read_buf[BUFFER_SIZE];
	char cmd_buf[BUFFER_SIZE];

	memset(read_buf, 0, BUFFER_SIZE);
	memset(cmd_buf, 0, BUFFER_SIZE);

	while(!shut_down)
	{
		/* To see what time it is.*/
		int ret = gettimeofday(&clock, NULL);
		if (ret < 0)
		{
			fprintf(stderr, "Error gettimeofday(): %s\n", strerror(errno));
		}

		if((enabled && (clock.tv_sec >= next_due)) == 1)
		{
			/*get the temperature*/
			float temp = read_temp(scale);
			int t = temp * 10;

			/*report the time and temperature*/
			now = localtime(&(clock.tv_sec));
			fprintf(stdout, "%02d:%02d:%02d %d.%1d\n", now->tm_hour, now->tm_min, now->tm_sec, t/10, t%10);

			if (is_tls)	//tls option write to ssl client
			{
				sprintf(ssl_buffer, "%02d:%02d:%02d %d.%1d\n", now->tm_hour, now->tm_min, now->tm_sec, t/10, t%10);
				SSL_write(ssl_client, ssl_buffer, strlen(ssl_buffer));
			}
			else	//tcp option write to TCP socket
				dprintf(socket_fd, "%02d:%02d:%02d %d.%1d\n", now->tm_hour, now->tm_min, now->tm_sec, t/10, t%10);

			if (logfile != -1)
			//	fputs(buffer, logfile);
				dprintf(logfile, "%02d:%02d:%02d %d.%1d\n", now->tm_hour, now->tm_min, now->tm_sec, t/10, t%10);

			/*schedule the next report*/
			next_due = clock.tv_sec + period;
		}
	

		int index = 0;

		// see if we have received a command
		poll_fd.revents = 0;
		ret = poll(&poll_fd, 1, 0);

		if(ret < 0)
		{
			fprintf(stderr, "Error poll(): %s\n", strerror(errno));
		}
		else if(ret >= 0)
		{
			if (poll_fd.revents & POLLIN)
			{
				int num_read = -1;
				if(is_tls)
					num_read = SSL_read(ssl_client, read_buf, sizeof(read_buf));
				else
					num_read = read(socket_fd, read_buf, sizeof(read_buf));
				
				if(num_read <0)
				{
					fprintf(stderr, "Error read(): %s\n", strerror(errno));
					exit(1);
				}

				for (int i = 0; i < num_read && index < BUFFER_SIZE; i++)
				{
					if(read_buf[i] == '\n')
					{
						do_command(cmd_buf);
						index = 0;
						memset(cmd_buf, 0, BUFFER_SIZE);
					}
					else
					{
						cmd_buf[index] = read_buf[i];
						index++;
					}
				}
			}
		}
	}
	/*	//see if we have a button-push
	if (mraa_gpio_read(button))
	{
		shutdown = 1;
	}
	*/

	/*log the shut-down message*/
	now = localtime(&(clock.tv_sec));

	fprintf(stdout, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
	if (is_tls)
	{
		sprintf(ssl_buffer, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);
		SSL_write(ssl_client, ssl_buffer, strlen(ssl_buffer));
	}
	else
		dprintf(socket_fd, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);

	if (logfile != -1)
		dprintf(logfile, "%02d:%02d:%02d SHUTDOWN\n", now->tm_hour, now->tm_min, now->tm_sec);

	//shutdown
	mraa_aio_close(temp_sensor);
	mraa_gpio_close(button);
	exit(0);
}
