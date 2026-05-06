/*
 * exp8/server.c
 * UDP concurrent time server.
 * This server listens for a time request, forks a handler process,
 * formats the local system time, and sends it back to the client.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 7779
#define BUFFER_SIZE 128

/*
 * Reap any terminated child processes to avoid zombie processes.
 */
static void reap_children(int signo) {
    (void)signo;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char time_str[BUFFER_SIZE];

    /* Install SIGCHLD handler so forked children are reaped cleanly. */
    if (signal(SIGCHLD, reap_children) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    /* Create a UDP socket for the time server. */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    /* Allow the server to reuse the port immediately after restart. */
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* Bind the socket to the port so it can receive client requests. */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("UDP Time Server listening on port %d\n", PORT);

    while (1) {
        /* Receive a UDP message from a client. */
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recvfrom");
            continue;
        }

        buffer[recv_len] = '\0';
        printf("Received time request from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* Fork a child to handle this request concurrently. */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            /* Child process - generate and send the current time. */
            time_t now = time(NULL);
            struct tm local_time;
            if (localtime_r(&now, &local_time) == NULL) {
                perror("localtime_r");
                close(sockfd);
                return EXIT_FAILURE;
            }

            if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &local_time) == 0) {
                strncpy(time_str, "Unable to format time", sizeof(time_str) - 1);
                time_str[sizeof(time_str) - 1] = '\0';
            }

            if (sendto(sockfd, time_str, strlen(time_str), 0,
                       (struct sockaddr *)&client_addr, client_len) < 0) {
                perror("sendto");
                close(sockfd);
                return EXIT_FAILURE;
            }

            close(sockfd);
            return EXIT_SUCCESS;
        }

        /* Parent continues to listen for new requests. */
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
