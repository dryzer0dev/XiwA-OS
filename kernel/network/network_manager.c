#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SOCKETS 64
#define MAX_PACKET_SIZE 1514
#define TCP_PORT_RANGE 65535
#define TCP_WINDOW_SIZE 65535
#define TCP_MAX_SEGMENT_SIZE 1460

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
    uint16_t size;
    uint32_t source_ip;
    uint32_t dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
} packet_t;

typedef struct {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t window_size;
    uint8_t state;
    uint8_t* receive_buffer;
    uint16_t receive_buffer_size;
    uint16_t receive_buffer_used;
    uint8_t* send_buffer;
    uint16_t send_buffer_size;
    uint16_t send_buffer_used;
} tcp_socket_t;

typedef struct {
    tcp_socket_t sockets[MAX_SOCKETS];
    uint32_t socket_count;
    uint32_t local_ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
} network_manager_t;

network_manager_t network_manager;

void init_network_manager() {
    memset(&network_manager, 0, sizeof(network_manager_t));
}

uint16_t calculate_ip_checksum(ip_header_t* header) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    uint16_t header_size = (header->version_ihl & 0x0F) * 4;

    // Calculer la somme des mots de 16 bits
    for (uint16_t i = 0; i < header_size / 2; i++) {
        sum += ptr[i];
    }

    // Ajouter les retenues
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Complément à 1
    return ~sum;
}

uint16_t calculate_tcp_checksum(tcp_header_t* header, uint8_t* data, uint16_t data_size) {
    uint32_t sum = 0;
    uint16_t* ptr;

    // Pseudo-en-tête
    sum += (network_manager.local_ip >> 16) & 0xFFFF;
    sum += network_manager.local_ip & 0xFFFF;
    sum += (header->dest_port >> 16) & 0xFFFF;
    sum += header->dest_port & 0xFFFF;
    sum += 6; // TCP
    sum += sizeof(tcp_header_t) + data_size;

    // En-tête TCP
    ptr = (uint16_t*)header;
    for (uint16_t i = 0; i < sizeof(tcp_header_t) / 2; i++) {
        sum += ptr[i];
    }

    // Données
    ptr = (uint16_t*)data;
    for (uint16_t i = 0; i < data_size / 2; i++) {
        sum += ptr[i];
    }

    // Ajouter les retenues
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Complément à 1
    return ~sum;
}

uint32_t create_socket() {
    if (network_manager.socket_count >= MAX_SOCKETS) {
        return 0;
    }

    tcp_socket_t* socket = &network_manager.sockets[network_manager.socket_count++];
    socket->local_ip = network_manager.local_ip;
    socket->remote_ip = 0;
    socket->local_port = 0;
    socket->remote_port = 0;
    socket->sequence_number = 0;
    socket->acknowledgment_number = 0;
    socket->window_size = TCP_WINDOW_SIZE;
    socket->state = 0; // CLOSED
    socket->receive_buffer = (uint8_t*)kmalloc(TCP_WINDOW_SIZE);
    socket->receive_buffer_size = TCP_WINDOW_SIZE;
    socket->receive_buffer_used = 0;
    socket->send_buffer = (uint8_t*)kmalloc(TCP_WINDOW_SIZE);
    socket->send_buffer_size = TCP_WINDOW_SIZE;
    socket->send_buffer_used = 0;

    return network_manager.socket_count;
}

void close_socket(uint32_t socket_id) {
    if (socket_id >= network_manager.socket_count) {
        return;
    }

    tcp_socket_t* socket = &network_manager.sockets[socket_id - 1];
    kfree(socket->receive_buffer);
    kfree(socket->send_buffer);

    // Supprimer le socket
    memmove(&network_manager.sockets[socket_id - 1],
            &network_manager.sockets[socket_id],
            (network_manager.socket_count - socket_id) * sizeof(tcp_socket_t));
    network_manager.socket_count--;
}

int send_data(uint32_t socket_id, const void* data, size_t size) {
    if (socket_id >= network_manager.socket_count) {
        return -1;
    }

    tcp_socket_t* socket = &network_manager.sockets[socket_id - 1];
    if (socket->state != 4) { // ESTABLISHED
        return -1;
    }

    // Vérifier l'espace disponible dans le buffer d'envoi
    if (socket->send_buffer_used + size > socket->send_buffer_size) {
        return -1;
    }

    // Copier les données dans le buffer d'envoi
    memcpy(socket->send_buffer + socket->send_buffer_used, data, size);
    socket->send_buffer_used += size;

    // Envoyer les données
    while (socket->send_buffer_used > 0) {
        uint16_t segment_size = min(socket->send_buffer_used, TCP_MAX_SEGMENT_SIZE);
        packet_t packet;

        // Construire l'en-tête IP
        ip_header_t* ip_header = (ip_header_t*)packet.data;
        ip_header->version_ihl = 0x45; // IPv4, 5 mots de 32 bits
        ip_header->tos = 0;
        ip_header->total_length = sizeof(ip_header_t) + sizeof(tcp_header_t) + segment_size;
        ip_header->identification = 0;
        ip_header->flags_fragment_offset = 0;
        ip_header->ttl = 64;
        ip_header->protocol = 6; // TCP
        ip_header->source_ip = socket->local_ip;
        ip_header->dest_ip = socket->remote_ip;
        ip_header->header_checksum = calculate_ip_checksum(ip_header);

        // Construire l'en-tête TCP
        tcp_header_t* tcp_header = (tcp_header_t*)(packet.data + sizeof(ip_header_t));
        tcp_header->source_port = socket->local_port;
        tcp_header->dest_port = socket->remote_port;
        tcp_header->sequence_number = socket->sequence_number;
        tcp_header->acknowledgment_number = socket->acknowledgment_number;
        tcp_header->data_offset = sizeof(tcp_header_t) / 4;
        tcp_header->flags = 0x18; // ACK, PSH
        tcp_header->window_size = socket->window_size;
        tcp_header->urgent_pointer = 0;
        tcp_header->checksum = calculate_tcp_checksum(tcp_header,
                                                    socket->send_buffer,
                                                    segment_size);

        // Copier les données
        memcpy(packet.data + sizeof(ip_header_t) + sizeof(tcp_header_t),
               socket->send_buffer,
               segment_size);

        // Envoyer le paquet
        send_packet(&packet);

        // Mettre à jour les compteurs
        socket->sequence_number += segment_size;
        memmove(socket->send_buffer,
                socket->send_buffer + segment_size,
                socket->send_buffer_used - segment_size);
        socket->send_buffer_used -= segment_size;
    }

    return size;
}

int receive_data(uint32_t socket_id, void* buffer, size_t size) {
    if (socket_id >= network_manager.socket_count) {
        return -1;
    }

    tcp_socket_t* socket = &network_manager.sockets[socket_id - 1];
    if (socket->state != 4) { // ESTABLISHED
        return -1;
    }

    // Vérifier les données disponibles
    if (socket->receive_buffer_used == 0) {
        return 0;
    }

    // Copier les données
    size_t bytes_to_copy = min(size, socket->receive_buffer_used);
    memcpy(buffer, socket->receive_buffer, bytes_to_copy);

    // Mettre à jour le buffer
    memmove(socket->receive_buffer,
            socket->receive_buffer + bytes_to_copy,
            socket->receive_buffer_used - bytes_to_copy);
    socket->receive_buffer_used -= bytes_to_copy;

    return bytes_to_copy;
}

void handle_tcp_packet(packet_t* packet) {
    ip_header_t* ip_header = (ip_header_t*)packet->data;
    tcp_header_t* tcp_header = (tcp_header_t*)(packet->data + sizeof(ip_header_t));
    uint8_t* data = packet->data + sizeof(ip_header_t) + sizeof(tcp_header_t);
    uint16_t data_size = ip_header->total_length - sizeof(ip_header_t) - sizeof(tcp_header_t);

    // Trouver le socket correspondant
    tcp_socket_t* socket = NULL;
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].local_port == tcp_header->dest_port &&
            network_manager.sockets[i].remote_port == tcp_header->source_port &&
            network_manager.sockets[i].local_ip == ip_header->dest_ip &&
            network_manager.sockets[i].remote_ip == ip_header->source_ip) {
            socket = &network_manager.sockets[i];
            break;
        }
    }

    if (!socket) {
        // Nouvelle connexion
        if (tcp_header->flags & 0x02) { // SYN
            socket = &network_manager.sockets[network_manager.socket_count++];
            socket->local_ip = ip_header->dest_ip;
            socket->remote_ip = ip_header->source_ip;
            socket->local_port = tcp_header->dest_port;
            socket->remote_port = tcp_header->source_port;
            socket->sequence_number = 0;
            socket->acknowledgment_number = tcp_header->sequence_number + 1;
            socket->window_size = TCP_WINDOW_SIZE;
            socket->state = 1; // LISTEN
            socket->receive_buffer = (uint8_t*)kmalloc(TCP_WINDOW_SIZE);
            socket->receive_buffer_size = TCP_WINDOW_SIZE;
            socket->receive_buffer_used = 0;
            socket->send_buffer = (uint8_t*)kmalloc(TCP_WINDOW_SIZE);
            socket->send_buffer_size = TCP_WINDOW_SIZE;
            socket->send_buffer_used = 0;

            // Envoyer SYN-ACK
            packet_t response;
            ip_header_t* response_ip = (ip_header_t*)response.data;
            tcp_header_t* response_tcp = (tcp_header_t*)(response.data + sizeof(ip_header_t));

            response_ip->version_ihl = 0x45;
            response_ip->tos = 0;
            response_ip->total_length = sizeof(ip_header_t) + sizeof(tcp_header_t);
            response_ip->identification = 0;
            response_ip->flags_fragment_offset = 0;
            response_ip->ttl = 64;
            response_ip->protocol = 6;
            response_ip->source_ip = socket->local_ip;
            response_ip->dest_ip = socket->remote_ip;
            response_ip->header_checksum = calculate_ip_checksum(response_ip);

            response_tcp->source_port = socket->local_port;
            response_tcp->dest_port = socket->remote_port;
            response_tcp->sequence_number = socket->sequence_number;
            response_tcp->acknowledgment_number = socket->acknowledgment_number;
            response_tcp->data_offset = sizeof(tcp_header_t) / 4;
            response_tcp->flags = 0x12; // SYN, ACK
            response_tcp->window_size = socket->window_size;
            response_tcp->urgent_pointer = 0;
            response_tcp->checksum = calculate_tcp_checksum(response_tcp, NULL, 0);

            send_packet(&response);
            socket->state = 2; // SYN_RECEIVED
        }
        return;
    }

    // Gérer les drapeaux TCP
    if (tcp_header->flags & 0x02) { // SYN
        if (socket->state == 1) { // LISTEN
            socket->acknowledgment_number = tcp_header->sequence_number + 1;
            socket->state = 2; // SYN_RECEIVED
        }
    } else if (tcp_header->flags & 0x10) { // ACK
        if (socket->state == 2) { // SYN_RECEIVED
            socket->state = 4; // ESTABLISHED
        }
    } else if (tcp_header->flags & 0x01) { // FIN
        socket->state = 5; // CLOSE_WAIT
    }

    // Traiter les données
    if (data_size > 0 && socket->state == 4) { // ESTABLISHED
        // Vérifier l'espace disponible dans le buffer de réception
        if (socket->receive_buffer_used + data_size <= socket->receive_buffer_size) {
            memcpy(socket->receive_buffer + socket->receive_buffer_used,
                   data,
                   data_size);
            socket->receive_buffer_used += data_size;
            socket->acknowledgment_number += data_size;
        }

        // Envoyer ACK
        packet_t response;
        ip_header_t* response_ip = (ip_header_t*)response.data;
        tcp_header_t* response_tcp = (tcp_header_t*)(response.data + sizeof(ip_header_t));

        response_ip->version_ihl = 0x45;
        response_ip->tos = 0;
        response_ip->total_length = sizeof(ip_header_t) + sizeof(tcp_header_t);
        response_ip->identification = 0;
        response_ip->flags_fragment_offset = 0;
        response_ip->ttl = 64;
        response_ip->protocol = 6;
        response_ip->source_ip = socket->local_ip;
        response_ip->dest_ip = socket->remote_ip;
        response_ip->header_checksum = calculate_ip_checksum(response_ip);

        response_tcp->source_port = socket->local_port;
        response_tcp->dest_port = socket->remote_port;
        response_tcp->sequence_number = socket->sequence_number;
        response_tcp->acknowledgment_number = socket->acknowledgment_number;
        response_tcp->data_offset = sizeof(tcp_header_t) / 4;
        response_tcp->flags = 0x10; // ACK
        response_tcp->window_size = socket->window_size;
        response_tcp->urgent_pointer = 0;
        response_tcp->checksum = calculate_tcp_checksum(response_tcp, NULL, 0);

        send_packet(&response);
    }
}

void send_packet(packet_t* packet) {
    // Trouver l'interface réseau
    device_t* device = find_device_by_type(DEVICE_TYPE_NETWORK);
    if (!device) return;

    // Envoyer le paquet
    write_device(device, packet->data, packet->size, 0);
} 