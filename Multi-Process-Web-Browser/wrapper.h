#ifndef __MAIN_H_
#define __MAIN_H_

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#define INITIAL_TAB_OFFSET 1
#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 400
#define UNRECLAIMED_TAB_COUNTER 10

void alert(gchar*);
void create_labeled_tab(void* cb_data);
void activate_uri_entry_cb(GtkWidget* entry, gpointer data);
void tab_selected_cb(GtkWidget* event_box, gint arg1, gpointer data);
void new_tab_clicked_cb(GtkButton *button, gpointer data);
void delete_tab_clicked_cb(GtkButton *button, gpointer data);
void create_add_remove_tab_button(char* label, void (*g_callback)(void), void* cb_data);
void show_browser();

typedef enum tab_type
{
        CONTROLLER_TAB,
        URL_RENDERING_TAB
}tab_type;

typedef struct browser_window
{
  WebKitWebView *web_view;
  GtkWidget	*uri_entry;
  GtkWidget 	*tab_selector;
  GtkWidget 	*notebook;
  int 		tab_index;
  char		tab_label[32];
  tab_type	type; 
}browser_window;

typedef enum req_type
{
	CREATE_TAB,
	NEW_URI_ENTERED,
	TAB_KILLED
}req_type;

typedef struct tab_killed_req
{
	int tab_index;
}tab_killed_req;

typedef struct create_new_tab_req
{
	int tab_index;
}create_new_tab_req;

typedef struct new_uri_req
{
	char uri[512];
	int render_in_tab;
}new_uri_req;

typedef union
{
	create_new_tab_req new_tab_req;
	new_uri_req	uri_req;
	tab_killed_req  killed_req;
}child_request;

typedef struct child_req_to_parent
{
	req_type type;
	child_request req;
}child_req_to_parent;


int render_web_page_in_tab(char* uri, browser_window* b_window);
int create_browser(tab_type t_type, 
	int tab_index,
	void (*new_tab_clicked_cb)(void), 
	void (*uri_entered_cb)(void), 
		   browser_window **b_window);

int query_tab_id_for_request(GtkWidget* entry, gpointer data);
char* get_entered_uri(GtkWidget* entry);
size_t get_shared_browser_size();
void page_added_cb(GtkWindow *notebook, GtkWidget* widget, gpointer user_data);

int create_proc_for_new_tab(int tab_index, int actual_tab_cnt);
void process_single_gtk_event();
void process_all_gtk_events(); 
#endif
