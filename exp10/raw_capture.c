/*
 * exp10/raw_capture.c
 * Simple packet capture utility using raw sockets.
 * It parses IPv4 packets and prints headers for ICMP, TCP, and UDP.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 65536
#define PREVIEW_BYTES 64

static int raw_sock = -1;
static unsigned long total_packets = 0;
static unsigned long total_bytes = 0;
static unsigned long icmp_packets = 0;
static unsigned long tcp_packets = 0;
static unsigned long udp_packets = 0;
static unsigned long other_packets = 0;

/* Map common protocol numbers to human-readable names. */
static const char *protocol_name(int proto) {
    switch (proto) {
        case IPPROTO_ICMP: return "ICMP";
        case IPPROTO_TCP: return "TCP";
        case IPPROTO_UDP: return "UDP";
        case IPPROTO_IP: return "IP";
        default: return "OTHER";
    }
}

/* Convert a protocol name or number into the raw socket protocol value. */
static int parse_protocol(const char *arg) {
    if (strcasecmp(arg, "icmp") == 0) return IPPROTO_ICMP;
    if (strcasecmp(arg, "tcp") == 0) return IPPROTO_TCP;
    if (strcasecmp(arg, "udp") == 0) return IPPROTO_UDP;
    if (strcasecmp(arg, "ip") == 0 || strcasecmp(arg, "any") == 0) return IPPROTO_IP;
    return atoi(arg);
}

/* Print a hexadecimal preview of packet payload bytes. */
static void print_bytes(const unsigned char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16) printf("\n");
}

/* Decode and print basic TCP header fields. */
static void print_tcp_header(const unsigned char *packet, ssize_t packet_len, int ip_header_len) {
    ssize_t min_len = (ssize_t)ip_header_len + (ssize_t)sizeof(struct tcphdr);
    if (packet_len < min_len) {
        printf("TCP header truncated\n");
        return;
    }

    struct tcphdr *tcp_hdr = (struct tcphdr *)(packet + ip_header_len);
    printf("TCP Source Port: %u\n", ntohs(tcp_hdr->th_sport));
    printf("TCP Destination Port: %u\n", ntohs(tcp_hdr->th_dport));
    printf("Sequence Number: %u\n", ntohl(tcp_hdr->th_seq));
    printf("Acknowledgment: %u\n", ntohl(tcp_hdr->th_ack));
    printf("Data Offset: %u bytes\n", tcp_hdr->th_off * 4);
    printf("Flags: ");
    if (tcp_hdr->th_flags & TH_FIN) printf("FIN ");
    if (tcp_hdr->th_flags & TH_SYN) printf("SYN ");
    if (tcp_hdr->th_flags & TH_RST) printf("RST ");
    if (tcp_hdr->th_flags & TH_PUSH) printf("PSH ");
    if (tcp_hdr->th_flags & TH_ACK) printf("ACK ");
    if (tcp_hdr->th_flags & TH_URG) printf("URG ");
    printf("\n");
    printf("Window: %u\n", ntohs(tcp_hdr->th_win));
    printf("Checksum: 0x%04x\n", ntohs(tcp_hdr->th_sum));
}

/* Decode and print basic UDP header fields. */
static void print_udp_header(const unsigned char *packet, ssize_t packet_len, int ip_header_len) {
    ssize_t min_len = (ssize_t)ip_header_len + (ssize_t)sizeof(struct udphdr);
    if (packet_len < min_len) {
        printf("UDP header truncated\n");
        return;
    }

    struct udphdr *udp_hdr = (struct udphdr *)(packet + ip_header_len);
    printf("UDP Source Port: %u\n", ntohs(udp_hdr->uh_sport));
    printf("UDP Destination Port: %u\n", ntohs(udp_hdr->uh_dport));
    printf("UDP Length: %u\n", ntohs(udp_hdr->uh_ulen));
    printf("UDP Checksum: 0x%04x\n", ntohs(udp_hdr->uh_sum));
}

/* Print capture statistics when the program exits. */
static void print_summary(void) {
    printf("\n===== Capture Statistics =====\n");
    printf("Total packets: %lu\n", total_packets);
    printf("Total bytes: %lu\n", total_bytes);
    printf("ICMP packets: %lu\n", icmp_packets);
    printf("TCP packets: %lu\n", tcp_packets);
    printf("UDP packets: %lu\n", udp_packets);
    printf("Other packets: %lu\n", other_packets);
}

/* Handle Ctrl+C and other termination signals cleanly. */
static void handle_signal(int signo) {
    (void)signo;
    if (raw_sock >= 0) close(raw_sock);
    print_summary();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int protocol = IPPROTO_ICMP;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t packet_len;

    /* Allow selecting a protocol by name or number. */
    if (argc > 1) {
        protocol = parse_protocol(argv[1]);
        if (protocol < 0) {
            fprintf(stderr, "Invalid protocol '%s'. Use icmp, tcp, udp, ip/any, or numeric value.\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    /* Create a raw socket bound to the selected protocol. */
    raw_sock = socket(AF_INET, SOCK_RAW, protocol);
    if (raw_sock < 0) {
        perror("socket");
        fprintf(stderr, "Raw sockets require elevated privileges. Run with sudo/root.\n");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("Packet capture started (protocol=%s). Press Ctrl+C to stop.\n", protocol_name(protocol));

    while (1) {
        packet_len = recv(raw_sock, buffer, sizeof(buffer), 0);
        if (packet_len < 0) {
            perror("recv");
            close(raw_sock);
            return EXIT_FAILURE;
        }

        total_packets++;
        total_bytes += packet_len;

        if (packet_len < (ssize_t)sizeof(struct ip)) {
            fprintf(stderr, "Received packet too short (%zd bytes).\n", packet_len);
            continue;
        }

        struct ip *ip_hdr = (struct ip *)buffer;
        int ip_header_len = ip_hdr->ip_hl * 4;
        char src_addr[INET_ADDRSTRLEN];
        char dst_addr[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &ip_hdr->ip_src, src_addr, sizeof(src_addr));
        inet_ntop(AF_INET, &ip_hdr->ip_dst, dst_addr, sizeof(dst_addr));

        printf("\n===== Packet %lu =====\n", total_packets);
        printf("IP Version: %d\n", ip_hdr->ip_v);
        printf("Header Length: %d bytes\n", ip_header_len);
        printf("Total Length: %u\n", ntohs(ip_hdr->ip_len));
        printf("TTL: %d\n", ip_hdr->ip_ttl);
        printf("Protocol: %s (%d)\n", protocol_name(ip_hdr->ip_p), ip_hdr->ip_p);
        printf("Source: %s\n", src_addr);
        printf("Destination: %s\n", dst_addr);

        if (ip_hdr->ip_p == IPPROTO_ICMP) {
            icmp_packets++;
            if (packet_len >= ip_header_len + 4) {
                unsigned char *icmp_hdr = buffer + ip_header_len;
                printf("ICMP Type: %u\n", icmp_hdr[0]);
                printf("ICMP Code: %u\n", icmp_hdr[1]);
                printf("ICMP Checksum: 0x%02x%02x\n", icmp_hdr[2], icmp_hdr[3]);
            } else {
                printf("ICMP header truncated\n");
            }
        } else if (ip_hdr->ip_p == IPPROTO_TCP) {
            tcp_packets++;
            print_tcp_header(buffer, packet_len, ip_header_len);
        } else if (ip_hdr->ip_p == IPPROTO_UDP) {
            udp_packets++;
            print_udp_header(buffer, packet_len, ip_header_len);
        } else {
            other_packets++;
        }

        size_t payload_offset = ip_header_len;
        size_t payload_len = (packet_len > (ssize_t)payload_offset) ? packet_len - payload_offset : 0;
        printf("Payload (%zu bytes):\n", payload_len);
        print_bytes(buffer + payload_offset, payload_len > PREVIEW_BYTES ? PREVIEW_BYTES : payload_len);
    }

    close(raw_sock);
    return EXIT_SUCCESS;
}
