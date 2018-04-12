#include<stdlib.h>
#include<signal.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<getopt.h>
#include<string.h>

typedef void sigfunc(int);

sigfunc *signal(int, sigfunc*);

void catchHandler(int sig)
{
	if(sig == SIGSEGV)
	{
		fprintf(stderr, "SIGSEGV detected: %s \n", strerror(errno));
		exit(4);
	}
}

int main(int argc, char** argv)
{
	char* input_file = NULL;
	char* output_file = NULL;
	char* nullPtr  = NULL;
	int segFlag = 0;
	int signalFlag = 0;
	int option_index = 0;   //We need pointer as argument
    int o = 0;

	static struct option long_options[] =
	{
		{"input",required_argument,0,'i'},
		{"output",required_argument,0,'o'},
		{"segfault",no_argument,0,'s'},
		{"catch",no_argument,0,'c'},
		{0,0,0,0}
	};
	/* getopt_long stores the option index here. */


    while(1)
    {	
    	o = getopt_long(argc, argv, "i::o::sc", long_options, &option_index);
		
	    /* Detect the end of the options. */

		if (o <= -1)	//if there is no argument
			break;

	    switch (o) 
	    {
			case 'i':
		  		input_file = optarg;
		  		break;
	        case 'o':
		  		output_file = optarg;
		  		break;
	    	case 's':
		  		segFlag = 1;
				break;
			case 'c':
				signalFlag = 1;
				// signal(SIGSEGV,catchHandler);
				break;
			default: 
		  		fprintf(stderr, "Usage: ./lab0 --input=filename --output=filename \n");
		 		exit(1);
		}
	}

	if(input_file!= NULL)
	{
		int fd0 = open(input_file, O_RDONLY);

		if(fd0 < 0)
		{
			fprintf(stderr, "%s Input file read error: %s\n",input_file, strerror(errno));
			exit(2);
		}
		if (fd0 >= 0)
		{
			close(0);
			dup(fd0);
			close(fd0);
		}
	}


	if(output_file!=NULL)
	{
		int fd1 = creat(output_file, 0666);
		///// 0666 is an octal number, i.e. every one of the 6's corresponds to three permission bits 6 = rw 7 = rwx
		if(fd1 < 0)
		{
			fprintf(stderr, "%s Output file create error: %s \n",output_file, strerror(errno));
			exit(3);
		}
		if (fd1 >= 0) {
			close(1);
			dup(fd1);
			close(fd1);
		}
	}


	if (signalFlag == 1)
	{
		signal(SIGSEGV,catchHandler);
	}

	if(segFlag == 1)
	{
		*nullPtr = 1;
	}


	char* buf[2];
	//buf = (char*) malloc(sizeof(char)*2);
	//ssize_t status;

	/*while(status>0)
	{
		status = read(0,buf,1);
		write(1,buf,1);

	}
	*/

	ssize_t count;
 	while((count=read(0,buf,1))!=0)
    	{write(1,buf,count);} //buffer is of type char*
  	exit(0);

  	/*
	free(buf);

	if (status<0)
	{
		fprintf(stderr, "Error reading file.\n");
	}

	exit(0);
	*/

}

