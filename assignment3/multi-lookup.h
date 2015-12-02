#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 2015
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

void* requester(void* filename);
void* resolver(void* outputfp);
