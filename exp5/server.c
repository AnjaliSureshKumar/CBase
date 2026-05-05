#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 7777
#define BACKLOG 5

static int is_upper_triangular(int *matrix, int n) {
    for (int i = 1; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (matrix[i*n + j] != 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int is_lower_triangular(int *matrix, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = i+1; j < n; j++) {
            if (matrix[i*n + j] != 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int is_diagonal(int *matrix, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j && matrix[i*n + j] != 0) {
                return 0;
            }
        }
    }
    return 1;
}

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
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("TCP Matrix Server listening on port %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        int32_t n_net;
        if (recv_all(client_fd, &n_net, sizeof(n_net)) != sizeof(n_net)) {
            perror("recv size");
            close(client_fd);
            continue;
        }

        int32_t n = ntohl(n_net);
        if (n <= 0 || n > 1000) {
            fprintf(stderr, "Invalid matrix size: %d\n", n);
            close(client_fd);
            continue;
        }

        size_t count = (size_t)n * n;
        int *matrix = malloc(count * sizeof(int));
        if (!matrix) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        for (size_t i = 0; i < count; i++) {
            int32_t value_net;
            if (recv_all(client_fd, &value_net, sizeof(value_net)) != sizeof(value_net)) {
                perror("recv matrix");
                free(matrix);
                close(client_fd);
                goto next_client;
            }
            matrix[i] = ntohl(value_net);
        }

        const char *result;
        if (is_diagonal(matrix, n)) {
            result = "Diagonal";
        } else if (is_upper_triangular(matrix, n)) {
            result = "Upper triangular";
        } else if (is_lower_triangular(matrix, n)) {
            result = "Lower triangular";
        } else {
            result = "Neither";
        }

        printf("Received %dx%d matrix, result: %s\n", n, n, result);
        int32_t len = htonl((int32_t)strlen(result));
        if (send_all(client_fd, &len, sizeof(len)) != sizeof(len) ||
            send_all(client_fd, result, strlen(result)) != (ssize_t)strlen(result)) {
            perror("send result");
        }

        free(matrix);

    next_client:
        close(client_fd);
        client_fd = -1;
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
