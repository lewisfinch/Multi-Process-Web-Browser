#ifndef _SERVER_H
#define _SERVER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

/********************* [ Helpful Macro Definitions ] **********************/
// IGNORE MIN_PORT, MAX_PORT
#define MIN_PORT 1024                               //Minimum port number, < 1024 is protected
#define MAX_PORT 65535                              //Maximum port number supported by OS
#define MAX_THREADS 100                             //Maximum number of threads
#define MAX_QUEUE_LEN 100                           //Maximum queue length
#define MAX_CE 100                                  //Maximum cache size
#define INVALID -1                                  //Reusable int for marking things as invalid or incorrect
#define INVALID_FLAG 99999                          //Reusable flag to marking a flag as invalid
#define BUFF_SIZE 1024                              //Maximum file path length to make things easier
#define LOG_FILE_NAME "webserver_log"               //Standardized log file name

/********************* [ Helpful Typedefs        ] ************************/

typedef struct request_queue {
  int fd;
  char *request;  // Now we will make URL dynamic in length (unlike project 2)
} request_t;

typedef struct cache_entry {
  int len;
  char *request; // The URL
  char *content; // The bytes of the file  
} cache_entry_t;

/********************* [ Function Prototypes       ] **********************/
int     getCacheIndex(char *request);
void    addIntoCache(char *mybuf, char *memory , int memory_size);
void    deleteCache();
void    initCache();
char*   getContentType(char * mybuf);
int     readFromDisk(int fd, char *mybuf, void **memory);
void *  dispatch(void *arg);
void *  worker(void *arg);

/********************* [ Static Helper Functions    ] **********************/


/**********************************************
 * LogPrettyPrint
   - parameters:
      - to_write is expected to be an open file pointer, or it 
        can be NULL which means that the output is printed to the terminal
      - All other inputs are self explanatory or specified in the writeup
   - returns:
       - no return value
************************************************/
void LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit){
    if(to_write == NULL){
        printf("[%3d] [%3d] [%3d] [%-30s] [%7d] [%5s]\n", threadId, requestNumber, file_descriptor, request_str, num_bytes_or_error, cache_hit? "TRUE" : "FALSE");
        fflush(stdout);
    }else{
        fprintf(to_write, "[%3d] [%3d] [%3d] [%-30s] [%7d] [%5s]\n", threadId, requestNumber, file_descriptor, request_str, num_bytes_or_error, cache_hit? "TRUE" : "FALSE");
        fflush(to_write);
    }    
}
#endif /* _SERVER_H */
