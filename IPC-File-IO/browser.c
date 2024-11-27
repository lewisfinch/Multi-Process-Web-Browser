#include "wrapper.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TABS 100 // this gives us 99 tabs, 0 is reserved for the controller
#define MAX_BAD 1000
#define MAX_URL 100
#define MAX_FAV 100
#define MAX_LABELS 100

comm_channel comm[MAX_TABS];      // Communication pipes
char favorites[MAX_FAV][MAX_URL]; // Maximum char length of a url allowed
int num_fav = 0;                  // # favorites
int tabsNum;                      // Keep track of the number of tabs and the free index.

typedef struct tab_list
{
  int free;
  int pid; // may or may not be useful
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];

/************************/
/* Simple tab functions */
/************************/

// return total number of tabs
int get_num_tabs()
{
  return tabsNum; // tabsNum represent how many tabs we own
}

// get next free tab index
int get_free_tab()
{
  for (int i = 1; i < MAX_TABS; i++)
  {
    if (TABS[i].free == 1)
    {
      return i;
    }
  }
  return -1;
  /*
  The above code will traverse through the TABS[] array and find the first
  free tab index, and return the index (start at 1)
  */
}

// init TABS data structure
void init_tabs()
{
  for (int i = 1; i < MAX_TABS; i++)
  {
    TABS[i].free = 1;
  }
  // The first free tab should have the index of 1.
  tabsNum = 1;
  TABS[0].free = 0;
  /*
  all tabs are free except the first one which
  reserves for controller
  */
}

/***********************************/
/* Favorite manipulation functions */
/***********************************/

// return 0 if favorite is ok, -1 otherwise
// both max limit, already a favorite (Hint: see util.h) return -1
int fav_ok(char *uri)
{
  if (num_fav >= MAX_FAV)
  {
    alert("HIT MAX FAV");
    return -1;
  } // if fav length hit max or uri is already on the favorites list, return -1
  if (on_favorites(uri) == 1)
  {
    alert("FAV EXISTS");
    return -1;
  }
  return 0;
}

// Add uri to favorites file and update favorites array with the new favorite
void update_favorites_file(char *uri)
{
  FILE *fp;
  fp = fopen(".favorites", "a");
  if (fp == NULL)
  {
    perror("Error opening file");
    return;
  } // error check
  strcpy(favorites[num_fav], uri);
  int ep = fprintf(fp, "%s\n", uri);
  if (ep < 0)
  {
    perror("fprintf error\n");
    exit(1);
  } // error check
  num_fav++;
  fclose(fp);
}

// Set up favorites array
void init_favorites(char *fname)
{
  FILE *fp;
  fp = fopen(fname, "r");
  if (fp == NULL)
  {
    perror("Error opening file");
    return;
  }

  while (fgets(favorites[num_fav], MAX_URL, fp) != NULL)
  {
    num_fav++;
  } // while
  fclose(fp);
  /*
      We use while loop to read in favorites line by line, and
      the fgets() will read in new line character as well
      so I need to manually remove all the \n
  */
  for (int i = 0; i < num_fav; i++)
  {
    favorites[i][strlen(favorites[i]) - 1] = '\0';
  } // for
  return;

} // init_favorites

// Make fd non-blocking just as in class!
// Return 0 if ok, -1 otherwise
// Really a util but I want you to do it :-)
int non_block_pipe(int fd)
{
  int nFlags;
  if ((nFlags = fcntl(fd, F_GETFL, 0)) < 0)
    return -1;
  if ((fcntl(fd, F_SETFL, nFlags | O_NONBLOCK)) < 0)
    return -1;
  return 0;
} // non_block_pipe

/***********************************/
/* Functions involving commands    */
/***********************************/

// Checks if tab is bad and url violates constraints; if so, return.
// Otherwise, send NEW_URI_ENTERED command to the tab on inbound pipe
void handle_uri(char *uri, int tab_index)
{
  if (bad_format(uri) || on_blacklist(uri))
  {
    return;
  } // check if tab is bad and url violates constraints
  req_t req = {NEW_URI_ENTERED, tab_index};
  strcpy(req.uri, uri);

  int er = write(comm[tab_index].inbound[1], &req, sizeof(req_t));
  if (er == -1)
  {
    perror("write error\n");
    exit(1);
  } // error check
  // close read-end and write-end is open: write kills the process, not gonna return anything
}

// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
// Short function
void uri_entered_cb(GtkWidget *entry, gpointer data)
{

  if (data == NULL)
  {
    return;
  }
  char *uri;
  int tab_index;

  // Get the tab
  tab_index = query_tab_id_for_request(entry, data);
  if (tab_index == 0 || TABS[tab_index].free == 1)
  {
    alert("BAD TAB");
    return;
  } // check if tab_index is empty(when it is 0), or it is a undeclared tab
  // Get the URL
  uri = get_entered_uri(entry);
  if (bad_format(uri))
  {
    alert("BAD FORMAT");
    return;
  } // check for bad format(withour https:// or http://)
  if (on_blacklist(uri))
  {
    alert("BLACK LIST");
    return;
  } // check if the uri is on the balck list
  // Now we are ready to handle_the_uri
  handle_uri(uri, tab_index);
  return;
}

// Called when + tab is hit
// Check tab limit ... if ok then do some heavy lifting (see comments)
// Create new tab process with pipes
// Long function
void new_tab_created_cb(GtkButton *button, gpointer data)
{
  if (data == NULL)
  {
    return;
  } // empty check

  // Check if at tab limit, alert().
  if (tabsNum >= MAX_TABS)
  {
    alert("Tabs Limit");
  }

  // Get # free tab, update tabsNum.
  int tab = get_free_tab();
  tabsNum++;

  // Create communication pipes for this tab
  if (pipe(comm[tab].inbound) == -1)
  {
    perror("pipe error\n");
    exit(1);
  }
  if (pipe(comm[tab].outbound) == -1)
  {
    perror("pipe error\n");
    exit(1);
  } // error check

  // Make the read ends non-blocking
  non_block_pipe(comm[tab].inbound[0]);
  non_block_pipe(comm[tab].outbound[0]);

  // fork and create new render tab
  TABS[tab].free = 0;

  pid_t pid = fork();
  if (pid > 0)
  {
    TABS[tab].pid = pid;
  }
  else if (pid == 0)
  {
    char pipe_str[20];
    char index[4];
    sprintf(pipe_str, "%d %d %d %d", comm[tab].inbound[0], comm[tab].inbound[1], comm[tab].outbound[0], comm[tab].outbound[1]);
    sprintf(index, "%d", tab);
    if (execl("./render", "render", index, pipe_str, NULL) < 0)
    {
      perror("execl failed to execute render");
      return;
    }
  } // the child process will read and render the tab
  else
  {
    printf("Fork Created Failed\n");
    exit(-1);
  }
  return;
  // Controller parent just does some TABS bookkeeping
}

// This is called when a favorite is selected for rendering in a tab
// Short
void menu_item_selected_cb(GtkWidget *menu_item, gpointer data)
{

  if (data == NULL)
  {
    return;
  } // empty check

  // Note: For simplicity, currently we assume that the label of the menu_item is a valid url
  // get basic uri
  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));

  // append "https://" for rendering
  char uri[MAX_URL];
  int es = sprintf(uri, "https://%s", basic_uri);
  if (es < 0)
  {
    perror("sprintf error\n");
    exit(1);
  } // error check

  // Get the tab (hint: wrapper.h)
  int tab = query_tab_id_for_request(menu_item, data);
  handle_uri(uri, tab);

  // Hint: now you are ready to handle_the_uri

  return;
}

// BIG CHANGE: the controller now runs an loop so it can check all pipes
// Long function
int run_control()
{
  browser_window *b_window = NULL;
  int i;
  int nRead;
  req_t req;

  // Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
                 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);
  while (1)
  {
    process_single_gtk_event();

    // Read from all tab pipes including private pipe (index 0)
    // to handle commands:
    // PLEASE_DIE (controller should die, self-sent): send PLEASE_DIE to all tabs
    // From any tab:
    //    IS_FAV: add uri to favorite menu (Hint: see wrapper.h), and update .favorites
    //    TAB_IS_DEAD: tab has exited, what should you do?

    // Loop across all pipes from VALID tabs -- starting from 0
    for (i = 0; i < MAX_TABS; i++)
    {
      if (TABS[i].free)
        continue;
      nRead = read(comm[i].outbound[0], &req, sizeof(req_t));

      // Check that nRead returned something before handling cases
      if (nRead == -1)
        continue;

      // Case 1: PLEASE_DIE
      if (req.type == PLEASE_DIE)
      {
        for (int j = 1; j < MAX_TABS; j++)
        {
          if (TABS[j].free)
            continue;
          req_t r;
          r.type = PLEASE_DIE;
          r.tab_index = j;

          if (write(comm[j].inbound[1], &r, sizeof(req_t)) == -1)
          {
            perror("Fail to write in pipe");
          } // error check
        }
        for (int j = 1; j < MAX_TABS; j++)
        {
          if (TABS[j].free)
            continue;
          wait(NULL);
        } // wait
        exit(1);
      } // case 1 if

      // Case 2: TAB_IS_DEAD
      if (req.type == TAB_IS_DEAD)
      {
        TABS[req.tab_index].free = 1;
        tabsNum--;
      } // free the tab

      // Case 3: IS_FAV
      if (req.type == IS_FAV)
      {
        if (fav_ok(req.uri) == 0)
        { // fav_ok check
          update_favorites_file(req.uri);
          add_uri_to_favorite_menu(b_window, req.uri);
        } // add uri to files and menu
      }
    }
    usleep(1000);
  }
  return 0;
} // run_control()

int main(int argc, char **argv)
{
  if (argc != 1)
  {
    fprintf(stderr, "browser <no_args>\n");
    exit(0);
  }

  init_tabs();
  // init blacklist (see util.h), and favorites (write this, see above)
  init_blacklist(".blacklist");
  init_favorites(".favorites");

  // Fork controller
  // Child creates a pipe for itself comm[0]
  // then calls run_control ()
  // Parent waits ...
  int pid = fork();
  if (pid < 0)
  {
    perror("fork failed");
    return 1;
  }
  if (pid == 0)
  { // then we are in child process
    if (pipe(comm[0].inbound) == -1)
    {
      perror("pipe error\n");
      exit(1);
    } // error check
    if (pipe(comm[0].outbound) == -1)
    {
      perror("pipe error\n");
      exit(1);
    } // error check
    non_block_pipe(comm[0].inbound[0]);
    non_block_pipe(comm[0].outbound[0]);
    run_control();
  }
  else
  {
    wait(NULL);
  }
} // main()
