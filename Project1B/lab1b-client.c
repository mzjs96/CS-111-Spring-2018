/*
NAME: MICHAEL ZHOU
EMAIL: mzjs96@gmail.com
ID: 804663317
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>			//used for atexit()
#include <termios.h>		//used for terminal set_input_mode
#include <getopt.h>			//used for getopt_long()
#include <fcntl.h>			//used for pipe()
#include <errno.h>			//used for errno
#include <signal.h>			//used for SIGINT and SIGPIPE
#include <string.h>			//used for strerror()
#include <poll.h>			//used for poll()
#include <sys/types.h>		//used for fork()
#include <sys/wait.h>		//used for waitpid()


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <zlib.h>


struct termios saved_attributes;

const char *HOSTNAME = "localhost";
const int BUFFER_SIZE = 2048;
int compression_flag = 0;
int log_flag = 0;
int log_fd = -1;
int socket_fd = -1;


z_stream to_shell;
z_stream from_shell;

/*
	Reference of socket tutorial is found on website:
	http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
*/

void cleanup_compression() 
{
	deflateEnd(&to_shell);
	inflateEnd(&from_shell);
}

void setup_compression() 
{
	if (atexit (cleanup_compression) == -1) {
		fprintf(stderr, "compresion atexit() Failed! Error: %s\n", strerror(errno));
	}

	to_shell.zalloc = Z_NULL;
	to_shell.zfree = Z_NULL;
	to_shell.opaque = Z_NULL;
	if (deflateInit(&to_shell, Z_DEFAULT_COMPRESSION) != Z_OK) {
		fprintf(stderr, "deflateInit() failed! Error: %s\n", to_shell.msg);
		exit(1);
	}
	
	from_shell.zalloc = Z_NULL;
	from_shell.zfree = Z_NULL;
	from_shell.opaque = Z_NULL;
	if (inflateInit(&from_shell) != Z_OK) 
	{
		fprintf(stderr, "inflateInit() failed! Error: %s\n", from_shell.msg);
		exit(1);
	}
}

int compression(char* buf, int read_bytes) {
	int num_bytes;
	char temp_buf[BUFFER_SIZE];
	memcpy(temp_buf, buf, read_bytes);

	to_shell.avail_in = read_bytes;
	to_shell.next_in = (Bytef *) temp_buf;
	to_shell.avail_out = BUFFER_SIZE;
	to_shell.next_out = (Bytef *) buf;

	do {
		deflate(&to_shell, Z_SYNC_FLUSH);
	}while(to_shell.avail_in > 0);

	num_bytes = BUFFER_SIZE - to_shell.avail_out;
	return num_bytes;
}

int decompression(char* buf, int read_bytes) {
	int num_bytes;
	char temp_buf[BUFFER_SIZE];
        memcpy(temp_buf, buf, read_bytes);

        from_shell.avail_in = read_bytes;
        from_shell.next_in = (Bytef *) temp_buf;
        from_shell.avail_out = BUFFER_SIZE;
        from_shell.next_out = (Bytef *) buf;

        do {
                inflate(&from_shell, Z_SYNC_FLUSH);
        } while (from_shell.avail_in > 0);

        num_bytes = BUFFER_SIZE - from_shell.avail_out;
	return num_bytes;
}

void restore_normal_mode(void)
{
	int ret = tcsetattr(0, TCSANOW, &saved_attributes);
	if(ret != 0){
		fprintf(stderr, "tcsetattr error: %s", strerror(errno));
    	exit(1);
	}
}

int setup_socket(int port_number)
{	
	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	//server = gethostbyname(argv[1]);

	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		fprintf(stderr, "Error: socket(). %s\n", strerror(errno));
		exit(1);
	}

	server = gethostbyname(HOSTNAME);
	if (server == NULL)
	{
		fprintf(stderr, "Error: gethostbyname(). %s\n", strerror(errno));
		exit(1);
	}

	//TA ZHAOXING BU told us to use memset and memcpy

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port_number);

   	int is_connect = -1;
	is_connect = connect(sockfd, (struct sockaddr *) &serv_addr , sizeof(serv_addr));

    if (is_connect <0) 
    {
		fprintf(stderr, "Error connecting to the server: %s\n", strerror(errno));
		exit(1);
	}
	return sockfd;
}

void keyboard_input_listener()
{	
	struct pollfd poll_fd[2];

	poll_fd[0].fd = STDIN_FILENO;	//Set to STDIN
	poll_fd[0].events = POLLIN;		//there is data to be read
	poll_fd[1].fd = socket_fd;		//reads from shell
	poll_fd[1].events = POLLIN;

	char buf[BUFFER_SIZE];
	int read_bytes = 0;
	int write_bytes = 0;
	//int shut_down = 0;

	while(1)
	{
		int ret = poll(poll_fd, 2, 0);
		/* int poll(struct pollfd *fds, nfds_t nfds, int timeout); */

		if (ret < 0)
		{
			fprintf(stderr, "poll() failed. Error: %s\n", strerror(errno));
			restore_normal_mode();
			exit(1);
		}
		else if (ret > 0)
		{
			if(poll_fd[0].revents & POLLIN)	//if there is returned events and there is also data to be read
			{
				read_bytes = read(STDIN_FILENO, buf, BUFFER_SIZE);
				
				if (read_bytes < 0)
				{
					fprintf(stderr, "read() failed. Error: %s\n", strerror(errno));
					exit(1);
				}

				int i;
				char c;
				for (i = 0; i < read_bytes; i++)
				{
					c = buf[i];
					if((c == '\r') || (c == '\n'))	//receive new line from keyboard
					{
						write_bytes = write(STDOUT_FILENO, "\r\n", 2);
						if(write_bytes < 0)
						{
							fprintf(stderr, "write() failed. Error: %s\n", strerror(errno));
							exit(1);
						}
					}
					else //normal input
					{
						write_bytes = write(STDOUT_FILENO, &c, 1);
						if(write_bytes < 0)
						{
							fprintf(stderr, "write() failed. Error: %s\n", strerror(errno));
							exit(1);
						}
					}
				}

				if(compression_flag)
				{
					read_bytes = compression(buf, read_bytes);
				}

				write_bytes = write(socket_fd, buf, read_bytes);
				if (write_bytes < 0)
				{
					fprintf(stderr, "write() failed! Error: %s\n", strerror(errno));
					exit(1);
				}

				if ( log_flag == 1) {

					if (dprintf(log_fd, "SENT %d bytes: ", read_bytes) < 0) 
					{
						fprintf(stderr, "log dprintf() failed! Error: %s\n", strerror(errno));
						exit(1);
					}
					if (write(log_fd, buf, read_bytes) < 0) {
						fprintf(stderr, "log writef() failed! Error: %s\n", strerror(errno));
						exit(1);
					} 
					if (dprintf(log_fd, "\n") < 0) {
						fprintf(stderr, "log dprintf() failed! Error: %s\n", strerror(errno));
						exit(1);
					}
				}
			}

			if(poll_fd[1].revents & POLLIN) 
			{
				int read_bytes = read(socket_fd, buf, BUFFER_SIZE);	
				if(read_bytes < 0)
				{
					fprintf(stderr, "read() failed! Error:%s\n", strerror(errno));
					exit(1);
				}
				else if (read_bytes == 0)
				{
					restore_normal_mode();
					exit(0);
				}
				else //read_bytes > 0
				{	
					if(log_flag == 1)
					{
						if (dprintf(log_fd, "RECEIVED %d bytes: ", read_bytes) < 0) 
						{
							fprintf(stderr, "Log dprintf() failed! Error: %s\n", strerror(errno));
							exit(1);
						}
						if (write(log_fd, buf, read_bytes) < 0) 
						{
							fprintf(stderr, "Log writef() failed! Error: %s\n", strerror(errno));
							exit(1);
						}
						if (dprintf(log_fd, "\n") < 0) 
						{
							fprintf(stderr, "Log dprintf() failed! Error: %s\n", strerror(errno));
							exit(1);
						}
					}

					if (compression_flag) 
					{
						read_bytes = decompression(buf, read_bytes);
					}

					int i;
					char c;
					for (i = 0; i < read_bytes; ++i)
					{
						c = buf[i];
						if (c == '\n')
						{
							write_bytes = write(STDOUT_FILENO, "\r\n", 2);
							if( write_bytes < 0 )
							{
								fprintf(stderr, "client write() failed! Error: %s\n", strerror(errno));
								exit(1);
							}
						}
						else
						{	
							write_bytes = write(STDOUT_FILENO, &c, 1 );
							if( write_bytes < 0 )
							{
								fprintf(stderr, "client write() failed! Error: %s\n", strerror(errno));
								exit(1);
							}
						}
					}
				}
			}
			if ( (poll_fd[0].revents & (POLLERR | POLLHUP)) || (poll_fd[1].revents & (POLLERR | POLLHUP)) ) 
			{
				restore_normal_mode();
				exit(0);
			}
		}
	}
}

void set_input_mode (void)
{	
	/* this process makes sure stdin is a terminal. */
	if(!isatty(STDIN_FILENO))
	{
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	 //Get the normal mode attributes and restore after exit
	//Save the terminal attributes so we can restore them later. 
	tcgetattr(0, &saved_attributes);
	atexit(restore_normal_mode);

	struct termios tattr;

	tcgetattr (0, &tattr);

	tattr.c_iflag = ISTRIP;	//only lower 7 bits
	tattr.c_oflag = 0; //no processing
	tattr.c_lflag = 0; //no processing

	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (0, TCSANOW, &tattr);
	//all input changes are effective immediately
}



int main(int argc, char ** argv)
{
	int port_flag = 0;
	int port_number = 0;
	//char* log_file = NULL;
	int opt;
	int option_index = 0;

	//int socket_fd = -1;
	//int new_socket_fd = 0;


	static struct option long_options[] =
	{
		{"port",required_argument,0,'p'},
		{"log",required_argument,0,'l'},
		{"compress", no_argument,0,'c'},
		{0,0,0,0}
	};
	/* getopt_long stores the option index here. */

    while(1)
    {	
    	opt = getopt_long(argc, argv, "", long_options, &option_index);
		
	    /* Detect the end of the options. */

		if (opt <= -1)	//if there is no argument			
			break;

	    switch (opt) 
	    {
			case 'p':
			 port_flag = 1;
				port_number = atoi(optarg);
			  	break;
	        case 'l':	  			
	        	log_flag = 1;
		  		log_fd = creat(optarg, 0666);
			if (log_fd <0){
				fprintf(stderr,"Invalid output file name %s. Error: %s\n",optarg,strerror(errno));
			}
		  		break;
		  	case 'c':
		  		compression_flag = 1;
		  		setup_compression();
		  		break;
			default: 
		  		fprintf(stderr, "Usage: --port=port# --log=filename \n");
		 		exit(1);
		}
	}

	if (port_flag == 0)
	{
		fprintf(stderr, "Error: --port=port# is mandatory: %s\n", strerror(errno));
		exit(1);
	}

	socket_fd = setup_socket(port_number);

	set_input_mode();

	keyboard_input_listener();

	exit (0);

}
