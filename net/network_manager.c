#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SOCKETS 256
#define MAX_PACKET_SIZE 1500
#define TCP_PORT_RANGE 65535
#define TCP_WINDOW_SIZE 65535
#define TCP_MAX_SEGMENT_SIZE 1460
#define TCP_TIMEOUT 5000

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
    uint16_t receive_buffer_size;
    uint16_t receive_buffer_used;
    uint8_t* send_buffer;
    uint16_t send_buffer_size;
    uint16_t send_buffer_used;
    uint32_t last_activity;
} tcp_socket_t;

typedef struct {
    tcp_socket_t sockets[MAX_SOCKETS];
    uint32_t socket_count;
    uint32_t next_socket_id;
    uint32_t local_ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
} network_manager_t;

network_manager_t network_manager;

void init_network_manager() {
    memset(&network_manager, 0, sizeof(network_manager_t));
    network_manager.next_socket_id = 1;
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

uint16_t calculate_tcp_checksum(tcp_header_t* header, uint8_t* data, uint16_t data_length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    uint16_t header_size = (header->data_offset >> 4) * 4;

    // Pseudo-header
    sum += (network_manager.local_ip >> 16) & 0xFFFF;
    sum += network_manager.local_ip & 0xFFFF;
    sum += (header->dest_port >> 16) & 0xFFFF;
    sum += header->dest_port & 0xFFFF;
    sum += header_size + data_length;
    sum += 6; // TCP protocol

    // TCP header
    for (uint16_t i = 0; i < header_size / 2; i++) {
        sum += ptr[i];
    }

    // TCP data
    ptr = (uint16_t*)data;
    for (uint16_t i = 0; i < data_length / 2; i++) {
        sum += ptr[i];
    }

    if (data_length % 2) {
        sum += ((uint16_t)data[data_length - 1]) << 8;
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
    socket->id = network_manager.next_socket_id++;
    socket->local_ip = network_manager.local_ip;
    socket->remote_ip = 0;
    socket->local_port = 0;
    socket->remote_port = 0;
    socket->sequence_number = 0;
    socket->acknowledgment_number = 0;
    socket->window_size = TCP_WINDOW_SIZE;
    socket->state = 0; // CLOSED
    socket->receive_buffer = kmalloc(TCP_WINDOW_SIZE);
    socket->receive_buffer_size = TCP_WINDOW_SIZE;
    socket->receive_buffer_used = 0;
    socket->send_buffer = kmalloc(TCP_WINDOW_SIZE);
    socket->send_buffer_size = TCP_WINDOW_SIZE;
    socket->send_buffer_used = 0;
    socket->last_activity = 0;

    return socket->id;
}

void close_socket(uint32_t socket_id) {
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            tcp_socket_t* socket = &network_manager.sockets[i];

            if (socket->receive_buffer) {
                kfree(socket->receive_buffer);
            }
            if (socket->send_buffer) {
                kfree(socket->send_buffer);
            }

            memmove(&network_manager.sockets[i],
                    &network_manager.sockets[i + 1],
                    (network_manager.socket_count - i - 1) * sizeof(tcp_socket_t));
            network_manager.socket_count--;
            break;
        }
    }
}

bool send_data(uint32_t socket_id, const uint8_t* data, uint16_t length) {
    tcp_socket_t* socket = NULL;
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            socket = &network_manager.sockets[i];
            break;
        }
    }

    if (!socket || socket->state != 1) {
        return false;
    }

    // Vérifier l'espace disponible dans le buffer d'envoi
    if (socket->send_buffer_used + length > socket->send_buffer_size) {
        return false;
    }

    // Copier les données dans le buffer d'envoi
    memcpy(socket->send_buffer + socket->send_buffer_used, data, length);
    socket->send_buffer_used += length;

    // Envoyer les données
    uint16_t remaining = socket->send_buffer_used;
    uint16_t offset = 0;

    while (remaining > 0) {
        uint16_t segment_size = (remaining > TCP_MAX_SEGMENT_SIZE) ? TCP_MAX_SEGMENT_SIZE : remaining;

        // Créer l'en-tête TCP
        tcp_header_t tcp_header;
        tcp_header.source_port = socket->local_port;
        tcp_header.dest_port = socket->remote_port;
        tcp_header.sequence_number = socket->sequence_number;
        tcp_header.acknowledgment_number = socket->acknowledgment_number;
        tcp_header.data_offset = 5 << 4;
        tcp_header.flags = 0x18; // ACK, PSH
        tcp_header.window_size = socket->window_size;
        tcp_header.urgent_pointer = 0;

        // Calculer le checksum
        tcp_header.checksum = calculate_tcp_checksum(&tcp_header,
                                                   socket->send_buffer + offset,
                                                   segment_size);

        // Créer l'en-tête IP
        ip_header_t ip_header;
        ip_header.version_ihl = 0x45;
        ip_header.tos = 0;
        ip_header.total_length = sizeof(ip_header_t) + sizeof(tcp_header_t) + segment_size;
        ip_header.identification = 0;
        ip_header.flags_fragment_offset = 0;
        ip_header.ttl = 64;
        ip_header.protocol = 6; // TCP
        ip_header.source_ip = socket->local_ip;
        ip_header.dest_ip = socket->remote_ip;

        // Calculer le checksum
        ip_header.header_checksum = calculate_ip_checksum(&ip_header);

        // Envoyer le paquet
        packet_t packet;
        memcpy(packet.data, &ip_header, sizeof(ip_header_t));
        memcpy(packet.data + sizeof(ip_header_t), &tcp_header, sizeof(tcp_header_t));
        memcpy(packet.data + sizeof(ip_header_t) + sizeof(tcp_header_t),
               socket->send_buffer + offset,
               segment_size);
        packet.length = sizeof(ip_header_t) + sizeof(tcp_header_t) + segment_size;
        packet.source_ip = socket->local_ip;
        packet.dest_ip = socket->remote_ip;
        packet.source_port = socket->local_port;
        packet.dest_port = socket->remote_port;

        // TODO: Envoyer le paquet via le pilote réseau

        // Mettre à jour les compteurs
        socket->sequence_number += segment_size;
        offset += segment_size;
        remaining -= segment_size;
    }

    // Vider le buffer d'envoi
    socket->send_buffer_used = 0;
    return true;
}

bool receive_data(uint32_t socket_id, uint8_t* data, uint16_t* length) {
    tcp_socket_t* socket = NULL;
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (network_manager.sockets[i].id == socket_id) {
            socket = &network_manager.sockets[i];
            break;
        }
    }

    if (!socket || socket->state != 1) {
        return false;
    }

    // Vérifier s'il y a des données disponibles
    if (socket->receive_buffer_used == 0) {
        *length = 0;
        return true;
    }

    // Copier les données du buffer de réception
    uint16_t copy_length = (socket->receive_buffer_used > *length) ? *length : socket->receive_buffer_used;
    memcpy(data, socket->receive_buffer, copy_length);
    *length = copy_length;

    // Déplacer les données restantes
    if (copy_length < socket->receive_buffer_used) {
        memmove(socket->receive_buffer,
                socket->receive_buffer + copy_length,
                socket->receive_buffer_used - copy_length);
    }
    socket->receive_buffer_used -= copy_length;

    return true;
}

void handle_tcp_packet(packet_t* packet) {
    ip_header_t* ip_header = (ip_header_t*)packet->data;
    tcp_header_t* tcp_header = (tcp_header_t*)(packet->data + sizeof(ip_header_t));
    uint8_t* tcp_data = packet->data + sizeof(ip_header_t) + sizeof(tcp_header_t);
    uint16_t tcp_data_length = packet->length - sizeof(ip_header_t) - sizeof(tcp_header_t);

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
            socket->receive_buffer = kmalloc(TCP_WINDOW_SIZE);
            socket->receive_buffer_size = TCP_WINDOW_SIZE;
            socket->receive_buffer_used = 0;
            socket->send_buffer = kmalloc(TCP_WINDOW_SIZE);
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
    if (tcp_data_length > 0 && socket->state == 4) { // ESTABLISHED
        // Vérifier l'espace disponible dans le buffer de réception
        if (socket->receive_buffer_used + tcp_data_length <= socket->receive_buffer_size) {
            memcpy(socket->receive_buffer + socket->receive_buffer_used,
                   tcp_data,
                   tcp_data_length);
            socket->receive_buffer_used += tcp_data_length;
            socket->acknowledgment_number += tcp_data_length;
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
    write_device(device, packet->data, packet->length, 0);
} 