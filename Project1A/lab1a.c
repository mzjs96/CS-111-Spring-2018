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

/*
	Non-canonocal input form referenced from: 
	https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
*/

struct termios saved_attributes;

/*Save the normal terminal attributes*/

void restore_normal_mode(void)
{

	int ret = tcsetattr(0, TCSANOW, &saved_attributes);
	if(ret != 0){
		fprintf(stderr, "tcsetattr error: %s", strerror(errno));
    	exit(1);
	}
	//tcsetattr - set the parameters associated with the terminal
	//TCSAFLUSH: the change shall occur after all output written to fildes is transmitted
	//
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

//To handle whether the pipe is broken
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

int main (int argc, char** argv)
{
		static struct option long_options[] = 
	{
		{"shell", no_argument, NULL, 's'},
	//	{"debug", no_argument, NULL, 'd'},
		{0,0,0,0}
	};

	int option_index = 0;
	int is_shell = 0;
	//	int debug_mode = 0;

	while(1)
	{
		int opt = getopt_long(argc, argv, "", long_options, &option_index);

		if(opt == -1)
			break;
		switch(opt)
		{
			case 's':
			is_shell = 1;
			break;

	//		case 'd':
	//		debug_mode = 1;
	//		break;

			default:
			printf ("Unrecognized use: lab1a [--shell]\n");
			fprintf (stderr, "getopt_long generated unrecognized character code: %d\n", opt);
			exit(1);
			break;
		}
	}

	set_input_mode();

	if (is_shell == 0) //not shell mode
	{
		char buf[256];
		int read_bytes = 0;
		int write_bytes = 0;
		int shut_down = 0;

		while(1){
			read_bytes = read(0, buf, 256);	// read from stdin keyboard input.

			if(read_bytes < 0)
			{
				fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
					exit(1);
			}

			int i;
			char c;
			for (i = 0; i < read_bytes; i++)
			{
				c = buf[i];
				if( c == '\x04' ) //receive EOF 
				{
					shut_down = 1;
					break;
				}
				else if( (c == '\r') || (c == '\n'))
				{
					write_bytes = write(STDOUT_FILENO, "\r\n", 2);
					if(write_bytes < 0)
					{
						fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
						exit(1);
					}
				}
				else{
					write_bytes = write(STDOUT_FILENO, &c, 1);
					if(write_bytes <0 ){
						fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
						exit(1);
					}
				}
			}
			if(shut_down == 1)
				break;
		}
		exit(0);
	}
	else	// --shell option
	{
		int to_child_pipe[2];	//read terminal --> write to shell pipe

		if (pipe(to_child_pipe) == -1)
		{
			fprintf(stderr, "pipe() failed! Error: %s\n", strerror(errno));
			exit(1);
		}

		int from_child_pipe[2];	//write terminal <-- read shell pipe

		if (pipe(from_child_pipe) == -1)
		{
			fprintf(stderr, "pipe() failed! Error: %s\n", strerror(errno));
			exit(1);
		}

		/*
		 	Poll waits for one of a set of file descriptors to become ready to perform I/O.
			struct pollfd {
			    int   fd;         file descriptor 
			    short events;     requested events 
			    short revents;    returned events 
			};
		*/

		pid_t child_pid = -1;

		struct pollfd poll_fd[2];

		poll_fd[0].fd = STDIN_FILENO;	//Set to STDIN
		poll_fd[0].events = POLLIN;		//there is data to be read
		poll_fd[1].fd = from_child_pipe[0];		//reads from shell
		poll_fd[1].events = POLLIN;

		signal(SIGPIPE, handler);
		child_pid = fork();	//creating child

		if(child_pid > 0)		//if this is the parent
		{
			close(to_child_pipe[0]);		//close unused read end of to child pipe
			close(from_child_pipe[1]);		//close unused write end of from child pipe
			int status = 0;
			pid_t wait_pid;
			char buf[256];
			int ret_status;
			int read_bytes;

			while(1)
			{
				int ret = poll(poll_fd, 2, 0);

				/* int poll(struct pollfd *fds, nfds_t nfds, int timeout); */
				/*
				  POLLIN : Data may be read without blocking
				  POLLOUT : Data may be written without blocking
				  POLLERR : (revents only) Error has occurred on device / stream
				  POLLHUP : (revents only) Device disconnected / pipe closed <- Mutually exclusive with POLLOUT
				  POLLNVAL : (revents only) Invalid fd
				*/

				if (ret > 0)
				{
					if(poll_fd[0].revents & POLLIN)	//if there is returned events and there is also data to be read
					{
						read_bytes = read(STDIN_FILENO, buf, 256);

						if(read_bytes < 0)
						{
							fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
							exit(1);
						}
						int i;
						char c;
						for (i = 0; i < read_bytes; i++)
						{
							c = buf[i];
							if( c == '\x04')	//receive EOF ^D from keyboard
							{
							//if(debug_mode)
							//	{
							//		printf("received ^D. \n");
							//  }
								ret_status = close(to_child_pipe[1]);	//shell (POLLHUP) will be set
								if(ret_status< 0)
								{
									fprintf(stderr, "Close fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
							else if (c == '\x03')
							{
							//if(debug_mode)
							//	{
							//		printf("received ^C. \n");
							//  }
								kill(child_pid, SIGINT);
							}
							else if((c == '\r') || (c == '\n'))	//receive new line from keyboard
							{
								int write_bytes = write(STDOUT_FILENO, "\r\n", 2);
								if(write_bytes < 0)
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}

								write_bytes = write(to_child_pipe[1], "\n", 1);
								if (write_bytes < 0)
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
							else	//normal input
							{
								int write_bytes = write(STDOUT_FILENO, &c, 1);
								if(write_bytes < 0)
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}

								write_bytes = write(to_child_pipe[1], &c, 1);
								if(write_bytes < 0)
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
						}

					}
					

					if( poll_fd[0].revents & (POLLERR + POLLHUP))
					{
						ret_status = close(from_child_pipe[0]);
						if(ret_status < 0)
						{
							fprintf(stderr, "Close fails. Error: %s\n", strerror(errno));
							exit(1);
						}
						exit(0);
					}

					if(poll_fd[1].revents & POLLIN) //from shell input reading to terminal
					{
						read_bytes = read(from_child_pipe[0], buf, 256);

						if (read_bytes < 0)
						{
							fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
							exit(1);
						}

						int i;
						for (i = 0; i < read_bytes; i++)
						{
							char c = buf[i];
							if ( c == '\n')		//if there is a newline from keyboard
							{
								int write_bytes = write(STDOUT_FILENO, "\r\n", 2);
								if( write_bytes < 0 )
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
							else
							{
								int write_bytes = write(STDOUT_FILENO, &c, 1);
								if ( write_bytes < 0 )
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
						}

					}

					if ( (poll_fd[1].revents & POLLHUP) || (poll_fd[1].revents & POLLERR))
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
				else if (ret < 0)
				{
						fprintf(stderr, "poll fails. Error: %s\n", strerror(errno));
						exit(1);
				}

			}
		}

		else if( child_pid == 0 )	//this is the child
		{
			//CLOSE ALL UNUSED PIPES
		//if(debug_mode)
		//	printf("In child mode. \n");

			close(to_child_pipe[1]);
			close(from_child_pipe[0]);
			dup2(to_child_pipe[0], STDIN_FILENO);
			dup2(from_child_pipe[1], STDOUT_FILENO);
			dup2(from_child_pipe[1], STDERR_FILENO);
			close(to_child_pipe[0]);
			close(from_child_pipe[1]);

			char *execvp_argv[2];
			char execvp_filename[] = "/bin/bash";
			execvp_argv[0] = execvp_filename;
			execvp_argv[1] = NULL;

			if (execvp(execvp_filename, execvp_argv) == -1)
			{
				fprintf(stderr, "execvp() failed! Error: %s\n", strerror(errno));
				exit(1);
			}
		}
		else		//fork has error
		{
				fprintf(stderr, "fork() failed! Error: %s\n", strerror(errno));
				exit(1);
		}
	}

	//if(debug_mode)
	//	printf("reaches the end. \n");

	exit(0);		//to prevent warning when compiling
}