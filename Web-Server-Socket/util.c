#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

int master_fd = -1;
pthread_mutex_t accept_con_mutex = PTHREAD_MUTEX_INITIALIZER;

#define CON_NUM 20	// Number of Connections Allowed
#define MAXBUF 2048 // Maximum size for buffer

/**********************************************
	* init
	- port is the number of the port you want the server to be
		started on
	- initializes the connection acception/handling system
	- if init encounters any errors, it will call exit().
************************************************/
void init(int port)
{
	int sd;
	struct sockaddr_in addr;
	//    int ret_val;
	//    int flag;

	// Create a socket and save the file descriptor to sd (declared above)
	// This socket should be for use with IPv4 and for a TCP connection.
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if (sd == -1)
	{
		perror("socket error");
		exit(1);
	}

	// Change the socket options to be reusable using setsockopt().
	int enable = 1;
	if ((setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&enable, sizeof(int))) == -1)
	{
		perror("setsockopt error");
		exit(1);
	}

	// Bind the socket to the provided port.
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		perror("bind error");
		exit(1);
	}

	// Mark the socket as a passive socket. (ie: a socket that will be used to receive connections)
	// Receive up to 20 connections
	if (listen(sd, CON_NUM) != 0)
	{
		perror("listen error");
		exit(1);
	}

	// We save the file descriptor to a global variable so that we can use it in accept_connection().
	master_fd = sd;
	printf("UTILS.O: Server Started on Port %d\n", port);
}

/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
	 DO NOT use the file descriptor on your own -- use
	 get_request() instead.
   - if the return value is negative, the thread calling
	 accept_connection must should ignore request.
***********************************************/
int accept_connection(void)
{
	int newsock;
	struct sockaddr_in new_recv_addr;
	uint addr_len;
	addr_len = sizeof(new_recv_addr);

	// Aquire the mutex lock
	pthread_mutex_lock(&accept_con_mutex);

	// Accept a new connection on the passive socket and save the fd to newsock
	newsock = accept(master_fd, (struct sockaddr *)&new_recv_addr, &addr_len);
	if (newsock == -1)
	{
		perror("accept error");
		exit(0);
	}

	// Release the mutex lock
	pthread_mutex_unlock(&accept_con_mutex);

	// Return the file descriptor for the new client connection
	return newsock;
}

/**********************************************
 * get_request
   - parameters:
	  - fd is the file descriptor obtained by accept_connection()
		from where you wish to get a request
	  - filename is the location of a character buffer in which
		this function should store the requested filename. (Buffer
		should be of size 1024 bytes.)
   - returns 0 on success, nonzero on failure. You must account
	 for failures because some connections might send faulty
	 requests. This is a recoverable error - you must not exit
	 inside the thread that called get_request. After an error, you
	 must NOT use a return_request or return_error function for that
	 specific 'connection'.
************************************************/
int get_request(int fd, char *filename)
{

	char buf[MAXBUF];

	// Read the request from the file descriptor into the buffer
	if (read(fd, buf, 2047) == -1)
	{
		perror("read error");
		return -1;
	}
	// NULL Terminator for the last char.
	buf[2047] = '\0';

	char s = '\n';
	char *token;
	token = strtok(buf, &s);
	if (token == NULL)
	{
		perror("strtok error");
		return -1;
	}
	printf("Get Request: %s\n", token);

	// Ensure that the incoming request is a properly formatted HTTP "GET" request
	// The first line of the request must be of the form: GET <file name> HTTP/1.0
	// or: GET <file name> HTTP/1.1

	char space[] = " ";
	char *ptr = strtok(token, space);
	if (ptr == NULL)
	{
		perror("strtok error");
		return -1;
	}

	if (strcmp(ptr, "GET") != 0)
	{
		perror("Wrong format1, not \"GET\"");
		return -1;
	}

	// Extract the file name from the request
	ptr = strtok(NULL, space);
	if (ptr == NULL)
	{
		perror("strtok error");
		return -1;
	}
	char fname[1024];
	strcpy(fname, ptr);
	ptr = strtok(NULL, space);
	if (ptr == NULL)
	{
		perror("strtok error");
		return -1;
	}

	// HTTP/1.0 or HTTP/1.1
	if ((strncmp(ptr, "HTTP/1.0", 8) != 0) && (strncmp(ptr, "HTTP/1.1", 8) != 0))
	{
		perror("Wrong format2, not HTTP/1.0 or HTTP/1.1");
		return -1;
	}

	// Ensure the file name does not contain with ".." or "//"
	// FILE NAMES WHICH CONTAIN ".." OR "//" ARE A SECURITY THREAT AND MUST NOT BE ACCEPTED!!!

	if (strstr(fname, "..") != NULL || strstr(fname, "//") != NULL)
	{
		perror("SECURITY THREAT!");
		return -1;
	}

	// Copy the file name to the provided buffer
	strcpy(filename, fname);

	return 0;
} // get_request

/**********************************************
 * return_result
   - returns the contents of a file to the requesting client
   - parameters:
	  - fd is the file descriptor obtained by accept_connection()
		to where you wish to return the result of a request
	  - content_type is a pointer to a string that indicates the
		type of content being returned. possible types include
		"text/html", "text/plain", "image/gif", "image/jpeg" cor-
		responding to .html, .txt, .gif, .jpg files.
	  - buf is a pointer to a memory location where the requested
		file has been read into memory (the heap). return_result
		will use this memory location to return the result to the
		user. (remember to use -D_REENTRANT for CFLAGS.) you may
		safely deallocate the memory after the call to
		return_result (if it will not be cached).
	  - numbytes is the number of bytes the file takes up in buf
   - returns 0 on success, nonzero on failure.
************************************************/
int return_result(int fd, char *content_type, char *buf, int numbytes)
{

	// NOTE: The items above in angle-brackes <> are placeholders. The file length should be a number
	// and the content type is a string which is passed to the function.

	/* EXAMPLE HTTP RESPONSE
	 *
	 * HTTP/1.0 200 OK
	 * Content-Length: <content length>
	 * Content-Type: <content type>
	 * Connection: Close
	 *
	 * <File contents>
	 */

	// Prepare the headers for the response you will send to the client.
	char header[MAXBUF];
	char num_bytes[100];
	char contenttype[100];

	// REQUIRED: The first line must be "HTTP/1.0 200 OK"
	sprintf(header, "HTTP/1.0 200 OK\n");

	// REQUIRED: Must send a line with the header "Content-Length: <file length>"
	sprintf(num_bytes, "Content-Length: %d\n", numbytes);
	strcat(header, num_bytes);

	// REQUIRED: Must send a line with the header "Content-Type: <content type>"
	sprintf(contenttype, "Content-Type: %s\n", content_type);
	strcat(header, contenttype);

	// REQUIRED: Must send a line with the header "Connection: Close"
	strcat(header, "Connection: Close\n");

	// Add an extra new-line to the end.
	strcat(header, "\n");

	// Send the HTTP headers to the client
	if (write(fd, header, strlen(header)) < 0)
	{
		perror("write error");
		return -1;
	}

	// Send the file contents to the client
	if (write(fd, buf, numbytes) < 0)
	{
		perror("write error");
		return -1;
	}

	// Close the connection to the client
	if (close(fd) != 0)
	{
		perror("close error");
		return -1;
	}

	return 0;
}

/**********************************************
 * return_error
   - returns an error message in response to a bad request
   - parameters:
	  - fd is the file descriptor obtained by accept_connection()
		to where you wish to return the error
	  - buf is a pointer to the location of the error text
   - returns 0 on success, nonzero on failure.
************************************************/
int return_error(int fd, char *buf)
{

	// NOTE: In this case, the content is what is passed to you in the argument "buf". This represents
	// a server generated error message for the user. The length of that message should be the content-length.

	// Prepare the headers to send to the client
	char header[MAXBUF];
	char num_bytes[100];

	// REQUIRED: First line must be "HTTP/1.0 404 Not Found"
	sprintf(header, "HTTP/1.0 404 Not Found\n");

	// REQUIRED: Must send a header with the line: "Content-Length: <content length>"
	int length = strlen(buf);
	sprintf(num_bytes, "Content-Length: %d\n", length);
	strcat(header, num_bytes);

	// REQUIRED: Must send a header with the line: "Connection: Close"
	strcat(header, "Connection: Close\n");

	// IMPORTANT: Similar to sending a file, there must be a blank line between the headers and the content.
	strcat(header, "\n");

	/* EXAMPLE HTTP ERROR RESPONSE
	 *
	 * HTTP/1.0 404 Not Found
	 * Content-Length: <content length>
	 * Connection: Close
	 *
	 * <Error Message>
	 */

	// Send headers to the client
	if (write(fd, header, strlen(header)) < 0)
	{
		perror("write error");
		return -1;
	};

	// Send the error message to the client
	if (write(fd, buf, length) < 0)
	{
		perror("write error");
		return -1;
	}

	// Close the connection with the client.
	if (close(fd) < 0)
	{
		perror("close error");
		return -1;
	}

	return 0;
}
