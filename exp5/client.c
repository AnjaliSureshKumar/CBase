#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define PORT 7777
#define BUFFER_SIZE 1024

static ssize_t recv_all(int sock, void *buf, size_t len) {
    size_t total = 0;
    char *ptr = buf;
    while (total < len) {
        ssize_t received = recv(sock, ptr + total, len - total, 0);
        if (received <= 0) {
            return received;
        }
        total += received;
    }
    return total;
}

static ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *ptr = buf;
    while (total < len) {
        ssize_t sent = send(sock, ptr + total, len - total, 0);
        if (sent <= 0) {
            return sent;
        }
        total += sent;
    }
    return total;
}

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    int n;

    printf("Enter matrix order N: ");
    if (scanf("%d", &n) != 1 || n <= 0 || n > 100) {
        fprintf(stderr, "Invalid matrix order. Please enter a positive integer up to 100.\n");
        return EXIT_FAILURE;
    }

    int *matrix = malloc((size_t)n * n * sizeof(int));
    if (!matrix) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL));
    printf("Generated %dx%d matrix:\n", n, n);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i*n + j] = rand() % 50 + 1;
            printf("%3d ", matrix[i*n + j]);
        }
        printf("\n");
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        free(matrix);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }

    int32_t n_net = htonl(n);
    if (send_all(sockfd, &n_net, sizeof(n_net)) != sizeof(n_net)) {
        perror("send size");
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n * n; i++) {
        int32_t value_net = htonl(matrix[i]);
        if (send_all(sockfd, &value_net, sizeof(value_net)) != sizeof(value_net)) {
            perror("send matrix");
            close(sockfd);
            free(matrix);
            return EXIT_FAILURE;
        }
    }

    int32_t len_net;
    if (recv_all(sockfd, &len_net, sizeof(len_net)) != sizeof(len_net)) {
        perror("recv length");
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }

    int32_t len = ntohl(len_net);
    if (len <= 0 || len > BUFFER_SIZE - 1) {
        fprintf(stderr, "Invalid response length: %d\n", len);
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }

    char result[BUFFER_SIZE];
    if (recv_all(sockfd, result, len) != len) {
        perror("recv result");
        close(sockfd);
        free(matrix);
        return EXIT_FAILURE;
    }
    result[len] = '\0';

    printf("Server identified matrix type: %s\n", result);

    close(sockfd);
    free(matrix);
    return EXIT_SUCCESS;
}
