#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 7778
#define BUFFER_SIZE 2048

struct replacement {
    const char *abbr;
    const char *full;
};

static const struct replacement replacements[] = {
    {"tbh", "to be honest"},
    {"ig", "I guess"},
    {"tbf", "to be fair"},
    {"atm", "at the moment"},
    {"irl", "in real life"},
    {"lol", "laughing out loud"},
    {"asap", "as soon as possible"},
    {"omg", "oh my god"},
    {"ttyl", "talk to you later"},
    {"idk", "I don't know"},
    {"nvm", "never mind"},
};

static int is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '\'';
}

static void translate_sentence(const char *input, char *output, size_t out_size) {
    size_t out_pos = 0;
    size_t len = strlen(input);

    for (size_t i = 0; i < len && out_pos + 1 < out_size; ) {
        if (!is_word_char(input[i])) {
            output[out_pos++] = input[i++];
            continue;
        }

        size_t start = i;
        while (i < len && is_word_char(input[i])) {
            i++;
        }
        size_t word_len = i - start;
        char word[128];
        if (word_len >= sizeof(word)) {
            word_len = sizeof(word) - 1;
        }
        for (size_t j = 0; j < word_len; j++) {
            word[j] = tolower((unsigned char)input[start + j]);
        }
        word[word_len] = '\0';

        const char *replacement = NULL;
        for (size_t r = 0; r < sizeof(replacements) / sizeof(replacements[0]); r++) {
            if (strcmp(word, replacements[r].abbr) == 0) {
                replacement = replacements[r].full;
                break;
            }
        }

        if (replacement) {
            size_t rep_len = strlen(replacement);
            if (out_pos + rep_len >= out_size - 1) {
                break;
            }
            memcpy(output + out_pos, replacement, rep_len);
            out_pos += rep_len;
        } else {
            if (out_pos + word_len >= out_size - 1) {
                break;
            }
            memcpy(output + out_pos, input + start, word_len);
            out_pos += word_len;
        }
    }

    output[out_pos] = '\0';
}

int main(void) {
    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("UDP translator server listening on port %d\n", PORT);
    printf("Supported abbreviations: tbh, ig, tbf, atm, irl, lol, asap, omg, ttyl, idk, nvm\n");

    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                    (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("recvfrom");
            continue;
        }
        buffer[recv_len] = '\0';

        printf("Received from %s:%d: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        translate_sentence(buffer, response, sizeof(response));

        if (sendto(sockfd, response, strlen(response), 0,
                   (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("sendto");
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
