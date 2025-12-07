#include "include/net.h"
#include "include/rtl8139.h"
#include "include/vga.h"
#include "include/auth.h"
#include "include/shell.h"
#include "include/string.h"

static uint8_t my_mac[6] = MY_MAC_ADDR;
static session_t sessions[5];
static uint8_t num_sessions = 0;

// Helper to convert network byte order
static uint16_t htons(uint16_t x) {
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
}

static uint32_t htonl(uint32_t x) {
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | 
           ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
}

void net_init(void) {
    // Initialize all sessions as inactive
    for (int i = 0; i < 5; i++) {
        sessions[i].active = 0;
        sessions[i].authenticated = 0;
    }
    num_sessions = 0;
}

uint16_t net_checksum(uint16_t* data, uint16_t length) {
    uint32_t sum = 0;
    
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    
    if (length == 1) {
        sum += *(uint8_t*)data;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

session_t* net_get_session(uint32_t ip, uint16_t port) {
    for (int i = 0; i < 5; i++) {
        if (sessions[i].active && 
            sessions[i].client_ip == ip && 
            sessions[i].client_port == port) {
            return &sessions[i];
        }
    }
    return 0;
}

session_t* net_create_session(uint32_t ip, uint16_t port) {
    if (num_sessions >= 5) return 0;
    
    for (int i = 0; i < 5; i++) {
        if (!sessions[i].active) {
            sessions[i].active = 1;
            sessions[i].authenticated = 0;
            sessions[i].client_ip = ip;
            sessions[i].client_port = port;
            sessions[i].seq_num = 1000;
            sessions[i].ack_num = 0;
            num_sessions++;
            return &sessions[i];
        }
    }
    return 0;
}

void net_send_tcp(session_t* session, const char* data, uint16_t data_len) {
    if (!session) return;
    
    uint16_t total_len = sizeof(eth_header_t) + sizeof(ip_header_t) + 
                         sizeof(tcp_header_t) + data_len;
    uint8_t packet[total_len];
    
    // Ethernet header
    eth_header_t* eth = (eth_header_t*)packet;
    for (int i = 0; i < 6; i++) {
        eth->dest_mac[i] = 0xFF;  // Broadcast for now
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = htons(0x0800);  // IPv4
    
    // IP header
    ip_header_t* ip = (ip_header_t*)(packet + sizeof(eth_header_t));
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(sizeof(ip_header_t) + sizeof(tcp_header_t) + data_len);
    ip->id = htons(1234);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = 6;  // TCP
    ip->src_ip = htonl(MY_IP_ADDR);
    ip->dest_ip = htonl(session->client_ip);
    ip->checksum = 0;
    ip->checksum = net_checksum((uint16_t*)ip, sizeof(ip_header_t));
    
    // TCP header
    tcp_header_t* tcp = (tcp_header_t*)(packet + sizeof(eth_header_t) + sizeof(ip_header_t));
    tcp->src_port = htons(TELNET_PORT);
    tcp->dest_port = htons(session->client_port);
    tcp->seq_num = htonl(session->seq_num);
    tcp->ack_num = htonl(session->ack_num);
    tcp->data_offset = 0x50;  // 20 bytes, no options
    tcp->flags = TCP_PSH | TCP_ACK;
    tcp->window = htons(8192);
    tcp->urgent_ptr = 0;
    tcp->checksum = 0;
    
    // Copy data
    if (data_len > 0) {
        uint8_t* payload = packet + sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(tcp_header_t);
        for (uint16_t i = 0; i < data_len; i++) {
            payload[i] = data[i];
        }
    }
    
    // Calculate TCP checksum (simplified - not including pseudo-header)
    tcp->checksum = net_checksum((uint16_t*)tcp, sizeof(tcp_header_t) + data_len);
    
    // Send packet
    rtl8139_send_packet(packet, total_len);
    
    // Update sequence number
    session->seq_num += data_len;
}

void net_process_packet(uint8_t* packet, uint16_t length) {
    if (length < sizeof(eth_header_t) + sizeof(ip_header_t)) return;
    
    eth_header_t* eth = (eth_header_t*)packet;
    
    // Check if IPv4
    if (htons(eth->ethertype) != 0x0800) return;
    
    ip_header_t* ip = (ip_header_t*)(packet + sizeof(eth_header_t));
    
    // Check if TCP
    if (ip->protocol != 6) return;
    
    tcp_header_t* tcp = (tcp_header_t*)(packet + sizeof(eth_header_t) + sizeof(ip_header_t));
    
    uint32_t src_ip = htonl(ip->src_ip);
    uint16_t src_port = htons(tcp->src_port);
    uint16_t dest_port = htons(tcp->dest_port);
    
    // Check if packet is for our telnet port
    if (dest_port != TELNET_PORT) return;
    
    session_t* session = net_get_session(src_ip, src_port);
    
    // Handle SYN (new connection)
    if (tcp->flags & TCP_SYN) {
        if (!session) {
            session = net_create_session(src_ip, src_port);
            if (session) {
                vga_print("New connection from IP\n");
                session->ack_num = htonl(tcp->seq_num) + 1;
                
                // Send SYN-ACK
                uint8_t syn_ack[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(tcp_header_t)];
                // ... build SYN-ACK packet ...
                
                // Send login prompt
                const char* prompt = "XAE OS Login\nUsername: ";
                net_send_tcp(session, prompt, strlen(prompt));
            }
        }
        return;
    }
    
    // Handle data packet
    if (session && (tcp->flags & TCP_PSH)) {
        uint16_t data_offset = (tcp->data_offset >> 4) * 4;
        uint8_t* payload = (uint8_t*)tcp + data_offset;
        uint16_t payload_len = htons(ip->total_length) - sizeof(ip_header_t) - data_offset;
        
        // Update ACK
        session->ack_num = htonl(tcp->seq_num) + payload_len;
        
        // Decrypt payload
        decrypt_data(payload, payload_len, 0x42);
        
        if (!session->authenticated) {
            // Handle authentication
            char username[32] = {0};
            char password[64] = {0};
            
            // Parse username:password from payload
            uint8_t i = 0, j = 0;
            while (i < payload_len && payload[i] != ':' && j < 31) {
                username[j++] = payload[i++];
            }
            i++; // skip ':'
            j = 0;
            while (i < payload_len && payload[i] != '\n' && j < 63) {
                password[j++] = payload[i++];
            }
            
            if (auth_verify(username, password)) {
                session->authenticated = 1;
                strcpy(session->username, username);
                const char* welcome = "\nWelcome to XAE OS!\n> ";
                net_send_tcp(session, welcome, strlen(welcome));
            } else {
                const char* fail = "\nAuthentication failed!\nUsername: ";
                net_send_tcp(session, fail, strlen(fail));
            }
        } else {
            // Execute shell command
            char cmd[256] = {0};
            for (uint16_t i = 0; i < payload_len && i < 255; i++) {
                cmd[i] = payload[i];
            }
            
            // Process command through shell
            extern void shell_execute_command(const char* cmd, session_t* session);
            shell_execute_command(cmd, session);
        }
    }
}
