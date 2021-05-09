#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void runPipe()
{
    int childID,bytesRead;
    int parentfd[2];
    char msgFromChild[5];
    if(pipe(parentfd) != 0)
    {
        fprintf(stderr,"Pipe function failed");
        _exit(1);
    }
    childID = fork();
    if(childID == 0)
    {

        close(parentfd[0]);
        write(parentfd[1],"hello",5);
        close(parentfd[1]);

    }
    else
    {
        close(parentfd[1]);

        bytesRead = read(parentfd[0],msgFromChild,5);
        close(parentfd[0]);
        if(bytesRead != 5)
        {
            fprintf(stderr,"Something went wrong while reading from child");
            _exit(0);
        }
        fprintf(stdout,"%s\n",msgFromChild);
    }
}

int main(int argc,char** argv)
{
    runPipe();
    exit(0);
}