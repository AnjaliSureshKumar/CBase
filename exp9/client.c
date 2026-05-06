/*
 * exp9/client.c
 * TCP client for requesting a file from the server.
 * It connects to the server, sends a filename line, and prints the response.
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
#define PORT 7780
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *server_ip = SERVER_IP;
    const char *filename;

    /* Allow optional server IP override and required filename argument. */
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        filename = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [server_ip] <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Create a TCP socket to connect to the file server. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", server_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Connect to the server before sending the requested filename. */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Send the requested filename terminated by newline. */
    if (send(sockfd, filename, strlen(filename), 0) < 0 || send(sockfd, "\n", 1, 0) < 0) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Read the server's response until the connection closes. */
    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        fputs(buffer, stdout);
    }
    if (bytes_received < 0) {
        perror("recv");
        close(sockfd);
        return EXIT_FAILURE;
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
