#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <gtk/gtk.h>

// Function Definitions
void new_tab_created_cb(GtkButton *button, gpointer data);
int run_control();
int on_blacklist(char *uri);
int bad_format(char *uri);
void uri_entered_cb(GtkWidget *entry, gpointer data);
void init_blacklist(char *fname);

// Global Definitions
#define MAX_TAB 100  // Maximum number of tabs allowed
#define MAX_BAD 1000 // Maximum number of URL's in blacklist allowed
#define MAX_URL 100  // Maximum char length of a url allowed

int tabs;
char blackList[2 * MAX_BAD][MAX_URL];
int blackListLength = 0;
int pids[MAX_TAB];

/*
 * Name:		          new_tab_created_cb
 *
 * Input arguments:
 *      'button'      - whose click generated this callback
 *			'data'        - auxillary data passed along for handling
 *			                this event.
 *
 * Output arguments:   void
 *
 * Description:        This is the callback function for the 'create_new_tab'
 *			               event which is generated when the user clicks the '+'
 *			               button in the controller-tab. The controller-tab
 *			               redirects the request to the parent (/router) process
 *			               which then creates a new child process for creating
 *			               and managing this new tab.
 */
// NO-OP for now
void new_tab_created_cb(GtkButton *button, gpointer data)
{
}

/*
 * Name:                run_control
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminates.
 */
int run_control()
{
    // (a) Init a browser_window object
    browser_window *b_window = NULL;

    // (b) Create controller window with callback function
    create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
                   G_CALLBACK(uri_entered_cb), &b_window);

    // (c) enter the GTK infinite loop
    show_browser();
    return 0;
}

/*
    Function: on_blacklist  --> "Check if the provided URI is in th blacklist"
    Input:    char* uri     --> "URI to check against the blacklist"
    Returns:  True  (1) if uri in blacklist
              False (0) if uri not in blacklist
    Hints:
            (a) File I/O
            (b) Handle case with and without "www." (see writeup for details)
            (c) How should you handle "http://" and "https://" ??
*/
int on_blacklist(char *uri)
{
    /*
     We don't need to check for empty uri case because we only call on_blacklist in
     uri_entered_cb and it is already been checked for "good format"
     So we assume uri is in "good format"
     return 1 if on blacklist, return 0 if not on blacklist
     return -1 if the blacklist file does not exit
    */
    // Open the file
    FILE *fp;
    fp = fopen("blacklist", "r");

    if (fp == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    // Use strtok() to get rid of http's
    char bar[2] = "//";
    char *token = strtok(uri, bar);
    token = strtok(NULL, bar);
    // Token should have the uri begins w/o www.

    for (int i = 0; i < blackListLength; i++)
    {
        char *blackListElement = blackList[i];
        if (strcmp(blackListElement, token) == 0)
        {
            // Do not use alert() here, it will crash the program
            printf("The website %s is on the blacklist\n", blackListElement);
            return 1;
        } // if
    } // for
    // file close and return
    fclose(fp);
    return 0;
}

/*
    Function: bad_format    --> "Check for a badly formatted url string"
    Input:    char* uri     --> "URI to check if it is bad"
    Returns:  True  (1) if uri is badly formatted
              False (0) if uri is well formatted
    Hints:
              (a) String checking for http:// or https://
*/
int bad_format(char *uri)
{
    /*
    return 0 if it is "good format", return 1 if it is bad(no http:// or https://)
    if the uri is empty, we will automatically return 1
    */
    if (strncmp(uri, "http://", 7) == 0 || strncmp(uri, "https://", 8) == 0)
    {
        if (strcmp(uri, "http://") == 0 || strcmp(uri, "https://") == 0)
        {
            return 1;
        }
        return 0;
    }
    return 1;
}

/*
 * Name:                uri_entered_cb
 *
 * Input arguments:
 *                      'entry'-address bar where the url was entered
 *			                'data'-auxiliary data sent along with the event
 *
 * Output arguments:     void
 *
 * Function:             When the user hits the enter after entering the url
 *			                 in the address bar, 'activate' event is generated
 *			                 for the Widget Entry, for which 'uri_entered_cb'
 *			                 callback is called. Controller-tab captures this event
 *			                 and sends the browsing request to the router(/parent)
 *			                 process.
 * Hints:
 *                       (a) What happens if data is empty? No Url passed in? Handle that
 *                       (b) Get the URL from the GtkWidget (hint: look at wrapper.h)
 *                       (c) Print the URL you got, this is the intermediate submission
 *                       (d) Check for a bad url format THEN check if it is in the blacklist
 *                       (e) Check for number of tabs! Look at constraints section in lab
 *                       (f) Open the URL, this will need some 'forking' some 'execing' etc.
 */
void uri_entered_cb(GtkWidget *entry, gpointer data)
{
    // Get the entered URL

    char userURI[MAX_URL];
    strcpy(userURI, get_entered_uri(entry));

    // Check whether URI entered is empty
    if (strcmp(userURI, "") == 0)
    {
        alert("Entry was null");
        return;
    }

    // Print Entered URI
    printf("URI Entered: %s\n", userURI);

    // Check for a bad url format
    if (bad_format(userURI))
    {
        alert("BAD FORMAT\n");
        return;
    }
    // Check if in the blacklist
    if (on_blacklist(userURI))
    {
        alert("BLACKLIST\n");
        return;
    }
    strcpy(userURI, get_entered_uri(entry));
    // Check for number of tabs
    if (tabs >= MAX_TAB)
    {
        alert("MAX TAB REACHED\n");
        return;
    }

    pid_t child = fork();
    if (child > 0)
    {
        tabs++;

        // Wait for children
        waitpid(tabs, NULL, 0);
        pids[tabs] = child;
    }
    if (child == 0)
    {
        char tabstr[4];
        sprintf(tabstr, "%d", tabs);

        // Render the tab
        execl("./render", "render", tabstr, userURI, NULL);
    }
    if (child < 0)
    {
        printf("Fork Created Failed\n");
        exit(-1);
    }

    return;
}

/*
    Function: init_blacklist --> "Open a passed in filepath to a blacklist of url's, read and parse into an array"
    Input:    char* fname    --> "file path to the blacklist file"
    Returns:  void
    Hints:
            (a) File I/O (fopen, fgets, etc.)
            (b) If we want this list of url's to be accessible elsewhere, where do we put the array?
*/
void init_blacklist(char *fname)
{
    /*
    Created a global array named blackList
    char blackList[MAX_BAD][MAX_URL]; this is declared elsewhere
    The idea is, whenever we see a website, we add two version of it to our array
    One is with www. one is without
    Assumption is the website in blacklist file does not start with https:// or http://
    */
    FILE *fp;
    fp = fopen(fname, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return;
    }

    // Token should have the uri begins w/o www.

    const char *readInLine = fgets(blackList[blackListLength], MAX_URL, fp);

    while (readInLine != NULL)
    {
        blackListLength++;
        const char www[MAX_URL] = "www.";
        if (strncmp(readInLine, www, 4) == 0)
        {
            char subToken[MAX_URL]; // subtoken is blacklist without www.
            strncpy(subToken, &readInLine[4], MAX_URL);
            strncpy(blackList[blackListLength], subToken, MAX_URL);
            blackListLength++;
        } // we add another version of the website to our array
        else
        {
            char wwwToken[MAX_URL] = "www.";
            strcat(wwwToken, readInLine);
            strncpy(blackList[blackListLength], wwwToken, MAX_URL);
            blackListLength++;
        } // else readInLine does not start with www. So we add one start with www.
        readInLine = fgets(blackList[blackListLength], MAX_URL, fp);
    } // while
    fclose(fp);
    /*
        The fgets will read in new line character as well
        so I need to manually remove all the \n
    */
    for (int i = 0; i < blackListLength; i++)
    {
        blackList[i][strlen(blackList[i]) - 1] = '\0';
    } // for
    return;
}

/*
    Function: main
    Hints:
            (a) Check for a blacklist file as argument, fail if not there [PROVIDED]
            (b) Initialize the blacklist of url's
            (c) Create a controller process then run the controller
                (i)  What should controller do if it is exited? Look at writeup (KILL! WAIT!)
            (d) Parent should not exit until the controller process is done
*/
int main(int argc, char **argv)
{
    // (a) Check arguments for blacklist, error and warn if no blacklist
    if (argc != 2)
    {
        fprintf(stderr, "browser <blacklist_file>\n");
        exit(0);
    }

    // Warn if no blacklist
    if (strcmp(argv[1], "blacklist") != 0)
    {
        perror("blacklist not found\n");
        return -1;
    }
    // (b) Initialize the blacklist
    init_blacklist(argv[1]);
    tabs = 0;

    // Create a controller process fork
    pid_t child = fork();
    if (child > 0)
    {
        wait(NULL);
    }

    // Run the controller
    if (child == 0)
    {
        run_control();

        // Kill all children tabs
        for (int i = 0; i < tabs; i++)
        {
            kill(pids[i], SIGKILL);
        }
    }
    else
    {
        printf("controller process fail\n");
        exit(-1);
    }

    return 0;
}
