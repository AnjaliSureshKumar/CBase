#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/*
 * Simple GTK GUI application in C to check if a number is odd or even.
 * Build with: gcc -o odd_even_gui odd_even_gui.c $(pkg-config --cflags --libs gtk+-3.0)
 */

static void on_check_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    const gchar *text = gtk_entry_get_text(entry);
    char result_text[128];
    char *endptr;
    long value;

    if (text == NULL || *text == '\0') {
        gtk_label_set_text(GTK_LABEL(gtk_widget_get_parent(GTK_WIDGET(button))), "Please enter a number.");
        return;
    }

    value = strtol(text, &endptr, 10);
    if (*endptr != '\0' && *endptr != '\n') {
        snprintf(result_text, sizeof(result_text), "Invalid input: '%s' is not an integer.", text);
    } else {
        if ((value % 2) == 0) {
            snprintf(result_text, sizeof(result_text), "%ld is even.", value);
        } else {
            snprintf(result_text, sizeof(result_text), "%ld is odd.", value);
        }
    }

    GtkWidget *result_label = g_object_get_data(G_OBJECT(button), "result_label");
    gtk_label_set_text(GTK_LABEL(result_label), result_text);
}

static void on_activate_entry(GtkEntry *entry, gpointer user_data) {
    GtkButton *button = GTK_BUTTON(user_data);
    on_check_button_clicked(button, entry);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Odd or Even Checker");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 180);
    gtk_container_set_border_width(GTK_CONTAINER(window), 12);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *prompt_label = gtk_label_new("Enter an integer:");
    gtk_box_pack_start(GTK_BOX(vbox), prompt_label, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g. 42");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    GtkWidget *button = gtk_button_new_with_label("Check");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    GtkWidget *result_label = gtk_label_new("Enter a number and click Check.");
    gtk_box_pack_start(GTK_BOX(vbox), result_label, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(button), "result_label", result_label);
    g_signal_connect(button, "clicked", G_CALLBACK(on_check_button_clicked), entry);
    g_signal_connect(entry, "activate", G_CALLBACK(on_activate_entry), button);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
