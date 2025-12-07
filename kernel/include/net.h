#ifndef NET_H
#define NET_H

#include <stdint.h>

// Ethernet frame structure
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_header_t;

// IP header (simplified)
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

// TCP header (simplified)
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// TCP flags
#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG  0x20

// Network configuration
#define MY_IP_ADDR    0x0A000002  // 10.0.0.2
#define MY_MAC_ADDR   {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}
#define TELNET_PORT   23

// Session structure
typedef struct {
    uint32_t client_ip;
    uint16_t client_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t authenticated;
    uint8_t active;
    char username[32];
} session_t;

// Network functions
void net_init(void);
void net_process_packet(uint8_t* packet, uint16_t length);
void net_send_tcp(session_t* session, const char* data, uint16_t length);
session_t* net_get_session(uint32_t ip, uint16_t port);
session_t* net_create_session(uint32_t ip, uint16_t port);

// Checksum calculation
uint16_t net_checksum(uint16_t* data, uint16_t length);

#endif
