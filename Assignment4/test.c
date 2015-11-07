#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argv, char **argc) {
    if(argv != 4) return 0;
    char* program = argc[1];
    long forks = atol(argc[2]);
    char* iterations = argc[3];
    char** args;
    args[0] = iterations;
    printf("%s forked %lu times each with %s\n",program,forks,iterations);

    int status;
    int i  = 0;
    for(i = 0; i < forks; i++) {
        if(fork() == 0) {
            int ret = execve(program, args, NULL); // run pi-sched with arguments
        }
    }
    int j = 0;
    for(j = 0; j < forks; j++) {
        waitpid(0, &status, 0); // wait for the children
    }
    return 0;
}
