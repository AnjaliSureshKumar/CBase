/*
 * exp9/server.c
 * Concurrent TCP file server.
 * This server accepts a filename from the client and returns the file contents
 * if the file exists, otherwise it sends a PID-tagged error message.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 7780
#define BUFFER_SIZE 1024
#define BACKLOG 10

/*
 * Reap child processes after they exit so the server does not accumulate zombies.
 */
static void handle_sigchld(int signo) {
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

/*
 * Remove CR and LF characters from the end of a received line.
 */
static void trim_newline(char *text) {
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[--len] = '\0';
    }
}

/*
 * Read a single line from the connected socket into buffer.
 * This is used to receive the requested filename terminated by '\n'.
 */
static ssize_t recv_line(int fd, char *buffer, size_t size) {
    ssize_t total = 0;
    while (total < (ssize_t)size - 1) {
        char ch;
        ssize_t count = recv(fd, &ch, 1, 0);
        if (count <= 0) {
            return count;
        }
        buffer[total++] = ch;
        if (ch == '\n') {
            break;
        }
    }
    buffer[total] = '\0';
    return total;
}

int main(void) {
    int listen_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char filename[BUFFER_SIZE];

    /* Setup SIGCHLD handler for child cleanup. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* Create a TCP server socket. */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* Reuse the address to avoid bind errors after restart. */
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    printf("File server listening on port %d\n", PORT);

    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            /* Child handles one client connection and then exits. */
            close(listen_fd);
            printf("Client connected from %s:%d (handler PID %d)\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), getpid());

            if (recv_line(client_fd, filename, sizeof(filename)) <= 0) {
                dprintf(client_fd, "PID: %d\nERROR: failed to receive filename\n", getpid());
                close(client_fd);
                return EXIT_FAILURE;
            }

            trim_newline(filename);
            if (filename[0] == '\0') {
                dprintf(client_fd, "PID: %d\nERROR: empty filename provided\n", getpid());
                close(client_fd);
                return EXIT_FAILURE;
            }

            FILE *file = fopen(filename, "rb");
            if (!file) {
                dprintf(client_fd, "PID: %d\nERROR: cannot open file '%s' (%s)\n",
                        getpid(), filename, strerror(errno));
                close(client_fd);
                return EXIT_FAILURE;
            }

            /* Send PID first, then transfer file contents in blocks. */
            dprintf(client_fd, "PID: %d\n", getpid());
            char file_buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
                if (send(client_fd, file_buffer, bytes_read, 0) != (ssize_t)bytes_read) {
                    perror("send");
                    break;
                }
            }
            fclose(file);
            close(client_fd);
            return EXIT_SUCCESS;
        }

        close(client_fd);
    }

    close(listen_fd);
    return EXIT_SUCCESS;
}
