#include <gtk/gtk.h>

void smooth_send_message(GtkWidget * button, gpointer user_data)
{
	GtkTextIter start, end;
	GtkWidget * text_view = GTK_WIDGET(user_data);
	GtkTextBuffer * text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	gtk_text_buffer_get_bounds(text_buffer, &start, &end);
	gchar * message = gtk_text_buffer_get_text(text_buffer, &start, &end, TRUE);
	gtk_text_buffer_set_text(text_buffer, "", -1);

	if (message && message[0])
		g_message("Send : %s", message);
}

void smooth_clear_message_area(GtkWidget * button, gpointer user_data)
{
	GtkWidget * text_view = GTK_WIDGET(user_data);
	GtkTextBuffer * text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(text_buffer, "", -1);
}

void smooth_show_respond_window(gchar * name)
{
	GtkWidget * window;
	GtkWidget * text_view;
	GtkWidget * scrolled_win;
	GtkWidget * paned;
	GtkWidget * vbox;
	GtkWidget * button_box;
	GtkWidget * button_send;
	GtkWidget * button_clear;
	int height;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), name);
	gtk_widget_set_size_request(window, 400, 600);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);

	paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

	text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view)), name, -1);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), text_view);
	gtk_paned_add1(GTK_PANED(paned), scrolled_win);

	text_view = gtk_text_view_new();
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view)), name, -1);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), text_view);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

	button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(button_box), 5);

	button_send = gtk_button_new_with_label("Send");
	gtk_box_pack_start(GTK_BOX(button_box), button_send, FALSE, FALSE, 0);
	g_signal_connect(button_send, "clicked", G_CALLBACK(smooth_send_message), text_view);

	button_clear = gtk_button_new_with_label("Clear");
	gtk_box_pack_start(GTK_BOX(button_box), button_clear, FALSE, FALSE, 0);
	g_signal_connect(button_clear, "clicked", G_CALLBACK(smooth_clear_message_area), text_view);

	gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

	gtk_paned_add2(GTK_PANED(paned), vbox);


	gtk_container_add(GTK_CONTAINER(window), paned);

	gtk_widget_show_all(window);

	height = gtk_widget_get_allocated_height(paned);
	g_message("Height: %d", height);
	gtk_paned_set_position(GTK_PANED(paned), height - 200);
}

enum {
	SMOOTH_COLUMN_NAME,
	SMOOTH_COLUMN_MAX,
};

void smooth_respond_tree_view_click(GtkTreeView * tree_view, GtkTreePath * path,
									GtkTreeViewColumn * column, gpointer user_data)
{
	GtkTreeIter iter;
	gchar * name;
	GtkTreeModel * tree_model = gtk_tree_view_get_model(tree_view);

	gtk_tree_model_get_iter(tree_model, &iter, path);

	gtk_tree_model_get(tree_model, &iter, SMOOTH_COLUMN_NAME, &name, -1);

	g_message("Column %s", name);

	smooth_show_respond_window(name);
}

GtkWidget * smooth_initailize_tree_view()
{
	GtkCellRenderer * renderer_text;
	GtkTreeViewColumn * column;
	GtkWidget * tree_view;
	GtkTreeIter iter;
	GtkListStore * list_store = gtk_list_store_new(SMOOTH_COLUMN_MAX, G_TYPE_STRING);

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, SMOOTH_COLUMN_NAME, "XiaoLi", -1);

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, SMOOTH_COLUMN_NAME, "XiaoWang", -1);

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, SMOOTH_COLUMN_NAME, "XiaoQiang", -1);

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));

	g_signal_connect(tree_view, "row_activated", G_CALLBACK(smooth_respond_tree_view_click), NULL);

	renderer_text = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Friends", renderer_text, "text", SMOOTH_COLUMN_NAME, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	return tree_view;
}

gboolean smooth_show_manager_panel(gpointer user_data)
{
	GtkWidget * window;
	GtkWidget * tree_view;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 300, 800);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	tree_view = smooth_initailize_tree_view();

	gtk_container_add(GTK_CONTAINER(window), tree_view);

	gtk_widget_show_all(window);

	g_message("Login success");

	return FALSE;
}

void smooth_respond_login(GtkWidget * button, gpointer user_data)
{
	GtkWidget * account_entry = GTK_WIDGET(user_data);
	const gchar * account = gtk_entry_get_text(GTK_ENTRY(account_entry));

	g_message("Account: %s", account);

	g_timeout_add(1000, smooth_show_manager_panel, NULL);
}

GtkWidget * smooth_login_panel()
{
	GtkWidget * window;
	GtkWidget * vbox, * vbox_outer;
	GtkWidget * account_entry;
	GtkWidget * passwd_entry;
	GtkWidget * login_button;
	GtkWidget * cancel_button;
	GtkWidget * button_box;
	GtkWidget * account_label;
	GtkWidget * passwd_label;
	GtkWidget * grid;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 300, 300);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	vbox_outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox_outer), vbox, TRUE, FALSE, 0);

	grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

	account_label = gtk_label_new("Account");
	gtk_widget_set_halign(account_label, 0);

	account_entry = gtk_entry_new();
	gtk_widget_set_hexpand(account_entry, TRUE);

	gtk_grid_attach(GTK_GRID(grid), account_label, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), account_entry, 1, 0, 1, 1);

	passwd_label = gtk_label_new("Passwd");
	gtk_widget_set_halign(passwd_label, 0);

	passwd_entry = gtk_entry_new();
	gtk_widget_set_hexpand(passwd_entry, TRUE);
	gtk_entry_set_visibility(GTK_ENTRY(passwd_entry), FALSE);
	
	gtk_grid_attach(GTK_GRID(grid), passwd_label, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), passwd_entry, 1, 1, 1, 1);

	gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

	button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_CENTER);
	gtk_box_set_spacing(GTK_BOX(button_box), 5);

	login_button = gtk_button_new_with_label("Login");
	gtk_box_pack_start(GTK_BOX(button_box), login_button, FALSE, FALSE, 0);
	g_signal_connect(login_button, "clicked", G_CALLBACK(smooth_respond_login), account_entry);

	cancel_button = gtk_button_new_with_label("Cancel");
	gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox_outer);

	gtk_widget_show_all(window);
}

int main(int ac, char ** av)
{
	gtk_init(&ac, &av);

	smooth_login_panel();

	gtk_main();

	return 0;
}
