/*
 * File: util.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2012/02/01
 * Modify Date: 2012/02/01
 * Description:
 * 	This file contains declarations of utility functions for
 *      Programming Assignment 2.
 *  
 */

#include "util.h"

int dnslookup(const char* hostname, char* firstIPstr, int maxSize){

    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct in_addr* ipv4addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];

    int addrError = 0;

    /* DEBUG: Print Hostname*/
#ifdef UTIL_DEBUG
    fprintf(stderr, "%s\n", hostname);
#endif

    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
        fprintf(stderr, "Error looking up Address: %s\n",
                gai_strerror(addrError));
        return UTIL_FAILURE;
    }
    /* Loop Through result Linked List */
    for(result=headresult; result != NULL; result = result->ai_next){
        /* Extract IP Address and Convert to String */
        if(result->ai_addr->sa_family == AF_INET){
            /* IPv4 Address Handling */
            ipv4sock = (struct sockaddr_in*)(result->ai_addr);
            ipv4addr = &(ipv4sock->sin_addr);
            if(!inet_ntop(result->ai_family, ipv4addr,
                        ipv4str, sizeof(ipv4str))){
                perror("Error Converting IP to String");
                return UTIL_FAILURE;
            }
#ifdef UTIL_DEBUG
            fprintf(stdout, "%s\n", ipv4str);
#endif
            strncpy(ipstr, ipv4str, sizeof(ipstr));
            ipstr[sizeof(ipstr)-1] = '\0';
        }
        else if(result->ai_addr->sa_family == AF_INET6){
            /* IPv6 Handling */
#ifdef UTIL_DEBUG
            fprintf(stdout, "IPv6 Address: Not Handled\n");
#endif
            strncpy(ipstr, "UNHANDLED", sizeof(ipstr));
            ipstr[sizeof(ipstr)-1] = '\0';
        }
        else{
            /* Unhandlded Protocol Handling */
#ifdef UTIL_DEBUG
            fprintf(stdout, "Unknown Protocol: Not Handled\n");
#endif
            strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
            ipstr[sizeof(ipstr)-1] = '\0';
        }
        /* Save First IP Address */
        if(result==headresult){
            strncpy(firstIPstr, ipstr, maxSize);
            firstIPstr[maxSize-1] = '\0';
        }
    }

    /* Cleanup */
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}
int mydnslookup(const char* hostname, char ips[MAX_IPS][INET6_ADDRSTRLEN], int * num_ips, int maxSize){

    /* Local vars */
    struct addrinfo* headresult = NULL;
    struct addrinfo* result = NULL;
    struct sockaddr_in* ipv4sock = NULL;
    struct sockaddr_in6* ipv6sock = NULL;
    struct in_addr* ipv4addr = NULL;
    struct in6_addr* ipv6addr = NULL;
    char ipv4str[INET_ADDRSTRLEN];
    char ipv6str[INET6_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    int addrError = 0;
    int i = 0;

    /* DEBUG: Print Hostname*/
#ifdef UTIL_DEBUG
    fprintf(stderr, "%s\n", hostname);
#endif

    /* Lookup Hostname */
    addrError = getaddrinfo(hostname, NULL, NULL, &headresult);
    if(addrError){
        fprintf(stderr, "Error looking up Address: %s\n",
                gai_strerror(addrError));
        return UTIL_FAILURE;
    }
    /* Loop Through result Linked List */
    for(result=headresult; result != NULL; result = result->ai_next){
        if(i == MAX_IPS) {
            break;
        }
        /* Extract IP Address and Convert to String */
        if(result->ai_addr->sa_family == AF_INET){
            /* IPv4 Address Handling */
            ipv4sock = (struct sockaddr_in*)(result->ai_addr);
            ipv4addr = &(ipv4sock->sin_addr);
            if(!inet_ntop(result->ai_family, ipv4addr,
                        ipv4str, sizeof(ipv4str))){
                perror("Error Converting IP to String");
                return UTIL_FAILURE;
            }
#ifdef UTIL_DEBUG
            fprintf(stdout, "%s\n", ipv4str);
#endif
            strncpy(ipstr, ipv4str, sizeof(ipstr));
            ipstr[sizeof(ipstr)-1] = '\0';
            strncpy(ips[i], ipstr, maxSize);
            ips[i][maxSize-1] = '\0';
            i++;
        }
        else if(result->ai_addr->sa_family == AF_INET6){
            /* IPv6 Handling */
#ifdef UTIL_DEBUG
            fprintf(stdout, "IPv6 Address: Not Handled\n");
#endif
            ipv6sock = (struct sockaddr_in6*)(result->ai_addr);
            ipv6addr = &(ipv6sock->sin6_addr);
            if(!inet_ntop(result->ai_family, ipv6addr,
                        ipv6str, sizeof(ipv6str))){
                perror("Error Converting IP to String");
                return UTIL_FAILURE;
            }
            strncpy(ipstr, ipv6str, sizeof(ipstr));
            ipstr[sizeof(ipstr)-1] = '\0';
            strncpy(ips[i], ipstr, maxSize);
            ips[i][maxSize-1] = '\0';
            i++;
        }
        else{
            /* Unhandlded Protocol Handling */
#ifdef UTIL_DEBUG
            fprintf(stdout, "Unknown Protocol: Not Handled\n");
#endif
            //strncpy(ipstr, "UNHANDELED", sizeof(ipstr));
            //ipstr[sizeof(ipstr)-1] = '\0';
            return UTIL_FAILURE;
        }
    }
    *num_ips = i;
    /* Cleanup */
    freeaddrinfo(headresult);

    return UTIL_SUCCESS;
}
