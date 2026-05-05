#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define PORT 7778
#define BUFFER_SIZE 2048

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("Enter a new-generation English sentence:\n");
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        fprintf(stderr, "Failed to read input.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    if (len == 0) {
        fprintf(stderr, "Empty sentence provided.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (sendto(sockfd, buffer, len, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return EXIT_FAILURE;
    }

    socklen_t addr_len = sizeof(server_addr);
    ssize_t recv_len = recvfrom(sockfd, response, sizeof(response) - 1, 0,
                                (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len < 0) {
        perror("recvfrom");
        close(sockfd);
        return EXIT_FAILURE;
    }
    response[recv_len] = '\0';

    printf("Translated sentence:\n%s\n", response);
    close(sockfd);
    return EXIT_SUCCESS;
}
