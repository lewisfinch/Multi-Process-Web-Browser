#include "wrapper.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Pops up an alert window
void alert(gchar*);

/*
 * Name:	get_entered_uri
 * Input:	'entry'-GtkWidget for address bar in controller-tab
 * Output:      user entered 'url'
 * Function:    returns the url entered in the address bar
 */
char* get_entered_uri(GtkWidget* entry)
{
  return((char*)gtk_entry_get_text (GTK_ENTRY (entry)));
}

void process_all_gtk_events()
{
  while(gtk_events_pending ())
    gtk_main_iteration();
}

void process_single_gtk_event()
{
  gtk_main_iteration_do(FALSE);
}

void create_add_remove_tab_button(char* label, void (*g_callback)(void), void* cb_data)
{

  browser_window *b_window=((browser_window*)cb_data);

  GtkWidget* new_tab_button = gtk_button_new_with_label (label);
  g_signal_connect (G_OBJECT (new_tab_button), "clicked", g_callback, cb_data);
  gtk_widget_show(new_tab_button);

  GtkWidget *window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (window, WINDOW_WIDTH, WINDOW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(window);

  gtk_notebook_append_page (GTK_NOTEBOOK (b_window->notebook), window, new_tab_button);
}

void create_labeled_tab(void* cb_data)
{
  GtkWidget* scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, WINDOW_WIDTH, WINDOW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
	GTK_POLICY_AUTOMATIC, 
	GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolled_window);

  // Create 'button-press-event' callback event data
  browser_window* b_window = (browser_window*)cb_data;

  // Create web-page rendering area
  b_window->web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET(b_window->web_view));

  //  webkit_web_view_open(b_window->web_view, "about:blank");
  webkit_web_view_load_uri(b_window->web_view, "about:blank");

  GtkWidget* label = gtk_label_new(b_window->tab_label);
  gtk_widget_show(label);

  // Attach tab to the browser
  gtk_notebook_append_page (GTK_NOTEBOOK (b_window->notebook), scrolled_window, label);

}

int render_web_page_in_tab(char* uri, browser_window* b_window)
{
  webkit_web_view_load_uri(b_window->web_view, uri);
  return 0;
}

void alert(gchar* msg) 
{ 
  GtkWidget* dialog = gtk_dialog_new_with_buttons("Message", 
		NULL,
		GTK_DIALOG_MODAL,
		"_Ok",
		GTK_RESPONSE_NONE,
		NULL); // create a new dialog
  GtkWidget* content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  GtkWidget* label = gtk_label_new(msg);
  g_signal_connect_swapped (dialog,
                           "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);
  gtk_container_add (GTK_CONTAINER (content_area), label);
  gtk_widget_show_all (dialog);
}

void delete_window_cb(GtkWidget *window, gpointer data) 
{
  gtk_main_quit();
}

int create_browser(tab_type t_type, 
		   int tab_index,
		   void (*create_new_tab_cb)(void), 
		   void (*uri_entered_cb)(void), 
		   browser_window **b_window) {
  GtkWidget *window;
  GtkWidget *grid;

  gtk_init(NULL, NULL);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

  grid = gtk_grid_new();

  gtk_container_add (GTK_CONTAINER (window), grid);

  // Allocate space for browser-window to be passed to various callbacks.
  *b_window = (browser_window *)calloc(1, sizeof(browser_window));
  (*b_window)->web_view = NULL;
  (*b_window)->notebook = gtk_notebook_new ();
  (*b_window)->tab_index = tab_index;
  sprintf((*b_window)->tab_label, "Tab %d", tab_index);

  gtk_notebook_set_tab_pos (GTK_NOTEBOOK ((*b_window)->notebook), GTK_POS_TOP);
  gtk_grid_attach (GTK_GRID (grid), (*b_window)->notebook, 0, 1, 7, 2);
  gtk_widget_show ((*b_window)->notebook);
 
  if(t_type == CONTROLLER_TAB) {
    gtk_window_set_title(GTK_WINDOW(window), "CONTROLLER-TAB");
    
    GtkWidget* url_label = gtk_label_new("URL to Render: ");
    gtk_grid_attach(GTK_GRID(grid), url_label,0, 0, 1, 1);
    gtk_widget_show(url_label);

    GtkWidget* uri_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), uri_entry, 1, 0, 6, 1); 
    gtk_widget_show(uri_entry);
    g_signal_connect (G_OBJECT (uri_entry), "activate", uri_entered_cb, *b_window);
    (*b_window)->uri_entry = uri_entry;

    // Removed this tab selector.. no use in 2022 version
    // GtkWidget* tab_selector = gtk_entry_new();
    // gtk_grid_attach(GTK_GRID(grid), tab_selector, 3, 0, 2, 1);
    // gtk_widget_show(tab_selector);
    // (*b_window)->tab_selector = tab_selector;

  }
  else {
    char szTitle[32];
    sprintf(szTitle, "URL-RENDERING TAB - %d", tab_index);
    gtk_window_set_title(GTK_WINDOW(window), szTitle);
    (*b_window)->tab_selector = NULL;
    (*b_window)->uri_entry = NULL;
  }

  if(t_type == CONTROLLER_TAB) {
    // Button does noting in 2022 lab
    create_add_remove_tab_button("HAVE FUN!", G_CALLBACK(create_new_tab_cb), *b_window);
  }
  else {
     create_labeled_tab(*b_window);
  }
  (*b_window)->type = t_type;

  g_signal_connect(G_OBJECT (window), "destroy", 
		   G_CALLBACK(delete_window_cb), *b_window);
  gtk_widget_show(grid);
  gtk_widget_show_all(window);
  return 0;
}

void show_browser() {
  gtk_main();
}