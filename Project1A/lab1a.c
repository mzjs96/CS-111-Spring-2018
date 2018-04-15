#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>

//Save the normal terminal attributes
struct termios saved_attributes;

void restore_normal_mode(void)
{
	tcsetattr(0, TCSAFLUSH, &saved_attributes);
	//tcsetattr - set the parameters associated with the terminal
	//TCSAFLUSH: the change shall occur after all output written to fildes is transmitted

}

void set_input_mode (void)
{	
	/* this process Make sure stdin is a terminal. */
	if(!isatty(STDIN_FILENO))
	{
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	 //Get the normal mode attributes and restore after exit
	/* Save the terminal attributes so we can restore them later. */
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
}

void handler (int signum)
{
	if (signum == SIGPIPE)
	{
		fprintf(stderr, "Caught Segmentation Fault with signal number %d\n", signum);
		printf("Error:%s\n", strerror(errno));
		printf("shell end SIGPIPE\n");
	}

	exit(0);
}


int main (int argc, char* argv[])
{
	static struct option long_options[] = {
		{"shell", no_argument, NULL, 's'},
		{0,0,0,0}
	};

	int option_index;
	int is_shell = 0;
	while(1)
	{
		int opt = getopt_long(argc,argv, "", long_options, &option_index);

		if(opt == -1)
			break;
		switch(opt)
		{
			case 's':
			is_shell = 1;
			break;
			default:
			printf ("Unrecognize argument\n");
			fprintf (stderr, "getopt generated unrecognized character code: %d\n", opt);
			exit(1);
			break;
		}
	}

	set_input_mode();

	if(is_shell == 0)
	{
		char buf[256];
		int read_bytes;
		int shut_down = 0;
		while(1)
		{
			read_bytes = read(0, buf, 256);

			if (read_bytes < 0)
			{
				fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
				exit(1);
			}

			int i;

			for (i = 0; i < read_bytes; i++)
			{
				char c = buf [i];
				if (c == '\x04')	//C-d
				{
					shut_down = 1;
					break;
				}
				else if ( (c == '\r') || (c == '\n') )
				{
					int write_bytes = write(1, "\r\n", 2);
					if (write_bytes < 0)
					{
						fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
						exit(1);
					}
				}
				else 
				{
					int write_bytes = write(1, &c, read_bytes);
					if (write_bytes < 0)
					{
						fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
						exit(1);
					}
				}
			}

			if (shut_down == 1)
				break; 
		}
		exit(0);
	}


	else	//if there is --shell option
	{
		int to_child_pipe[2];	//read terminal --> write to shell pipe
		int from_child_pipe[2];	//write terminal <-- read shell pipe
		pid_t child_pid = -1;	

		if (pipe(to_child_pipe) == -1)
		{
			fprintf(stderr, "pipe() failed! Error: %s\n", strerror(errno));
			exit(1);
		}

		if (pipe(from_child_pipe) == -1)
		{
			fprintf(stderr, "pipe() failed! Error: %s\n", strerror(errno));
			exit(1);
		}

		struct pollfd pfd[2];
		/*
		 	Poll waits for one of a set of file descriptors to become ready to perform I/O.
			struct pollfd {
			    int   fd;         /* file descriptor 
			    short events;     /* requested events 
			    short revents;    /* returned events 
			};
		*/

		pfd[0].fd = STDIN_FILENO;	 	 //set to 0
		pfd[0].events = POLLIN;			 //there is data to be read
		pfd[1].fd =from_child_pipe[0]; 
		pfd[1].events = POLLIN;

		signal(SIGPIPE, handler);
		child_pid = fork();	//creating child

		if (child_pid > 0)	//if this is the parent.
		{
			int status = 0;
			pid_t wait_pid;

			close(to_child_pipe[0]);	//close unused read end of pipe
			close(from_child_pipe[1]);	//close unused write end of pipe

			char buf[256];
			int read_bytes = 0;
			int keyboard_off = 0;
			int shell_off = 0;

			while(1)
			{
				int ret = poll(pfd, 2, 0);
				/* int poll(struct pollfd *fds, nfds_t nfds, int timeout); */
				if (ret > 0)
				{
					if (pfd[0].revents & POLLIN)	//if there is returned events and there is also data to be read
					{
						read_bytes = read(STDIN_FILENO, buf, 256);

						if(read_bytes < 0)
						{
							fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
							exit(1);
						}

						int i = 0;
						for (i = 0; i < read_bytes; i++)
						{
							char c = buf[i];
							if( c == '\x04')	//receive EOF from keyboard
							{
								keyboard_off = 1;
								close(to_child_pipe[1]);	//shell (POLLHUP) will be set
							}
							else if (c == '\x03')
							{
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
								int write_bytes = write(STDIN_FILENO, &c, 1);
								if(write_bytes <0)
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

					if (pfd[1].revents & POLLIN)	//from shell input waiting
					{
						read_bytes = read(from_child_pipe[0], buf, 256);
						int i = 0;

						if (read_bytes < 0)
						{
							fprintf(stderr, "Read fails. Error: %s\n", strerror(errno));
							exit(1);
						}

						for (i = 0; i < read_bytes; i++)
						{
							char c = buf[i];
							if ( c == '\n')		//new line from keyboard
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
								if ( write_bytes <0 )
								{
									fprintf(stderr, "Write fails. Error: %s\n", strerror(errno));
									exit(1);
								}
							}
						}
					}

					if ( pfd[0].revents & (POLLERR + POLLHUP))
					{
						close (from_child_pipe[0]);
						exit(0);
					}

					if( (pfd[1].revents & POLLHUP) || (pfd[1].revents & POLLERR) )
					{
						wait_pid = waitpid(child_pid, &status, WNOHANG);
						int p_status = 0;
						int p_signal = 0;
						if(WIFEXITED(status))
						{
							p_status = WEXITSTATUS(status);
						}
						if(WIFSIGNALED(status))
						{
							p_signal = WTERMSIG(status);
						}

						fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", p_signal, p_status);

						exit(0);
					}

				}
				else if(ret < 0)
				{
					fprintf(stderr, "poll fails. Error: %s\n", strerror(errno));
					exit(1);
				}
			}
		}
		else if(child_pid == 0)	//child
		{
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
		else
		{
			fprintf(stderr, "fork() failed! Error: %s\n", strerror(errno));
			exit(1);
		}
	}
}



