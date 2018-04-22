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


void handler (int signum)
{
	if (signum == SIGPIPE)
	{
		fprintf(stderr, "Caught Segmentation Fault with signal number %d\n", signum);
		printf("Error:%s\n", strerror(errno));
		printf("shell end SIGPIPE\n");
		//restore_normal_mode();
	}
	exit(0);
}


int socket_setup(int port_number)
{
	int new_socket_fd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
	{
		fprintf(stderr, "Error creating socket(): %s\n", strerror(errno));
		exit(1);

	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port_no);

	if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		fprintf(stderr, "Error bind(): %s\n", strerror(errno));
		exit(1);
	}

	listen(sock_fd,5);

	clilen = sizeof(cli_addr);
	new_socket_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &clilen);
	if (new_socket_fd < 0) 
	{
		fprintf(stderr, "Error accept(): %s\n", strerror(errno));
		exit(1);
	}
	return new_socket_fd;
}

void client_shell_listener(int new_socket_fd)
{
	// setup and fork
	////////////////////////////////////////////////////////////////////////////

	int ret;
	char buf_server[BUFFER_SIZE];
	char buf_shell[BUFFER_SIZE];

	ret = pipe(from_child_pipe);
	if (ret < 0 )
	{
		fprintf(stderr, "Error pipe(from_child_pipe): %s\n", strerror(errno));
		exit(1);
	}

	ret pipe(to_child_pipe);
	if (ret < 0 )
	{
		fprintf(stderr, "Error pipe(from_child_pipe): %s\n", strerror(errno));
		exit(1);
	}

	child_pid = fork()

	//Child
	////////////////////////////////////////////////////////////////////////////
	if(child_pid < 0 )
	{
		fprintf(stderr, "Error fork()%s\n", strerror());
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
		poll_fd[0].events = POLLIN;

		poll_fd[1].fd = from_child_pipe[0];
		poll_fd[1].events = POLLIN;

		int read_bytes = 0;
		int write_bytes = 0;

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
				if(poll_fd[0].revent & POLLIN)
				{
					read_bytes = read (new_socket_fd, buf_server, BUFFER_SIZE);
					if (read_bytes < 0)
					{
						fprintf(stderr, "Error read(): %s\n", strerror);
						exit(1);
					}
					int i;
					char c;
					for(i =0 ; i< read_bytes, i++)
					{
						c = buf_server[i];
						if (c == '\x04')	//receive the end of file (^D) from keyboard
						{
							ret = close(to_child_pipe[0]);
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
							write_bytes = write(to_child_pipe[1], '\n', 1);
							if ( write_bytes < 0 )
							{
								fprintf(stderr, "Error write() to shell: %s\n", strerror(errno));
							}
						}
						else		//normal input
						{
							write_bytes = write(to_child_pipe[1], &c, 1);
							if ( write_bytes < 0 )
							{
								fprintf(stderr, "Error write() to shell: %s\n", strerror(errno));
							}
						}
					}
				}

				if(poll_fd[1].revent & POLLIN)	//from shell input
				{
					read_bytes = read(new_socket_fd, buf_shell, BUFFER_SIZE);
					if(read_bytes <0)
					{
						fprintf(stderr, "Error read(): %s\n", strerror(errno));
						exit(0);
					}
					int i;
					char c;
					for (i = 0; i < count; ++i)
					{
						c = buf_shell[i];
						write(new_socket_fd, &c, 1);
					}
				}

				if( (poll_fd[0].revent & POLLHUP) || ( poll_fd[0].revent & POLLERR))
				{
					ret = close(from_child_pipe[0]);
					if ( ret == -1 )
					{
						fprintf(stderr, "Error close(to_child_pipe[0]): %s\n", strerror(errno));
						exit(1);
					}
					exit(0);
				}

				if( (poll_fd[1].revent & POLLHUP) || ( poll_fd[1].revent & POLLERR))
				{
					wait_pid = waitpid(child_pid, &status, 0);
					if(wait_pid == -1)
					{
						fprintf(stderr, "Waitpid failed. Error: %s\n", strerror(errno));
						exit(1);
					}

					int pstatus = 0;
					int psignal = 0;

					if(WIFEXITED(status))
					{
						pstatus = WEXITSTATUS(status);
					}
					if (WIFSIGNALED(status))
					{
						psignal = WTERMSIG(status);
					}

					fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", psignal, pstatus);
					exit(0);
				}
			}
		}
	}
	else //if this is the child
	{
		ret = close(to_child_pipe[1]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
			exit(1);
		}
		ret = close(from_child_pipe[0]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
			exit(1);
		}
		ret = dup2(to_child_pipe[0], STDIN_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
			exit(1);
		}
		ret = dup2(from_child_pipe[1], STDOUT_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
			exit(1);
		}
		ret = dup2(from_child_pipe[1], STDERR_FILENO);
		if (ret < 0)
		{
			fprintf(stderr, "Error dup2() failed: %s\n", strerror);
			exit(1);
		}
		ret = close(to_child_pipe[0]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
			exit(1);
		}
		ret = close(from_child_pipe[1]);
		if (ret < 0)
		{
			fprintf(stderr, "Error close() failed: %s\n", strerror);
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

void cleanup_compression() {
        deflateEnd(&to_client);
        inflateEnd(&from_client);
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


int main(int argc, char** argv)
{
	int port_number = 0;
	int option_index = 0;
	int socket_fd;

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
			  /*
	        case 'l':	  			
	        	log_flag = 1;
		  		int log_fd = creat(optarg, 0666);
		  		if (log_fd < 0 )
		  		{
		  			fprintf(stderr, "Error creat() when loading log file. %s\n", strerror(errno));
		  		}
		  		break;
		  	*/
		  	case 'c':
		  		compression_flag = 1;
		  		break;
			default: 
		  		fprintf(stderr, "Usage: --port=port# --log=filename \n");
		 		exit(1);
		}
	}

	socket_fd = socket_setup(port_number);


	if (socket_fd >= 0)
	{
		printf("Socekt connect success!\n");
	}

	client_shell_listener(socket_fd);
	exit(0);

}