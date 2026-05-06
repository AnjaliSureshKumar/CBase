/*
 * exp8/client.c
 * UDP client that sends a time request and prints the server's response.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define PORT 7779
#define BUFFER_SIZE 128

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *server_ip = SERVER_IP;
    const char *request_message = "TIME";

    /* Allow optional server IP override from command line. */
    if (argc > 1) {
        server_ip = argv[1];
    }

    /* Create a UDP socket for sending the request. */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    /* Convert the text IP address into binary form. */
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", server_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Send the fixed time request string to the server. */
    printf("Sending time request to %s:%d...\n", server_ip, PORT);
    if (sendto(sockfd, request_message, strlen(request_message), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Receive the server response and print it. */
    ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    if (recv_len < 0) {
        perror("recvfrom");
        close(sockfd);
        return EXIT_FAILURE;
    }

    buffer[recv_len] = '\0';
    printf("Server time: %s\n", buffer);

    close(sockfd);
    return EXIT_SUCCESS;
}
