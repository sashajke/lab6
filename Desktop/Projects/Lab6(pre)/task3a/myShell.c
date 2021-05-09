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
#define ARR_LENGTH 10


int getNumOfElements(char* commandHistory[ARR_LENGTH])
{
    int size=0;
    while(commandHistory[size] != NULL && size < 10)
        size++;
    return size;
}
void addCommand(char* command,char* commandHistory[ARR_LENGTH])
{
    int size=getNumOfElements(commandHistory);
    free(commandHistory[size%10]);
    commandHistory[size%10] = command;
}

void freeCommandsHistory(char* commandsHistory[ARR_LENGTH])
{
    int i=0;
    while(commandsHistory[i] != NULL && i < 10)
    {
        free(commandsHistory[i]);
    }
}

void printHistory(char* commandHistory[ARR_LENGTH])
{
    int i = 0;
    while(commandHistory[i] != NULL && i < ARR_LENGTH)
    {
        fprintf(stdout,"%d) %s\n",i,commandHistory[i]);
        i++;
    }
}


void printDebug(char* command)
{
    fprintf(stderr,"PID: %d\n", getpid());
    fprintf(stderr,"Executing Command: %s\n",command);
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

int checkIfInBounds(int pick,char* commandHistory[ARR_LENGTH])
{
    int ret = commandHistory[pick] != NULL ? 1 : 0;
    return ret;
}

void execute(cmdLine* pCmdLine,int debug,char* commandHistory[ARR_LENGTH],char* inputFromUser)
{
    int res1,res2,inputFD,outputFD,childID1,childID2,
    chdirRes = 0,
    needPipe = pCmdLine -> next != NULL ? 1 : 0,
    cd = strncmp("cd",pCmdLine->arguments[0],2),
    history = strncmp("history",pCmdLine->arguments[0],7),
    reUse = pCmdLine->arguments[0][0] == '!' ? 1 : 0,
    reUseFunctionPick;

    int pipefd[2];
    cmdLine* reUseCommand;
    if(reUse)
    {
        reUseFunctionPick = atoi(pCmdLine->arguments[0]+1);
        if(checkIfInBounds(reUseFunctionPick,commandHistory))
        {
            reUseCommand = parseCmdLines(commandHistory[reUseFunctionPick]);
            execute(reUseCommand,debug,commandHistory,commandHistory[reUseFunctionPick]);
            freeCmdLines(reUseCommand);
        }
        else
        {
            fprintf(stderr,"non existing function pick\n");
        }
    }
    else
    {
        addCommand(strdup(inputFromUser),commandHistory);

        if(needPipe)
        {
            if(pipe(pipefd) != 0)
            {
                perror("pipe");
            }
        }
        if(cd == 0)
        {
            chdirRes = chdir(pCmdLine -> arguments[1]);
            if(chdirRes == -1)
            {
                perror("chdir");
            }
        }
        else if(history == 0)
        {
            printHistory(commandHistory);
        }
        else
        {
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
    }
    


}

int main(int argc,char** argv)
{
    char* commandsHistory[ARR_LENGTH];
    char* res;
    int i;
    for(i = 0; i<ARR_LENGTH;i++)
    {
        commandsHistory[i++] = NULL;
    }
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
            perror("getcwd");
        }
        fprintf(stdout,"%s\n",workingDir);
        
        fgets(inputFromUser,inputMaxSize,input);
        if(strncmp(inputFromUser,"quit",4) == 0)
        {
            //freeCommandsHistory(commandsHistory);
            break;
        }
        command = parseCmdLines(inputFromUser);
        execute(command,debug,commandsHistory,inputFromUser);
        freeCmdLines(command);
    }
    exit(EXIT_SUCCESS);
}