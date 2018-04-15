/*Sample code test fork shell*/

#include "myheader.h"

int main(){
	
	int to_child_pipe[2];
	int from_child_pipe[2];
	pid_t child_pid = -1;

	if(pipe(to_child_pipe) == -1)
	{
		fprintf(stderr, "pipe() failed. \n");
		exit(1);
	}

	if (pipe(from_child_pipe) == -1)
	{
		fprintf(stderr, "pipe() failed. \n");
		exit(1);
	}

	child_pid = fork(); 

	if(child_pid >0)	//parent process
	{
		close(to_child_pipe[0]);
		close(from_child_pipe[1]);

		char buffer[2048];
		int count = 0;
		count = read(STDIN_FILEIO, buffer, 2048);
		write(to_child_pipe[1], buffer, count);
		count = read(from_child_pipe[0], buffer, 2048);
		write(STDOUT_FILEIO, buffer, count);
	}
	else if(chid_pid == 0) {
		close(to_child_pipe[1]);
		close(from_child_pipe[0]);
		dup2(to_child_pipe[0], STDIN_FILENO);
		dup2(from_child_pipe[1], STDOUT_FILENO);
		close(to_child_pipe[0]);
		close(from_child_pipe[1]);

		char *execvp_argv[2];
		char execvp_filename[] = "/bin/bash";
		execvp_argv[0] = execvp_filename;
		execvp_argv[1] = NULL;
		if (execvp(execvp_filename, execvp_argv) == -1){
			fprintf(stderr, "execvp() failed!\n");
			exit(1);
		}
	}
	else{
		fprintf(stderr, "fork() failed. \n");
		exit(1);
	}

	return 0;
}

