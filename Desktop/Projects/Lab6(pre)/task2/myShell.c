#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "linux/limits.h"
#include "./LineParser.h"
#include "sys/wait.h"
#include <sys/stat.h>
#include <fcntl.h>



void printDebug(char* command)
{
    fprintf(stderr,"PID: %d\n", getpid());
    fprintf(stderr,"Executing Command: %s\n",command);
}
void handler(int sig)
{
	printf("\nRecieved Signal : %s\n",strsignal(sig));
	if(sig == SIGTSTP)
	{
		signal(SIGCONT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
	}
	else if(sig == SIGCONT)
	{
        signal(SIGCONT,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
	}
	else
	{
		signal(sig,SIG_DFL);
	}
    raise(sig);
}

void executeChild(cmdLine* pCmdLine,int debug,int needPipe,int pipefd[2],int child)
{
    int inputFD,outputFD,res;
    if(pCmdLine -> inputRedirect != NULL)
    {
        inputFD = open(pCmdLine -> inputRedirect,O_RDONLY);
        dup2(inputFD,STDIN_FILENO);
        close(inputFD);
    }
    if(pCmdLine -> outputRedirect != NULL)
    {
        outputFD = open(pCmdLine -> outputRedirect,O_WRONLY);
        dup2(outputFD,STDOUT_FILENO);
        close(outputFD);
    }
    if(needPipe) // if we have a pipe
    {
        if(child == 1)
        {
            close(STDOUT_FILENO);
            dup(pipefd[1]);
            close(pipefd[1]);
        }
        else if(child == 2)
        {
            close(STDIN_FILENO);
            dup(pipefd[0]);
            close(pipefd[0]);
        }
    }      
    res = execvp(pCmdLine->arguments[0],pCmdLine -> arguments);
    if(res == -1)
    {
        perror("ERROR!!!");
        free(pCmdLine);
        _exit(EXIT_FAILURE);
    }
    if(debug)
    {
        printDebug(pCmdLine -> arguments[0]);
    }
    _exit(0); 
}

void execute(cmdLine* pCmdLine,int debug)
{
    int res1,res2,inputFD,outputFD,childID1,childID2,needPipe = pCmdLine -> next != NULL ? 1 : 0;
    int pipefd[2];
    int chdirRes = 0;

    if(needPipe)
    {
        if(pipe(pipefd) != 0)
        {
            perror("pipe");
        }
    }
    if(strncmp("cd",pCmdLine->arguments[0],2) == 0)
    {
        chdirRes = chdir(pCmdLine -> arguments[1]);
        if(chdirRes == -1)
        {
            perror("chdir");
        }
    }
    childID1 = fork();
    if(childID1 == 0)
    {
        executeChild(pCmdLine,debug,needPipe,pipefd,1);     
    }
    if(needPipe)
    {
        close(pipefd[1]);
        childID2 = fork();
        if(childID2 == 0)
        {
            executeChild(pCmdLine->next,debug,needPipe,pipefd,2);
        }
        close(pipefd[0]);
        if(pCmdLine -> next -> blocking == 1)
            waitpid(childID2,NULL,0);
    }
    if(pCmdLine ->blocking == 1)
        waitpid(childID1,NULL,0);
    
}

int main(int argc,char** argv)
{
    signal(SIGINT,handler);
    signal(SIGTSTP,handler);
    signal(SIGCONT,handler);
    char* res;
    while(1)
    {

        const int inputMaxSize = 2048;
        int i,debug = 0;
        FILE* outpout = stdout;
        FILE* input = stdin;
        cmdLine* command;
        char inputFromUser[inputMaxSize];
        char workingDir[PATH_MAX];
        
        for(i=1;i<argc;i++)
        {
            if(argv[i][1] == 'd')
                debug = 1;
        }

        res = getcwd(workingDir,PATH_MAX);
        if(errno == ERANGE)
        {   
            printf("%s", "error when reading current directory name");
        }
        fprintf(stdout,"%s\n",workingDir);
        
        fgets(inputFromUser,inputMaxSize,input);
        if(strncmp(inputFromUser,"quit",4) == 0)
        {
            break;
        }
        command = parseCmdLines(inputFromUser);
        execute(command,debug);
        freeCmdLines(command);
    }
    exit(EXIT_SUCCESS);
}