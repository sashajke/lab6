#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "linux/limits.h"
#include "sys/wait.h"
#include <sys/stat.h>
#include <fcntl.h>


void runMyPipeLine(int debug)
{
    int child1,child2;
    char* lsCommand[] = {"ls","-l",NULL};
    char* tailCommand[] = {"tail","-n","2",NULL};



    int pipeArr[2];
    if(pipe(pipeArr) != 0)
    {
        perror("pipe");
        _exit(1);
    }
    if(debug)
        fprintf(stderr,"Parent_Process > forking...\n");
    child1 = fork();
    if(child1 == 0)
    {   
        close(STDOUT_FILENO);
        if(debug)
            fprintf(stderr,"Child1 > Redirecting stdout to the write end of the pipe...\n");
        dup(pipeArr[1]);
        close(pipeArr[1]);
        if(debug)
            fprintf(stderr,"Child1 > Going to execute cmd...\n");
        execvp(lsCommand[0],lsCommand);
    }
    if(debug)
    {
        fprintf(stderr,"Parent_Process > Created process with id: %d\n",child1);
        fprintf(stderr,"Parent_Process > Closing the write end of the pipe...\n");
    }
    close(pipeArr[1]);
    if(debug)
        fprintf(stderr,"Parent_Process > forking...\n");
    child2 = fork();
    if(child2 == 0)
    {
        close(STDIN_FILENO);
        if(debug)
            fprintf(stderr,"Child2 > Redirecting stdin to the read end of the pipe...\n");
        dup(pipeArr[0]);
        close(pipeArr[0]);
        if(debug)
            fprintf(stderr,"Child2 > Going to execute cmd...\n");
        execvp(tailCommand[0],tailCommand);
    }
    if(debug)
    {
        fprintf(stderr,"Parent_Process > Created process with id: %d\n",child2);
        fprintf(stderr,"Parent_Process > Closing the read end of the pipe...\n");

    }
    close(pipeArr[0]);
    if(debug)
        fprintf(stderr,"Parent_Process > Waiting for child processes to terminate...\n");
    waitpid(child1,NULL,0);
    waitpid(child2,NULL,0);
    if(debug)
        fprintf(stderr,"Parent_Process > exiting...\n");
    _exit(0);
}


int main(int argc,char** argv)
{
    int i,debug = 0;
    for(i = 1; i<argc;i++)
    {
        if(argv[i][1] == 'd')
        {
            debug = 1;
        }
    }
    runMyPipeLine(debug);
    return 0;
}
