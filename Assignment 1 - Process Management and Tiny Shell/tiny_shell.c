#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#define DEFAULT_STACK_SIZE	8192

//#define _POSIX_C_SOURCE 199309

//functions defined
/*int my_system_f(char *user_command);
int my_system_v(char *user_command);
int my_system_c(char *user_command);
int clone_function(void* command);*/

//read from terminal
char* get_a_line(){
    printf("Enter command:");
    char* line;
    size_t lineSize = 40;
    line = (char *) malloc(lineSize *sizeof(char));
	getline(&line,&lineSize,stdin);
    strtok (line, "\n");
    return line;
}

//length algorithm
int length(char* line){
    return strlen(line);
}

//System call for Fork
int my_system_f(char* command){
    pid_t pid_from_fork;
	pid_from_fork = fork();     //fork started, pid passed                     
    int status;
    //failure to fork will return -1 to parent, no child created
    if (pid_from_fork == -1) {
		perror("Fork failed\n");
        // exit(EXIT_FAILURE);
	}
    //if fork successful, child will get pid = 0
    else if (pid_from_fork == 0) {
        int i = execl("/bin/sh", "sh", "-c", command, (char *)0);
        if (i==-1){		//execl only returns something if the execution fails
			_exit(2);   // terminates the calling process "immediately"
		}
    }
    //if fork successful, parent will get pid of child
    else if (pid_from_fork > 0) {
		waitpid(pid_from_fork, &status, 0);
		if(status == -1)
		{
			perror("Child failed\n");
		}
	}
}

//System call for VFork
int my_system_v(char* command){
    pid_t pid_from_vfork;
	pid_from_vfork = vfork();   //vfork started, pid passed
    int status;
    //failure to fork will return -1 to parent, no child created
    if (pid_from_vfork == -1) {
		perror("VFork error\n");
        exit(EXIT_FAILURE);
	}
    //if fork successful, child will get pid = 0
    else if (pid_from_vfork == 0) {
        int i= execl("/bin/sh", "sh", "-c", command, (char *)0);    //execl accesses path
            if (i==-1){		//execl only returns something if the execution fails
			_exit(2);	// terminates the calling process "immediately"
		}
        return EXIT_SUCCESS;
	}
    //if fork successful, parent will get pid of child
    else if (pid_from_vfork > 0) {
		waitpid(pid_from_vfork, &status, 0);
		if(status == -1)
		{
			perror("Child failed\n");
		}
	}
}

//Child clone function
int clone_function(void* command){
    char* command_in = (char*)command;      //convert from void* to char*
    int len = (int)strlen(command_in);      //get length of string
    char dircommand[len];                   //create new array
    strcat(dircommand, command_in);         //concatenate string to empty array
    strtok (dircommand, " ");               //extract cd by removing everything after " "
    if (dircommand == "cd"){
        //extract the directory if the command is cd
        char* dircommand = (char*)command + 3; //3 being the lenght of cd, moving the pointer by 3
        chdir(dircommand);    //an alternate to cd, argument of chdir is a directory
    }
    else{
        int i = execl("/bin/sh", "sh", "-c", command_in, (char *)0);
        if (i==-1){
            _exit(2);
        }
    }
    return 0;
}

//System call for Clone
int my_system_c(char* command){
	void* clone_process_stack = malloc(DEFAULT_STACK_SIZE);
    if(clone_process_stack == NULL){
        perror("malloc null");
        exit(EXIT_FAILURE);
    }
	void* stack_top = clone_process_stack + DEFAULT_STACK_SIZE;
    int clone_flags = CLONE_VFORK | CLONE_FS;

	int child_pid = clone(clone_function, stack_top, clone_flags | SIGCHLD, command);
	
    int status;
	pid_t parent_wait;
    parent_wait = waitpid(child_pid, &status, __WALL);
	if(status == -1)
	{
		perror("waitpid");
	}
	return 0;
}

int my_system_p_read(char *command, char *my_fifo){
	pid_t pid_from_fork;
	pid_from_fork = fork();
	int status;
	int fd_read;

    //Fork Fail
    if (pid_from_fork == -1){
		perror("fork failed");
		exit(EXIT_FAILURE);
    }
	//child
    else if(pid_from_fork == 0)
	{
	    fd_read = open(my_fifo, O_RDONLY);
		if(fd_read == -1)
		{
			perror("open");
		}
		close(0);
		dup2(fd_read, 0);

		int i = execl("/bin/sh", "sh", "-c", command, (char *)0);
        if (i==-1){
            _exit(2);
        }
	}
    //parent
	else if (pid_from_fork > 0){
		waitpid(pid_from_fork, &status, 0);
		if(status == -1)
		{
			perror("The child process failed.\n");
		}
	}
	return(0);
}


int my_system_p_write(char *command, char *my_fifo){
	pid_t pid_from_fork;
	pid_from_fork = fork();
	int status;
	int fd_write;

    //Fork Fail
    if (pid_from_fork == -1){
		perror("fork failed");
		exit(EXIT_FAILURE);
    }
    //child
	else if(pid_from_fork == 0){
		fd_write = open(my_fifo, O_WRONLY);
		if(fd_write == -1)
		{
			perror("fdopen");
		}
		close(1);
		dup2(fd_write, 1);
		int i = execl("/bin/sh", "sh", "-c", command, (char *)0);
        if (i==-1){
            _exit(2);
        }
	}
    //parent
	else if (pid_from_fork > 0){
		waitpid(pid_from_fork, &status, 0);
		if(status == -1)
		{
			perror("The child process failed.\n");
		}
	}
	return(0);
}

//Function to call processes based to Defined Flag
int my_system(char* line, char* my_fifo){
    #ifdef FORK
        my_system_f(line);

    #elif VFORK
        my_system_v(line);
    
    #elif CLONE 
        my_system_c(line);

    #elif FIFO_READ
        my_system_p_read(line, my_fifo);

    #elif FIFO_WRITE
        my_system_p_write(line, my_fifo);

    #else
        system(line);

    #endif
}

int main(int argc, char *argv[]){
    char* line; 
    struct timespec startTime, stopTime;
    char *my_fifo = argv[1];
    while(1){
        line = get_a_line();
        if (length(line) > 1){
            //timer starts
			if(clock_gettime(CLOCK_REALTIME, &startTime) == -1)
			{
				perror("clock_gettime");
			}

            //execution of my_system
            my_system(line, my_fifo);

            //timer ends
            if(clock_gettime(CLOCK_REALTIME, &stopTime) == -1)
			{
				perror("clock_gettime");
			}
            //calculating time in milliseconds
			double time_passed_miliseconds = ((stopTime.tv_sec - startTime.tv_sec)*1000)+(stopTime.tv_nsec - startTime.tv_nsec)/1000000;
			printf("time elapsed = %f ms\n",time_passed_miliseconds);
        }
        else{
            exit(0);
        }
    }
    return 0;
}