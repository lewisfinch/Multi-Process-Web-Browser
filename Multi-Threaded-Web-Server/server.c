#include "server.h"
#define PERM 0644

// Global Variables [Values Set in main()]
int queue_len = INVALID;      // Global integer to indicate the length of the queue
int cache_len = INVALID;      // Global integer to indicate the length or # of entries in the cache
int num_worker = INVALID;     // Global integer to indicate the number of worker threads
int num_dispatcher = INVALID; // Global integer to indicate the number of dispatcher threads
FILE *logfile;                // Global file pointer for writing to log file in worker

/* ************************ Global Hints **********************************/

int cacheIndex = 0;      //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
int workerIndex = 0;     //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0; //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int queue_rear = -1;     // --> keep track of the rear of circular queue
int queue_front = -1;    // --> keep track of the front of circular queue
int queue_count = 0;     // --> keep track of the number of requests in the circular queue

pthread_t worker_thread[MAX_THREADS];     //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS]; //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int threadID[MAX_THREADS];                //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array

pthread_mutex_t req_lock = PTHREAD_MUTEX_INITIALIZER; // What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER; // Lock to make cache thread-safe.
pthread_cond_t some_content = PTHREAD_COND_INITIALIZER; // What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t free_space = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN]; // How will you track the requests globally between threads? How will you ensure this is thread safe?

cache_entry_t *cacheEntry; //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global

/**********************************************************************************/

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/

/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request)
{
    /*   (GET CACHE INDEX)
     *    Description:      return the index if the request is present in the cache otherwise return INVALID
     */
    // loop through cacheEntry to find the request if there. lock for thread-safe.
    for (int i = 0; i < cache_len; i++)
    {
        if ((cacheEntry[i].request != NULL) && (strcmp(cacheEntry[i].request, request) == 0))
        {
            return i;
        }
    }

    return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory, int memory_size)
{
    // Check if cache is full, time to evict!
    if (cacheIndex == cache_len)
    {
        cacheIndex = 0;
    }

    // check request
    if (cacheEntry[cacheIndex].request != NULL)
    {
        free(cacheEntry[cacheIndex].request);
    }
    if ((cacheEntry[cacheIndex].request = malloc(strlen(mybuf + 1))) == NULL)
    {
        perror("malloc failed\n");
    }
    strcpy(cacheEntry[cacheIndex].request, mybuf);

    // check memory / content
    if (cacheEntry[cacheIndex].content != NULL)
    {
        free(cacheEntry[cacheIndex].content);
    }
    if ((cacheEntry[cacheIndex].content = malloc(memory_size)) == NULL)
    {
        perror("malloc failed\n");
    }
    memcpy(cacheEntry[cacheIndex].content, memory, memory_size);

    // update cacheIndex
    cacheEntry[cacheIndex].len = memory_size;
    cacheIndex++;
}

// Function to clear the memory allocated to the cache
void deleteCache()
{
    // do the free work!
    for (int i = 0; i < cache_len; i++)
    {
        free(&cacheEntry[i]);
    }

    // free the cacheEntry
    free(cacheEntry);
}

// Function to initialize the cache
void initCache()
{
    // malloc cache
    cacheEntry = malloc(cache_len * sizeof(cache_entry_t));
    if (cacheEntry == NULL)
    {
        perror("malloc error");
    }
    // initialize all cache entries with a length of -1, indicate NULL.
    for (int i = 0; i < cache_len; i++)
    {
        cacheEntry[i].len = -1;
        cacheEntry[i].content = NULL;
        cacheEntry[i].request = NULL;
    }
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char *getContentType(char *mybuf)
{
    // use of strrchr() to get the last dot in the request
    char *loc;
    loc = strrchr(mybuf, '.');

    // error check, locate our file type
    if (loc == NULL)
    {
        perror("strrchr error");
    }
    loc++;

    // A nested if-else statement to determine the content-type
    static char type[11];
    if (!strcmp(loc, "html") || !strcmp(loc, "htm"))
    {
        strcpy(type, "text/html");
    }
    else if (!strcmp(loc, "jpg"))
    {
        strcpy(type, "image/jpeg");
    }
    else if (!strcmp(loc, "gif"))
    {
        strcpy(type, "image/gif");
    }
    else
    {
        strcpy(type, "text/plain");
    }

    // return the content-type
    return type;
}

// Function to open and read the file from the disk into the memory. Add necessary arguments as needed
int readFromDisk(int fd, char *mybuf, void **memory)
{
    // open file
    int fp;
    if ((fp = open(mybuf + 1, O_RDONLY)) == -1)
    {
        fprintf(stderr, "ERROR: Fail to open the file.\n");
        return INVALID;
    }

    // get file and stat
    struct stat file;
    if (stat(mybuf + 1, &file) != 0)
    {
        perror("Access file failed");
    }
    int size = file.st_size;

    // malloc memory
    *memory = malloc(size);
    if (*memory == NULL)
    {
        perror("malloc error in readFromDisk");
        return INVALID;
    }
    if ((read(fp, (void *)*memory, size)) == -1)
    {
        perror("read error");
    }

    // file close
    if (close(fp) == EOF)
    {
        perror("Close file falied");
        return INVALID;
    }
    return size;
}

// Function to receive the path)request from the client and add to the queue
void *dispatch(void *arg)
{

    /********************* DO NOT REMOVE SECTION - TOP     *********************/
    request_t new_req;

    while (1)
    {
        /*   (B.II)
         *    Description:      Accept client connection
         *    Utility Function: int accept_connection(void) //utils.h => Line 24
         */
        int fd = accept_connection();

        // error check, ignore the request.
        if (fd < 0)
        {
            perror("accept_connection error.");
        }

        /*   (B.III)
         *    Description:      Get request from the client
         *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
         */
        char filename[BUFF_SIZE];
        int ret = get_request(fd, filename);
        if (ret != 0)
        {
            perror("Failed to get request.");
        } // error check

        /*   (B.IV)
         *    Description:      Add the request into the queue
         */
        // receive a single request and print the contents
        new_req.fd = fd;
        new_req.request = malloc(sizeof(char) * strlen(filename));
        if (new_req.request == NULL)
        {
            perror("malloc error for new_req.request in dispatcher thread");
        }
        strcpy(new_req.request, filename);

        //(2) Request thread safe access to the request queue
        pthread_mutex_lock(&req_lock);

        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull
        while (queue_count == MAX_QUEUE_LEN)
        {
            pthread_cond_wait(&free_space, &req_lock);
        }

        //(4) Insert the request into the queue
        //(5) Update the queue index in a circular fashion
        if (queue_front == -1)
        {
            queue_front = 0;
        }
        queue_rear = (queue_rear + 1) % queue_len;
        req_entries[queue_rear].fd = fd;
        req_entries[queue_rear].request = malloc(sizeof(char) * strlen(filename));
        if (req_entries[queue_rear].request == NULL)
        {
            perror("malloc error for req_entries[queue_rear].request in dispatcher thread");
        }

        strcpy(req_entries[queue_rear].request, filename);
        free(new_req.request);
        queue_count++;

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
        pthread_cond_signal(&some_content);
        pthread_mutex_unlock(&req_lock);

    } // While loop

    return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void *worker(void *arg)
{
    /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/
    // Helpful/Suggested Declarations
    int num_request = 0;    // Integer for tracking each request for printing into the log
    bool cache_hit = false; // Boolean flag for tracking cache hits or misses if doing
    int filesize = 0;       // Integer for holding the file size returned from readFromDisk or the cache
    void *memory = NULL;    // memory pointer where contents being requested are read and stored
    int fd = INVALID;       // Integer to hold the file descriptor of incoming request
    char mybuf[BUFF_SIZE];  // String to hold the file path from the request
    int id = -1;            // Integer for tracking worker id

    /*   (C.I)
     *    Description:      Get the id as an input argument from arg, set it to ID
     */
    id = *(int *)arg;

    while (1)
    {
        /*   (C.II)
         *    Description:      Get the request from the queue and do as follows
         */
        //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
        pthread_mutex_lock(&req_lock);

        //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
        while (queue_count <= 0)
        {
            pthread_cond_wait(&some_content, &req_lock);
        }

        //(3) Now that you have the lock AND the queue is not empty, read from the request queue
        // Get the request (file path) and fd from queue
        if (req_entries[queue_front].request == NULL)
        {
            perror("NULL pointer1");
        }
        strcpy(mybuf, req_entries[queue_front].request);
        fd = req_entries[queue_front].fd;
        num_request++;

        //(4) Update the request queue remove index in a circular fashion
        // if only one thing in the queue and solved, we reset the queue.
        req_entries[queue_front].fd = 0;
        req_entries[queue_front].request = NULL;
        if (queue_front == queue_rear)
        {
            queue_front = -1;
            queue_rear = -1;
            queue_count = 0;
        }
        else
        {
            queue_front = (queue_front + 1) % queue_len;
            queue_count--;
        }

        //(5) Check for a path with only a "/" if that is the case add index.html to it
        if (strcmp(mybuf, "/") == 0)
        {
            strcat(mybuf, "index.html");
        }

        //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
        free(req_entries[queue_front].request);
        pthread_cond_signal(&free_space);
        pthread_mutex_unlock(&req_lock);

        /*   (C.III)
         *    Description:      Get the data from the disk or the cache
         *    Local Function:   int readFromDisk(//necessary arguments//);
         *                      int getCacheIndex(char *request);
         *                      void addIntoCache(char *mybuf, char *memory , int memory_size);
         */

        // Check the cache first!
        int hit = getCacheIndex(mybuf);
        if (hit == INVALID)
        {
            // get the data from the disk and add to the cache
            cache_hit = false;
            filesize = readFromDisk(fd, mybuf, &memory);
            pthread_mutex_lock(&cache_lock);
            addIntoCache(mybuf, memory, filesize);
            pthread_mutex_unlock(&cache_lock);
        }
        else
        {
            // we found it in the cache!
            cache_hit = true;
            filesize = cacheEntry[hit].len;
            memory = malloc(filesize);
            if (memory == NULL)
            {
                perror("malloc error");
            }
            memcpy(memory, cacheEntry[hit].content, filesize);
        }

        /*   (C.IV)
         *    Description:      Log the request into the file and terminal
         */
        // log lock
        pthread_mutex_lock(&log_lock);

        LogPrettyPrint(NULL, id, num_request, fd, mybuf, filesize, cache_hit);
        LogPrettyPrint(logfile, id, num_request, fd, mybuf, filesize, cache_hit);

        // log unlock
        pthread_mutex_unlock(&log_lock);

        /*   (C.V)
         *    Description:      Get the content type and return the result or error
         */
        char *conType = getContentType(mybuf);

        if (return_result(fd, conType, memory, filesize) != 0)
        {
            return_error(fd, mybuf);
        }
    }

    return NULL;
}

/**********************************************************************************/

int main(int argc, char **argv)
{

    /********************* DO NOT REMOVE SECTION - TOP     *********************/
    // Error check on number of arguments
    if (argc != 7)
    {
        printf("usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
        return -1;
    }

    int port = -1;
    char path[PATH_MAX] = "no path set\0";
    num_dispatcher = -1; // global variable
    num_worker = -1;     // global variable
    queue_len = -1;      // global variable
    cache_len = -1;      // global variable

    /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/
    /*   (A.I)
     *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length (6) cache_size
     */
    port = atoi(argv[1]);
    strcpy(path, argv[2]);
    num_dispatcher = atoi(argv[3]);
    num_worker = atoi(argv[4]);
    queue_len = atoi(argv[5]);
    cache_len = atoi(argv[6]);

    /*   (A.II)
     *    Description:     Perform error checks on the input arguments
     *    Hints:           (1) port: {Should be >= MIN_PORT and <= MAX_PORT} | (2) path: {Consider checking if path exists (or will be caught later)}
     *                     (3) num_dispatcher: {Should be >= 1 and <= MAX_THREADS} | (4) num_workers: {Should be >= 1 and <= MAX_THREADS}
     *                     (5) queue_length: {Should be >= 1 and <= MAX_QUEUE_LEN} | (6) cache_size: {Should be >= 1 and <= MAX_CE}
     */
    if (port < MIN_PORT || port > MAX_PORT)
    {
        perror("port error");
    }
    if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS)
    {
        perror("num_dispatcher error");
    }
    if (num_worker < 1 || num_worker > MAX_THREADS)
    {
        perror("num_worker error");
    }
    if (queue_len < 1 || queue_len > MAX_QUEUE_LEN)
    {
        perror("queue_len error");
    }
    if (cache_len < 1 || cache_len > MAX_CE)
    {
        perror("cache_len error");
    }

    /********************* DO NOT REMOVE SECTION - TOP    *********************/
    printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n",
           port, path, num_dispatcher, num_worker, queue_len, cache_len);
    /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/

    /*   (A.III)
     *    Description:      Open log file
     *    Hint:             Use Global "File* logfile", use "web_server_log" as the name, what open flags do you want? --> r+
     */
    logfile = fopen("web_server_log", "a");
    if (logfile == NULL)
    {
        perror("logfile not exist");
    }

    /*   (A.IV)
     *    Description:      Change the current working directory to server root directory
     *    Hint:             Check for error!
     */
    if (chdir(path) != 0)
    {
        perror("Change to root directory failed");
    }

    /*   (A.V)
     *    Description:      Initialize cache
     *    Local Function:   void    initCache();
     */
    initCache();

    /*   (A.VI)
     *    Description:      Start the server
     *    Utility Function: void init(int port); //look in utils.h
     */
    init(port);

    /*   (A.VII)
     *    Description:      Create dispatcher and worker threads
     *    Hints:            Use pthread_create, you will want to store pthread's globally
     *                      You will want to initialize some kind of global array to pass in thread ID's
     *                      How should you track this p_thread so you can terminate it later? [global]
     */
    int arg_arr[num_worker];
    for (int i = 0; i < num_worker; i++)
    {
        arg_arr[i] = i;
    }

    for (int i = 0; i < num_worker; i++)
    {
        if ((pthread_create(&worker_thread[i], NULL, worker, (void *)&arg_arr[i])) == 0)
        {
            printf("Worker                              [%d] Started\n", i);
        }
        else
        {
            printf("Fail to create worker thread %d", i);
        }
    }
    for (int i = 0; i < num_dispatcher; i++)
    {
        if ((pthread_create(&dispatcher_thread[i], NULL, dispatch, (void *)&arg_arr[i])) == 0)
        {
            printf("Dispatcher                          [%d] Started\n", i);
        }
        else
        {
            printf("Fail to create dispatcher thread %d", i);
        }
    }

    // Wait for each of the threads to complete their work
    // Threads (if created) will not exit (see while loop), but this keeps main from exiting
    int i;

    for (i = 0; i < num_dispatcher; i++)
    {
        fprintf(stderr, "JOINING DISPATCHER %d \n", i);
        if ((pthread_join(dispatcher_thread[i], NULL)) != 0)
        {
            printf("ERROR : Fail to join dispatcher thread %d.\n", i);
        }
    }

    for (i = 0; i < num_worker; i++)
    {
        fprintf(stderr, "JOINING WORKER %d \n", i);
        if ((pthread_join(worker_thread[i], NULL)) != 0)
        {
            printf("ERROR : Fail to join worker thread %d.\n", i);
        }
    }
    fprintf(stderr, "SERVER DONE \n"); // will never be reached in SOLUTION
}
