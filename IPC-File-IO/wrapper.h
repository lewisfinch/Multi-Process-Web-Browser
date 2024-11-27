#ifndef __MAIN_H_
#define __MAIN_H_

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 400
#define MAX_LABELS 100 // for controller window (for both header bar and favorites -> = MAX_FAV)

/********************************/
/** Of concern to you .... ******/
/********************************/

// Tab/controller read from outbound, write to inbound
typedef struct comm_channel
{
  int		inbound[2];  
  int		outbound[2];
} comm_channel;

// type of commands/messages
typedef enum req_type
{
 NEW_URI_ENTERED,
 IS_FAV,
 TAB_IS_DEAD,
 PLEASE_DIE
} req_type;


// This represents a command/message sent in a pipe
typedef struct req_t
{
  req_type type;
  int tab_index;
  char uri[512];
} req_t;

typedef enum tab_type
{
        CONTROLLER_TAB,
        URL_RENDERING_TAB
} tab_type;


typedef struct browser_window
{
  WebKitWebView *web_view;
  GtkWidget	*uri_entry;
  GtkWidget 	*tab_selector;
  GtkWidget 	*notebook;
  GtkWidget     *menu_bar;
  int 		tab_index;
  char		tab_label[32];
  comm_channel  channel;
  tab_type	type; 
} browser_window;


void alert(gchar*);
void create_browser_menu(browser_window **b_window, char (*label_list)[][MAX_LABELS], int num);
int create_browser(tab_type t_type, 
	int tab_index,
	void (*new_tab_clicked_cb)(void), 
	void (*uri_entered_cb)(void), 
	browser_window **b_window,
	comm_channel channel);

int query_tab_id_for_request(GtkWidget* entry, gpointer data);
char* get_entered_uri(GtkWidget* entry);
void add_uri_to_favorite_menu (browser_window *b_window, char *uri);

/********************************/
/** END Of concern to you .... **/
/********************************/


/* These are no concern of yours ... */
void activate_uri_entry_cb(GtkWidget* entry, gpointer data);
GtkWidget *get_menu_item_by_label(GtkWidget *menu, char *label);
void new_tab_clicked_cb(GtkButton *button, gpointer data);
void delete_tab_clicked_cb(GtkButton *button, gpointer data);
void create_add_remove_tab_button(char* label, void (*g_callback)(void), void* cb_data);
void show_browser();
void add_items_with_labels_to_menu(GtkWidget *menu, char (*label_list)[][MAX_LABELS], int n_items, void (*menu_item_selected_cb)(void), gpointer data);
void create_labeled_tab(void* cb_data);
int render_web_page_in_tab(char* uri, browser_window* b_window);
GtkWidget *get_menu_item_by_label(GtkWidget *menu, char *label);
void process_single_gtk_event();
void process_all_gtk_events(); 
#endif
