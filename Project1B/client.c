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


struct termios saved_attributes;

const char *HOSTNAME = "localhost";

struct pollfd pollFd [2];


/*
	Reference of socket tutorial is found on website:
	http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
*/

int socketSetup(int portNumber)
{

	struct sockaddr_in serv_addr;
	struct hostent *server;

	//server = gethostbyname(argv[1]);

	int socket_Fd = socket (AF_INET, SOCK_STREAM, 0);
	if (socket_Fd<0)
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

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portNumber);

    if (connect(socket_Fd,&serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Error connecting to the server.%s\n", strerror(errno));
		exit(1);
	}
	return socket_Fd;
}

void waitForKeyboardInput()
{

}


void restore_normal_mode(void)
{

	int ret = tcsetattr(0, TCSANOW, &saved_attributes);
	if(ret != 0){
		fprintf(stderr, "tcsetattr error: %s", strerror(errno));
    	exit(1);
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
	int portFlag = 0;
	int logFlag = 0;
	int portNumber = 0;
	char* logFile = NULL;
	int opt;
	int option_index = 0;

	int socketFd = 0;
	int newSocketFd = 0;
	char buffer[256];


	static struct option long_options[] =
	{
		{"port",required_argument,0,'p'},
		{"option",required_argument,0,'l'},

		{0,0,0,0}
	};
	/* getopt_long stores the option index here. */

    while(1)
    {	
    	opt = getopt_long(argc, argv, "p::l::", long_options, &option_index);
		
	    /* Detect the end of the options. */

		if (opt <= -1)	//if there is no argument			
			break;

	    switch (opt) 
	    {
			case 'p':
				portFlag = 1;
				portNumber = atoi(optarg);
			  	break;
	        case 'l':
	        	logFlag = 1;
		  		int logFd = creat(optarg, 0666);
		  		if (logFd < 0 ){
		  			fprintf(stderr, "Error creat() when loading log file. %s\n", strerror(errno));
		  		}
		  		break;
			default: 
		  		fprintf(stderr, "Usage: --port=port# --log=filename \n");
		 		exit(1);
		}
	}

	if (portFlag == 0)
		{
			fprintf(stderr, "Error. --port=port# is mandatory%s\n", strerror(errno));
			exit(1);
		}


	socketFd = socketSetup(portNumber);

	socketFd = socket (AF_INET, SOCK_STREAM, 0);
	if (socketFd<0)
	{
		fprintf(stderr, "Error socket(). %s\n", strerror(errno));
		exit(1);
	}

	printf("Socket create success./n");

	waitForKeyboardInput();

	exit (0);

}
