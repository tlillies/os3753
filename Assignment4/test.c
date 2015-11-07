#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argv, char **argc) {
    if(argv != 4){};
    char* args[10];
    char* args2[10];
    char* program = argc[1];
    args[0] = program;
    long forks = atol(argc[2]);
    
    if(!strcmp(program,"./pi-sched")) { // not mixed
        char* iterations = argc[3];
        char* sched = argc[4];
        args[1] = iterations;
        args[2] = sched;
        args[3] = 0;
    }
    
    if(!strcmp(program,"./rw")) { // not mixed
        char* bytes = argc[3];
        char* blocks = argc[4];
        char* sched = argc[5];

        args[1] = bytes;
        args[2] = blocks;
        args[3] = sched;
        args[4] = 0;
        printf("%s, %s, %s", args[1],args[2],args[3]);
    }

    if(!strcmp(program,"mixed")) { // not mixed
        char* iterations = argc[3];
        char* bytes = argc[4];
        char* blocks = argc[5];
        char* sched = argc[6];
        args[1] = iterations;
        args[2] = sched;
        args[3] = 0;

        args2[1] = bytes;
        args2[2] = blocks;
        args2[3] = sched;
        args2[4] = 0;
    }
    args[0] = program;

    int status;
    int i  = 0;

    if(strcmp(program,"mixed")) { // not mixed
        printf("%s forked %lu times",program,forks);
        for(i = 0; i < forks; i++) {
            if(fork() == 0) {
                execve(program, &args[0], NULL); // run pi-sched with arguments
            }
        }
    }
    else { // mixed program
        for(i = 0; i < forks/2; i++) {
            if(fork() == 0) {
                args[0] = "./pi-sched";
                execve("./pi-sched", &args[0], NULL); // run pi-sched with arguments
            }
        }
        for(i = 0; i < forks; i++) {
            if(fork() == 0) {
                args[0] = "./rw";
                execve("./rw", &args2[0], NULL); // run pi-sched with arguments
            }
        }
    }
    int j = 0;
    for(j = 0; j < forks; j++) {
        waitpid(0, &status, 0); // wait for the children
    }
    return 0;
}
