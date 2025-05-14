#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SOCKETS 256
#define MAX_PACKET_SIZE 1500
#define TCP_WINDOW_SIZE 65535
#define TCP_MSS 1460

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
} ip_header_t;

typedef struct {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} tcp_header_t;

typedef struct {
    uint8_t data[MAX_PACKET_SIZE];
    uint16_t length;
    uint32_t source_ip;
    uint32_t dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
} packet_t;

typedef struct {
    uint32_t id;
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t window_size;
    uint8_t state;
    uint8_t* receive_buffer;
    uint32_t receive_buffer_size;
    uint32_t receive_buffer_head;
    uint32_t receive_buffer_tail;
} tcp_socket_t;

typedef struct {
    tcp_socket_t sockets[MAX_SOCKETS];
    uint32_t socket_count;
    uint32_t next_socket_id;
    packet_t* packet_buffer;
    uint32_t packet_buffer_size;
    uint32_t packet_buffer_head;
    uint32_t packet_buffer_tail;
} network_manager_t;

static network_manager_t network_manager;

void init_network_manager() {
    memset(&network_manager, 0, sizeof(network_manager_t));
    network_manager.next_socket_id = 1;
    network_manager.packet_buffer = (packet_t*)kmalloc(MAX_PACKET_SIZE * 64);
    network_manager.packet_buffer_size = 64;
}

uint16_t calculate_ip_checksum(ip_header_t* header) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    for (int i = 0; i < sizeof(ip_header_t) / 2; i++) {
        sum += ptr[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

uint16_t calculate_tcp_checksum(tcp_header_t* header, uint8_t* data, uint16_t data_length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    for (int i = 0; i < sizeof(tcp_header_t) / 2; i++) {
        sum += ptr[i];
    }
    ptr = (uint16_t*)data;
    for (int i = 0; i < data_length / 2; i++) {
        sum += ptr[i];
    }
    if (data_length % 2) {
        sum += data[data_length - 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

uint32_t create_socket(uint32_t local_ip, uint16_t local_port) {
    if (network_manager.socket_count >= MAX_SOCKETS) return 0;

    tcp_socket_t* socket = &network_manager.sockets[network_manager.socket_count++];
    socket->id = network_manager.next_socket_id++;
    socket->local_ip = local_ip;
    socket->local_port = local_port;
    socket->remote_ip = 0;
    socket->remote_port = 0;
    socket->sequence_number = 0;
    socket->acknowledgment_number = 0;
    socket->window_size = TCP_WINDOW_SIZE;
    socket->state = 0;
    socket->receive_buffer = (uint8_t*)kmalloc(TCP_WINDOW_SIZE);
    socket->receive_buffer_size = TCP_WINDOW_SIZE;
    socket->receive_buffer_head = 0;
    socket->receive_buffer_tail = 0;

    return socket->id;
}

void close_socket(uint32_t socket_id) {
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            tcp_socket_t* socket = &network_manager.sockets[i];
            kfree(socket->receive_buffer);
            socket->state = 0;
            break;
        }
    }
}

bool send_data(uint32_t socket_id, const uint8_t* data, uint32_t length) {
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            tcp_socket_t* socket = &network_manager.sockets[i];
            if (!socket->remote_ip || !socket->remote_port) return false;

            uint32_t remaining = length;
            uint32_t offset = 0;

            while (remaining > 0) {
                uint32_t segment_size = remaining > TCP_MSS ? TCP_MSS : remaining;
                packet_t* packet = &network_manager.packet_buffer[network_manager.packet_buffer_tail];

                tcp_header_t* tcp_header = (tcp_header_t*)packet->data;
                tcp_header->source_port = socket->local_port;
                tcp_header->dest_port = socket->remote_port;
                tcp_header->sequence_number = socket->sequence_number;
                tcp_header->acknowledgment_number = socket->acknowledgment_number;
                tcp_header->data_offset = sizeof(tcp_header_t) / 4;
                tcp_header->flags = 0x18;
                tcp_header->window_size = socket->window_size;
                tcp_header->urgent_pointer = 0;

                memcpy(packet->data + sizeof(tcp_header_t), data + offset, segment_size);
                tcp_header->checksum = calculate_tcp_checksum(tcp_header, packet->data + sizeof(tcp_header_t), segment_size);

                ip_header_t* ip_header = (ip_header_t*)(packet->data - sizeof(ip_header_t));
                ip_header->version_ihl = 0x45;
                ip_header->tos = 0;
                ip_header->total_length = sizeof(ip_header_t) + sizeof(tcp_header_t) + segment_size;
                ip_header->identification = 0;
                ip_header->flags_fragment_offset = 0;
                ip_header->ttl = 64;
                ip_header->protocol = 6;
                ip_header->source_ip = socket->local_ip;
                ip_header->dest_ip = socket->remote_ip;
                ip_header->header_checksum = calculate_ip_checksum(ip_header);

                packet->length = sizeof(ip_header_t) + sizeof(tcp_header_t) + segment_size;
                packet->source_ip = socket->local_ip;
                packet->dest_ip = socket->remote_ip;
                packet->source_port = socket->local_port;
                packet->dest_port = socket->remote_port;

                network_manager.packet_buffer_tail = (network_manager.packet_buffer_tail + 1) % network_manager.packet_buffer_size;
                socket->sequence_number += segment_size;
                remaining -= segment_size;
                offset += segment_size;
            }

            return true;
        }
    }
    return false;
}

uint32_t receive_data(uint32_t socket_id, uint8_t* buffer, uint32_t buffer_size) {
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            tcp_socket_t* socket = &network_manager.sockets[i];
            if (socket->receive_buffer_head == socket->receive_buffer_tail) return 0;

            uint32_t available = (socket->receive_buffer_tail - socket->receive_buffer_head) % socket->receive_buffer_size;
            uint32_t to_copy = available > buffer_size ? buffer_size : available;

            if (socket->receive_buffer_head + to_copy <= socket->receive_buffer_size) {
                memcpy(buffer, socket->receive_buffer + socket->receive_buffer_head, to_copy);
            } else {
                uint32_t first_part = socket->receive_buffer_size - socket->receive_buffer_head;
                memcpy(buffer, socket->receive_buffer + socket->receive_buffer_head, first_part);
                memcpy(buffer + first_part, socket->receive_buffer, to_copy - first_part);
            }

            socket->receive_buffer_head = (socket->receive_buffer_head + to_copy) % socket->receive_buffer_size;
            return to_copy;
        }
    }
    return 0;
}

void handle_tcp_packet(packet_t* packet) {
    tcp_header_t* tcp_header = (tcp_header_t*)(packet->data + sizeof(ip_header_t));
    uint32_t data_length = packet->length - sizeof(ip_header_t) - sizeof(tcp_header_t);

    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        tcp_socket_t* socket = &network_manager.sockets[i];
        if (socket->local_ip == packet->dest_ip && socket->local_port == tcp_header->dest_port) {
            if (socket->remote_ip == 0 || socket->remote_ip == packet->source_ip) {
                if (socket->remote_port == 0 || socket->remote_port == tcp_header->source_port) {
                    if (data_length > 0) {
                        uint32_t free_space = (socket->receive_buffer_size - socket->receive_buffer_tail + socket->receive_buffer_head) % socket->receive_buffer_size;
                        if (free_space >= data_length) {
                            if (socket->receive_buffer_tail + data_length <= socket->receive_buffer_size) {
                                memcpy(socket->receive_buffer + socket->receive_buffer_tail, packet->data + sizeof(ip_header_t) + sizeof(tcp_header_t), data_length);
                            } else {
                                uint32_t first_part = socket->receive_buffer_size - socket->receive_buffer_tail;
                                memcpy(socket->receive_buffer + socket->receive_buffer_tail, packet->data + sizeof(ip_header_t) + sizeof(tcp_header_t), first_part);
                                memcpy(socket->receive_buffer, packet->data + sizeof(ip_header_t) + sizeof(tcp_header_t) + first_part, data_length - first_part);
                            }
                            socket->receive_buffer_tail = (socket->receive_buffer_tail + data_length) % socket->receive_buffer_size;
                        }
                    }

                    socket->acknowledgment_number = tcp_header->sequence_number + data_length;
                    socket->window_size = tcp_header->window_size;

                    packet_t* ack_packet = &network_manager.packet_buffer[network_manager.packet_buffer_tail];
                    tcp_header_t* ack_tcp_header = (tcp_header_t*)ack_packet->data;
                    ack_tcp_header->source_port = socket->local_port;
                    ack_tcp_header->dest_port = tcp_header->source_port;
                    ack_tcp_header->sequence_number = socket->sequence_number;
                    ack_tcp_header->acknowledgment_number = socket->acknowledgment_number;
                    ack_tcp_header->data_offset = sizeof(tcp_header_t) / 4;
                    ack_tcp_header->flags = 0x10;
                    ack_tcp_header->window_size = socket->window_size;
                    ack_tcp_header->urgent_pointer = 0;
                    ack_tcp_header->checksum = calculate_tcp_checksum(ack_tcp_header, NULL, 0);

                    ip_header_t* ack_ip_header = (ip_header_t*)(ack_packet->data - sizeof(ip_header_t));
                    ack_ip_header->version_ihl = 0x45;
                    ack_ip_header->tos = 0;
                    ack_ip_header->total_length = sizeof(ip_header_t) + sizeof(tcp_header_t);
                    ack_ip_header->identification = 0;
                    ack_ip_header->flags_fragment_offset = 0;
                    ack_ip_header->ttl = 64;
                    ack_ip_header->protocol = 6;
                    ack_ip_header->source_ip = socket->local_ip;
                    ack_ip_header->dest_ip = packet->source_ip;
                    ack_ip_header->header_checksum = calculate_ip_checksum(ack_ip_header);

                    ack_packet->length = sizeof(ip_header_t) + sizeof(tcp_header_t);
                    ack_packet->source_ip = socket->local_ip;
                    ack_packet->dest_ip = packet->source_ip;
                    ack_packet->source_port = socket->local_port;
                    ack_packet->dest_port = tcp_header->source_port;

                    network_manager.packet_buffer_tail = (network_manager.packet_buffer_tail + 1) % network_manager.packet_buffer_size;
                    break;
                }
            }
        }
    }
} 