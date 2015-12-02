/*
 * File: multi-lookup.c
 * Author: Thomas Lillis
 * Project: CSCI 3753 Programming Assignment 3
 *  
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "queue.h"
#include "util.h"

#include "multi-lookup.h"

#define MINARGS 3
#define USAGE "<inputFilePath> ... <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue q;
pthread_mutex_t q_lock;
pthread_mutex_t file_lock;
pthread_mutex_t finished_lock;
int requesters_finished = 0;

void* requester(void* filename) {
    char* file = filename;
    FILE* inputfp = NULL;
    char* payload;
    char hostname[SBUFSIZE];

    printf("%s\n",file);

    /* Open Input File */
    inputfp = fopen(file, "r");
    if(!inputfp){
        fprintf(stderr, "Error Opening Input File: %s", file);
        return NULL;
    }

    /* Read File and Process*/
    while(fscanf(inputfp, INPUTFS, hostname) > 0) {
        pthread_mutex_lock(&q_lock);
        while(queue_is_full(&q)) { // Sleep while queue is full
            pthread_mutex_unlock(&q_lock);
            usleep(rand() % 100);
            pthread_mutex_lock(&q_lock);
        }
        //printf("%s\n",hostname);

        /* Prepare Payload */
        payload = malloc(sizeof(hostname));
        strcpy(payload,hostname);

        /* Push to Queue */
        while (queue_push(&q, (void*) payload) == QUEUE_FAILURE) {
            fprintf(stderr, "Queue push failed \n");
        }
        pthread_mutex_unlock(&q_lock);
    }

    /* Close Input File */
    if (fclose(inputfp)) {
        fprintf(stderr, "Error closing output file \n");
    }

    printf("Finished %s\n",file);
    return 0;
}

void* resolver(void *outputfp) {
    char ips[100][INET6_ADDRSTRLEN];
    char lastip[INET6_ADDRSTRLEN];
    char * hostname = NULL;
    int num_ips = 0;
    int i = 0;

    /* Loop While Requesters or Data In Queue */
    pthread_mutex_lock(&q_lock);
    pthread_mutex_lock(&finished_lock);
    while (!queue_is_empty(&q) || !requesters_finished) {
        pthread_mutex_unlock(&finished_lock);
        if ((hostname = (char*)queue_pop(&q)) == NULL) {
            pthread_mutex_unlock(&q_lock);
            //fprintf(stderr, "Queue pop failed \n");
        } else { //Pop from queue was succsesful

            pthread_mutex_unlock(&q_lock);
            //printf("%s\n",hostname);

            /* Lookup hostname and get IP string */
            num_ips = 0;
            if(mydnslookup(hostname, ips, &num_ips, sizeof(ips[0]))
                    == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", hostname);
                strncpy(ips[0], "", sizeof(ips));
                num_ips++;
            }
            
            /* Write to Output File */
            pthread_mutex_lock(&file_lock);
            fprintf(outputfp, "%s", hostname);
            for(i = 0; i < num_ips; i++) {
                    if(strcmp(lastip, ips[i]) != 0) {
                        fprintf(outputfp, ",%s", ips[i]);
                        strcpy(lastip,ips[i]);
                    }
                    memset(ips[i],0,strlen(ips[i]));
            }
            fprintf(outputfp, "\n");
            pthread_mutex_unlock(&file_lock);
            free(hostname);
        }

        pthread_mutex_lock(&q_lock);
        pthread_mutex_lock(&finished_lock);

    }
    pthread_mutex_unlock(&finished_lock);
    pthread_mutex_unlock(&q_lock);
    return 0;
}


int main(int argc, char* argv[]){

    /* Local Vars */
    int num_files = argc -2;
    int num_requester_threads = num_files;
    int num_resolver_threads = sysconf(_SC_NPROCESSORS_ONLN); // number of cores

    pthread_t requester_threads[num_requester_threads];
    pthread_t resolver_threads[num_resolver_threads];

    FILE* outputfp = NULL;

    int t;
    int rc;

    /* Check Arguments */
    if(argc < MINARGS){
        fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
        fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }

    /* Check Number of Files */
    if(num_files > MAX_INPUT_FILES) {
        fprintf(stderr, "Too many input files: %d\n", num_files);
        fprintf(stderr, "Max of %d files\n", MAX_INPUT_FILES);
        return EXIT_FAILURE;
    }

    /* Check Number of Resolvers */
    if (num_resolver_threads < MIN_RESOLVER_THREADS) {
        num_resolver_threads = MIN_RESOLVER_THREADS;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if (!outputfp) {
        fprintf(stderr, "Error opening output file \n");
        return EXIT_FAILURE;
    }

    /* Create the Queue */
    if (queue_init(&q, QUEUEMAXSIZE) == QUEUE_FAILURE) {
        fprintf(stderr, "Initializing queue failed \n");
        return EXIT_FAILURE;
    }

    /* Init Mutex */
    if (pthread_mutex_init(&q_lock, NULL)) {
        fprintf(stderr, "Creating queue mutex failed \n");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&file_lock, NULL)) {
        fprintf(stderr, "Creating file mutex failed \n");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&finished_lock, NULL)) {
        fprintf(stderr, "Creating finished mutex failed \n");
        return EXIT_FAILURE;
    }

    /* Spawn Requester Threads */
    for(t=0; t<num_requester_threads; t++){
        rc = pthread_create(&(requester_threads[t]), NULL, requester, argv[t+1]);
        if (rc){
            printf("ERROR in input thread creation; return code from pthread_create() is %d\n", rc);
            return EXIT_FAILURE;
        }
    }

    /* Spawn Resolver Threads */
    for(t=0; t<num_resolver_threads; t++){
        rc = pthread_create(&(resolver_threads[t]), NULL, resolver,(void*)outputfp);
        if (rc){
            printf("ERROR in output thread creation; return code from pthread_create() is %d\n", rc);
            return EXIT_FAILURE;
        }
    }

    /* Wait For Requester Threads */
    for(t=0; t<num_requester_threads; t++){
        pthread_join(requester_threads[t],NULL);
    }

    /* Let The Resolvers Know Requesters are done */
    pthread_mutex_lock(&finished_lock);
    requesters_finished = 1;
    pthread_mutex_unlock(&finished_lock);

    /* Wait for Resolver Threads */
    for(t=0; t<num_resolver_threads; t++){
        pthread_join(resolver_threads[t],NULL);
    }

    /* Close Output File */
    if (fclose(outputfp)) {
        fprintf(stderr, "Error closing output file \n");
    }

    /* Cleanup Queue */
    queue_cleanup(&q);

    /* Destroy Mutex */
    if (pthread_mutex_destroy(&q_lock)) {
        fprintf(stderr, "Destroying queue mutex failed \n");
    } 
    if (pthread_mutex_destroy(&file_lock)) {
        fprintf(stderr, "Destroying file mutex failed \n");
    }
    if (pthread_mutex_destroy(&finished_lock)) {
        fprintf(stderr, "Destroying finished mutex failed \n");
    }

    printf("All of the threads were completed!\n");
    return EXIT_SUCCESS;
}
