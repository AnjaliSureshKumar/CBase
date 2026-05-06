#include <arpa/inet.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8888
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 2

static int sockfd = -1;
static int client_id = 0;
static GtkWidget *chat_view;
static GtkWidget *input_entry;
static GtkWidget *connect_button;
static GtkWidget *reconnect_button;
static GtkWidget *disconnect_button;
static GtkWidget *send_button;
static GtkWidget *host_entry;
static GtkWidget *port_entry;
static GtkWidget *id_label;
static GtkWidget *client_list_label;
static bool connected = false;
static bool peer_present[MAX_CLIENTS];
static pthread_t receiver_thread;

static gboolean append_text_to_chat(gpointer data) {
    char *text = data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);
    g_free(text);
    return FALSE;
}

static gboolean update_client_id_label(gpointer data) {
    int id = GPOINTER_TO_INT(data);
    if (id <= 0) {
        gtk_label_set_text(GTK_LABEL(id_label), "-");
        return FALSE;
    }

    char label_text[64];
    snprintf(label_text, sizeof(label_text), "Client ID: %d", id);
    gtk_label_set_text(GTK_LABEL(id_label), label_text);
    return FALSE;
}

static gboolean update_client_list_label(gpointer data) {
    char *text = data;
    gtk_label_set_text(GTK_LABEL(client_list_label), text);
    g_free(text);
    return FALSE;
}

static void refresh_client_list(void) {
    char text[128] = "Connected clients: ";
    bool first = true;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (peer_present[i]) {
            if (!first) {
                strncat(text, ", ", sizeof(text) - strlen(text) - 1);
            }
            char item[32];
            if (client_id == i + 1) {
                snprintf(item, sizeof(item), "%d (you)", i + 1);
            } else {
                snprintf(item, sizeof(item), "%d", i + 1);
            }
            strncat(text, item, sizeof(text) - strlen(text) - 1);
            first = false;
        }
    }

    if (first) {
        strncat(text, "None", sizeof(text) - strlen(text) - 1);
    }

    g_idle_add(update_client_list_label, g_strdup(text));
}

static void reset_client_list(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        peer_present[i] = false;
    }
    client_id = 0;
    g_idle_add(update_client_id_label, GINT_TO_POINTER(0));
    refresh_client_list();
}

static void set_connection_buttons(void) {
    gtk_widget_set_sensitive(send_button, connected);
    gtk_widget_set_sensitive(connect_button, !connected);
    gtk_widget_set_sensitive(reconnect_button, !connected);
    gtk_widget_set_sensitive(disconnect_button, connected);
}

static void add_chat_message(const char *message) {
    char *copy = g_strdup(message);
    g_idle_add(append_text_to_chat, copy);
}

static void parse_presence_message(const char *message) {
    if (!g_str_has_prefix(message, "Client ")) {
        return;
    }

    int id = 0;
    if (sscanf(message, "Client %d", &id) != 1 || id < 1 || id > MAX_CLIENTS) {
        return;
    }

    if (strstr(message, "joined the chat.")) {
        peer_present[id - 1] = true;
        refresh_client_list();
    } else if (strstr(message, "disconnected.")) {
        peer_present[id - 1] = false;
        refresh_client_list();
    }
}

static void *receive_messages(void *arg) {
    (void)arg;
    char buffer[BUFFER_SIZE];

    while (connected) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            if (bytes < 0) {
                perror("recv");
            }
            connected = false;
            add_chat_message("Disconnected from server.\n");
            reset_client_list();
            gtk_widget_set_sensitive(send_button, FALSE);
            gtk_widget_set_sensitive(connect_button, TRUE);
            gtk_widget_set_sensitive(reconnect_button, TRUE);
            break;
        }
        buffer[bytes] = '\0';
        if (g_str_has_prefix(buffer, "WELCOME ")) {
            client_id = atoi(buffer + 8);
            if (client_id >= 1 && client_id <= MAX_CLIENTS) {
                peer_present[client_id - 1] = true;
            }
            g_idle_add(update_client_id_label, GINT_TO_POINTER(client_id));
            refresh_client_list();
        }
        parse_presence_message(buffer);
        add_chat_message(buffer);
    }

    return NULL;
}

static void on_send_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(input_entry));
    if (message == NULL || *message == '\0') {
        return;
    }

    if (!connected) {
        add_chat_message("Not connected.\n");
        return;
    }

    char message_line[BUFFER_SIZE];
    snprintf(message_line, sizeof(message_line), "%s\n", message);
    ssize_t sent = send(sockfd, message_line, strlen(message_line), 0);
    if (sent < 0) {
        perror("send");
        add_chat_message("Failed to send message.\n");
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(input_entry), "");
}

static bool connect_to_server(void) {
    const gchar *host = gtk_entry_get_text(GTK_ENTRY(host_entry));
    const gchar *port_text = gtk_entry_get_text(GTK_ENTRY(port_entry));
    int port = atoi(port_text);
    if (port <= 0) {
        add_chat_message("Invalid port number.\n");
        return false;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        add_chat_message("Unable to create socket.\n");
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        add_chat_message("Invalid host address.\n");
        close(sockfd);
        sockfd = -1;
        return false;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        add_chat_message("Unable to connect to server.\n");
        close(sockfd);
        sockfd = -1;
        return false;
    }

    connected = true;
    set_connection_buttons();
    add_chat_message("Connected to server.\n");

    if (pthread_create(&receiver_thread, NULL, receive_messages, NULL) != 0) {
        perror("pthread_create");
        add_chat_message("Failed to start receiver thread.\n");
        connected = false;
        close(sockfd);
        sockfd = -1;
        set_connection_buttons();
        return false;
    }

    return true;
}

static void on_connect_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (connected) {
        return;
    }
    reset_client_list();
    connect_to_server();
}

static void on_reconnect_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (connected) {
        return;
    }

    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    reset_client_list();
    connect_to_server();
}

static void on_disconnect_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    if (!connected) {
        return;
    }

    connected = false;
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    add_chat_message("Disconnected from server.\n");
    reset_client_list();
    set_connection_buttons();
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    connected = false;
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "TCP Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 420, 360);
    gtk_container_set_border_width(GTK_CONTAINER(window), 12);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *host_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), host_box, FALSE, FALSE, 0);

    GtkWidget *host_label = gtk_label_new("Server IP:");
    gtk_box_pack_start(GTK_BOX(host_box), host_label, FALSE, FALSE, 0);

    host_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(host_entry), DEFAULT_HOST);
    gtk_box_pack_start(GTK_BOX(host_box), host_entry, TRUE, TRUE, 0);

    GtkWidget *port_label = gtk_label_new("Port:");
    gtk_box_pack_start(GTK_BOX(host_box), port_label, FALSE, FALSE, 0);

    port_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(port_entry), "8888");
    gtk_entry_set_width_chars(GTK_ENTRY(port_entry), 6);
    gtk_box_pack_start(GTK_BOX(host_box), port_entry, FALSE, FALSE, 0);

    connect_button = gtk_button_new_with_label("Connect");
    gtk_box_pack_start(GTK_BOX(host_box), connect_button, FALSE, FALSE, 0);

    reconnect_button = gtk_button_new_with_label("Reconnect");
    gtk_widget_set_sensitive(reconnect_button, FALSE);
    gtk_box_pack_start(GTK_BOX(host_box), reconnect_button, FALSE, FALSE, 0);

    disconnect_button = gtk_button_new_with_label("Disconnect");
    gtk_widget_set_sensitive(disconnect_button, FALSE);
    gtk_box_pack_start(GTK_BOX(host_box), disconnect_button, FALSE, FALSE, 0);

    GtkWidget *id_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *id_label_title = gtk_label_new("Client ID:");
    id_label = gtk_label_new("-");
    gtk_box_pack_start(GTK_BOX(id_box), id_label_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(id_box), id_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), id_box, FALSE, FALSE, 0);

    GtkWidget *client_list_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *client_list_title = gtk_label_new("Client List:");
    client_list_label = gtk_label_new("Connected clients: None");
    gtk_box_pack_start(GTK_BOX(client_list_box), client_list_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(client_list_box), client_list_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), client_list_box, FALSE, FALSE, 0);

    chat_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(chat_view), FALSE);
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled), chat_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget *message_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), message_box, FALSE, FALSE, 0);

    input_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(message_box), input_entry, TRUE, TRUE, 0);

    send_button = gtk_button_new_with_label("Send");
    gtk_widget_set_sensitive(send_button, FALSE);
    gtk_box_pack_start(GTK_BOX(message_box), send_button, FALSE, FALSE, 0);

    g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_signal_connect(reconnect_button, "clicked", G_CALLBACK(on_reconnect_clicked), NULL);
    g_signal_connect(disconnect_button, "clicked", G_CALLBACK(on_disconnect_clicked), NULL);
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    g_signal_connect(input_entry, "activate", G_CALLBACK(on_send_clicked), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
