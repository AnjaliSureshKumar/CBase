#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define PORT 7779
#define BUFFER_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    fd_set readfds;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Connected to chat server %s:%d\n", SERVER_IP, PORT);
    printf("Type messages and press Enter. Ctrl+C to quit.\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        int max_fd = sockfd;
        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (!fgets(send_buf, sizeof(send_buf), stdin)) {
                break;
            }
            size_t len = strlen(send_buf);
            if (len > 0 && send_buf[len - 1] == '\n') send_buf[len - 1] = '\0';
            if (strlen(send_buf) == 0) continue;
            if (send(sockfd, send_buf, strlen(send_buf), 0) < 0) {
                perror("send");
                break;
            }
        }

        if (FD_ISSET(sockfd, &readfds)) {
            ssize_t bytes_read = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    printf("Server closed connection.\n");
                } else {
                    perror("recv");
                }
                break;
            }
            recv_buf[bytes_read] = '\0';
            printf("%s", recv_buf);
            if (recv_buf[bytes_read - 1] != '\n') printf("\n");
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
