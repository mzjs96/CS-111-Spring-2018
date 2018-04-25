/*
NAME: MICHAEL ZHOU
EMAIL: mzjs96@gmail.com
ID: 804663317
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <zlib.h>

const int BUFFER_SIZE = 2048;

int to_child_pipe[2];
int from_child_pipe[2];
pid_t child_pid = -1;
int socket_fd = -1;
struct pollfd poll_fd[2];

int compression_flag = 0;
int port_flag = 0;

z_stream to_client;
z_stream from_client;

void reset() {

	shutdown(socket_fd, SHUT_RDWR);

	close(socket_fd);
	
	int status;
	if (waitpid(child_pid, &status, 0) < 0) {
		fprintf(stderr, "Error: with waitpid. %s\r\n", strerror(errno));
		exit(1);
	}
	int p_status = 0;
	int p_signal = 0;

	if (WIFEXITED(status)) {
		p_status = WEXITSTATUS(status);
	}
	if (WIFSIGNALED(status)) {
		p_signal = WTERMSIG(status);
	}
	fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d\r\n", p_signal, p_status);
}

void handler (int signum)
{	
	if (signum == SIGPIPE)
	{
		fprintf(stderr, "Caught Segmentation Fault with signal number %d\n", signum);
		printf("Error:%s\n", strerror(errno));
		printf("shell end SIGPIPE\n");
		exit(0);
	}
}

void cleanup_compression() {
        deflateEnd(&to_client);
        inflateEnd(&from_client);
}

void setup_compression() {
    if (atexit (cleanup_compression) == -1) {
    	fprintf(stderr, "atexit() error when cleanup_compression. Error: %s\n", strerror(errno));
    	exit(1);
    }
    to_client.zalloc = Z_NULL;
    to_client.zfree = Z_NULL;
    to_client.opaque = Z_NULL;
    if (deflateInit(&to_client, Z_DEFAULT_COMPRESSION) != Z_OK) {
            fprintf(stderr, "deflateInit(): %s\n", to_client.msg);
            exit(1);
    }

    from_client.zalloc = Z_NULL;
    from_client.zfree = Z_NULL;
    from_client.opaque = Z_NULL;
    if (inflateInit(&from_client) != Z_OK) {
            fprintf(stderr, "inflateInit(): %s\n", from_client.msg);
            exit(1);
    }
}

int compression(char* buf, int read_bytes) {
	int num_bytes;
	char temp_buf[BUFFER_SIZE];
	memcpy(temp_buf, buf, read_bytes);

	to_client.avail_in = read_bytes;
	to_client.next_in = (Bytef *) temp_buf;
	to_client.avail_out = BUFFER_SIZE;
	to_client.next_out = (Bytef *) buf;

	do {
		deflate(&to_client, Z_SYNC_FLUSH);
	}while(to_client.avail_in > 0);

	num_bytes = BUFFER_SIZE - to_client.avail_out;
	return num_bytes;
}

int decompression(char* buf, int read_bytes) {
	int num_bytes;
	char temp_buf[BUFFER_SIZE];
	memcpy(temp_buf, buf, read_bytes);

	from_client.avail_in = read_bytes;
	from_client.next_in = (Bytef *) temp_buf;
	from_client.avail_out = BUFFER_SIZE;
	from_client.next_out = (Bytef *) buf;

	do {
        inflate(&from_client, Z_SYNC_FLUSH);
	} while (from_client.avail_in > 0);

	num_bytes = BUFFER_SIZE - from_client.avail_out;
	return num_bytes;
}


void client_shell_listener()
{
	// setup and fork
	///////////////////////////////////////////////////////////////////////////
	int read_bytes = -1;
	int write_bytes = -1;
	int ret = -1;
	char buf_server[BUFFER_SIZE];
	char buf_shell[BUFFER_SIZE];

	ret = pipe(from_child_pipe);
	if (ret < 0 )
	{
		fprintf(stderr, "Error pipe(from_child_pipe): %s\n", strerror(errno));
		exit(1);
	}

	ret = pipe(to_child_pipe);
	if (ret < 0 )
	{
		fprintf(stderr, "Error pipe(from_child_pipe): %s\n", strerror(errno));
		exit(1);
	}

	child_pid = fork();

	if(child_pid < 0 )
	{
		fprintf(stderr, "fork() failed! Error:%s\n", strerror(errno));
		exit(1);
	}

	else if (child_pid > 0 ) //if this is the parent
	{
		ret = close(to_child_pipe[0]);
		if (ret == -1 )
		{
			fprintf(stderr, "Error close(to_child_pipe[0]): %s\n", strerror(errno));
			exit(1);
		}
		ret = close(from_child_pipe[1]);
		if (ret == -1)
		{
			fprintf(stderr, "Error close(from_child_pipe[1]): %s\n", strerror(errno));
			exit(1);

		}

		poll_fd[0].fd = socket_fd;
		poll_fd[0].events = POLLIN | POLLHUP | POLLERR;

		poll_fd[1].fd = from_child_pipe[0];
		poll_fd[1].events = POLLIN | POLLHUP | POLLERR;

		if(signal(SIGPIPE, handler) == SIG_ERR)
		{
			fprintf(stderr, "signal () Error: %s\n", strerror(errno));
		}

		if(atexit(reset) == -1)
		{
			fprintf(stderr, "atexit(reset) Error: %s\n", strerror(errno));
		}

		while(1)
		{
			ret = poll(poll_fd, 2, 0);
			if(ret <0)
			{
				fprintf(stderr, "Error poll(): %s\n", strerror(errno));
				exit(1);
			}
			else if (ret >0)
			{
				if(poll_fd[0].revents & POLLIN)
				{
					read_bytes = read (socket_fd, buf_server, BUFFER_SIZE);
					if (read_bytes < 0)
					{
						fprintf(stderr, "Error read(): %s\n", strerror(errno));
						exit(1);
					}

					if (compression_flag)
					{
						read_bytes = decompression(buf_server, read_bytes);
					}

					int i;
					char c;
					for(i = 0 ; i< read_bytes; i++)
					{
						
						c = buf_server[i];

						if (c == '\x04')	//receive the end of file (^D) from keyboard
						{
							ret = close(to_child_pipe[0]);		//shell POLLHUP will be set
							if (ret == -1 )
							{
								fprintf(stderr, "Error close(to_child_pipe[1]): %s\n", strerror(errno));
								exit(1);
							}
						}
						else if (c == '\x03') //receiving ^C
						{
							kill(child_pid, SIGINT);	//kill the shell
						}
						else if ( (c == '\r') || (c == '\n'))	//receive new line from keyboard;
						{
							write_bytes = write(to_child_pipe[1], "\n", 1);
							if ( write_bytes < 0 )
							{
								fprintf(stderr, "Error write() to shell: %s\n", strerror(errno));
								exit(1);
							}
						}
						else		//normal input
						{
							write_bytes = write(to_child_pipe[1], &c, 1);
							if ( write_bytes < 0 )
							{
								fprintf(stderr, "Error write() to shell: %s\n", strerror(errno));
								exit(1);
							}
						}
					}
				}

				if(poll_fd[1].revents & POLLIN)	//from shell input
				{
					read_bytes = read(from_child_pipe[0], buf_shell, BUFFER_SIZE);
					if(read_bytes <0)
					{
						fprintf(stderr, "Error read(): %s\n", strerror(errno));
						exit(1);
					}
					char c;

					for (int i = 0; i< read_bytes; i++)
					{
						c = buf_shell[i];
						if (c == '\x04')
						{
							exit(0);
						}
					}

					if (compression_flag)
					{
						read_bytes = compression(buf_shell, read_bytes);
					}

					write_bytes = write(socket_fd, buf_shell, read_bytes);
					if (write_bytes < 0)
					{
						fprintf(stderr, "Error write(): %s\n", strerror(errno));
						exit(1);
					}
				}

				if (poll_fd[0].revents & (POLLERR | POLLHUP) || poll_fd[1].revents & (POLLERR | POLLHUP)) 
				{
					exit(0);
				}

			}
		}

	}

	else if (child_pid == 0)	//if this is the child
	{
		ret = close(to_child_pipe[1]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}
		ret = close(from_child_pipe[0]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = close(socket_fd);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() socket failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = dup2(to_child_pipe[0], STDIN_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = close(to_child_pipe[0]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = dup2(from_child_pipe[1], STDOUT_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = dup2(from_child_pipe[1], STDERR_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error dup2() failed: %s\n", strerror(errno));
			exit(1);
		}

		ret = close(from_child_pipe[1]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror(errno));
			exit(1);
		}
		
		char *execvp_argv[2];
		char execvp_filename[] = "/bin/bash";
		execvp_argv[0] = execvp_filename;
		execvp_argv[1] = NULL;

		if (execvp(execvp_filename, execvp_argv) == -1)
		{
			fprintf(stderr, "execvp() failed Error: %s\n", strerror(errno));
			exit(1);
		}
	}	
}


int socket_setup (int port_no) {

	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port_no);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	if (newsockfd < 0) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}

	return newsockfd;
}

int main(int argc, char** argv)
{
	int opt;
	int port_number = -1;
	int option_index;

	static struct option long_options[] =
	{
		{"port",required_argument,0,'p'},
		{"compress", no_argument,0,'c'},
		{0,0,0,0}
	};

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

		  	case 'c':
		  		compression_flag = 1;
		  		setup_compression();
		  		break;
			default: 
		  		fprintf(stderr, "Usage: --port=port#\n");
		 		exit(1);
		}
	}

	if (port_number < 0) 
	{
		fprintf(stderr, "usage: lab1b-server --port=PORTNUMBER [--compress]\r\n");
		exit(1);
	}

	socket_fd = socket_setup (port_number);
	client_shell_listener();

	exit(0);

}

