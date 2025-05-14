#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SOCKETS 64
#define MAX_PACKET_SIZE 1514
#define TCP_PORT_RANGE 65535
#define TCP_WINDOW_SIZE 65535
#define TCP_MAX_SEGMENT_SIZE 1460

typedef struct {
    uint8_t version;
    uint8_t ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
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
    uint32_t timestamp;
} packet_t;

typedef struct {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t state;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t window_size;
    packet_t send_buffer[32];
    uint8_t send_buffer_head;
    uint8_t send_buffer_tail;
    packet_t recv_buffer[32];
    uint8_t recv_buffer_head;
    uint8_t recv_buffer_tail;
} tcp_socket_t;

typedef struct {
    tcp_socket_t sockets[MAX_SOCKETS];
    uint32_t socket_count;
    uint32_t local_ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    uint16_t next_port;
} network_manager_t;

network_manager_t network_manager;

void init_network_manager() {
    memset(&network_manager, 0, sizeof(network_manager_t));
    network_manager.next_port = 1024; // Ports dynamiques commencent à 1024
}

uint16_t calculate_ip_checksum(ip_header_t* header) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    
    // Calculer la somme des mots de 16 bits
    for (int i = 0; i < sizeof(ip_header_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Ajouter le carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // Complément à 1
    return ~sum;
}

uint16_t calculate_tcp_checksum(tcp_header_t* header, uint8_t* data, uint16_t data_length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)header;
    
    // Pseudo-en-tête
    sum += (network_manager.local_ip >> 16) & 0xFFFF;
    sum += network_manager.local_ip & 0xFFFF;
    sum += (header->dest_port >> 16) & 0xFFFF;
    sum += header->dest_port & 0xFFFF;
    sum += htons(IP_PROTOCOL_TCP);
    sum += htons(sizeof(tcp_header_t) + data_length);
    
    // En-tête TCP
    for (int i = 0; i < sizeof(tcp_header_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    
    // Données
    for (int i = 0; i < data_length / 2; i++) {
        sum += ntohs(((uint16_t*)data)[i]);
    }
    
    // Ajouter le dernier octet si la longueur est impaire
    if (data_length % 2) {
        sum += (data[data_length - 1] << 8);
    }
    
    // Ajouter le carry
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // Complément à 1
    return ~sum;
}

tcp_socket_t* create_socket() {
    if (network_manager.socket_count >= MAX_SOCKETS) {
        return NULL;
    }

    tcp_socket_t* socket = &network_manager.sockets[network_manager.socket_count++];
    memset(socket, 0, sizeof(tcp_socket_t));
    socket->local_port = network_manager.next_port++;
    socket->window_size = TCP_WINDOW_SIZE;
    socket->send_buffer_head = 0;
    socket->send_buffer_tail = 0;
    socket->recv_buffer_head = 0;
    socket->recv_buffer_tail = 0;

    return socket;
}

void close_socket(tcp_socket_t* socket) {
    if (!socket) return;

    // Envoyer un paquet FIN
    tcp_header_t header;
    memset(&header, 0, sizeof(tcp_header_t));
    header.source_port = socket->local_port;
    header.dest_port = socket->remote_port;
    header.sequence_number = htonl(socket->sequence_number);
    header.acknowledgment_number = htonl(socket->acknowledgment_number);
    header.data_offset = sizeof(tcp_header_t) / 4;
    header.flags = TCP_FLAG_FIN | TCP_FLAG_ACK;
    header.window_size = htons(socket->window_size);
    header.checksum = calculate_tcp_checksum(&header, NULL, 0);

    // Envoyer le paquet
    send_packet(socket, &header, NULL, 0);

    // Supprimer le socket
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        if (&network_manager.sockets[i] == socket) {
            memmove(&network_manager.sockets[i],
                    &network_manager.sockets[i + 1],
                    (network_manager.socket_count - i - 1) * sizeof(tcp_socket_t));
            network_manager.socket_count--;
            break;
        }
    }
}

int send_data(tcp_socket_t* socket, const void* data, uint16_t length) {
    if (!socket || !data || length == 0) return -1;

    // Vérifier si le buffer d'envoi est plein
    uint8_t next_tail = (socket->send_buffer_tail + 1) % 32;
    if (next_tail == socket->send_buffer_head) {
        return -1; // Buffer plein
    }

    // Copier les données dans le buffer d'envoi
    packet_t* packet = &socket->send_buffer[socket->send_buffer_tail];
    memcpy(packet->data, data, length);
    packet->length = length;
    packet->timestamp = get_current_time();
    socket->send_buffer_tail = next_tail;

    // Envoyer le paquet
    tcp_header_t header;
    memset(&header, 0, sizeof(tcp_header_t));
    header.source_port = socket->local_port;
    header.dest_port = socket->remote_port;
    header.sequence_number = htonl(socket->sequence_number);
    header.acknowledgment_number = htonl(socket->acknowledgment_number);
    header.data_offset = sizeof(tcp_header_t) / 4;
    header.flags = TCP_FLAG_ACK;
    header.window_size = htons(socket->window_size);
    header.checksum = calculate_tcp_checksum(&header, packet->data, packet->length);

    send_packet(socket, &header, packet->data, packet->length);

    // Mettre à jour le numéro de séquence
    socket->sequence_number += length;

    return length;
}

int receive_data(tcp_socket_t* socket, void* buffer, uint16_t length) {
    if (!socket || !buffer || length == 0) return -1;

    // Vérifier si le buffer de réception est vide
    if (socket->recv_buffer_head == socket->recv_buffer_tail) {
        return 0; // Aucune donnée disponible
    }

    // Copier les données du buffer de réception
    packet_t* packet = &socket->recv_buffer[socket->recv_buffer_head];
    uint16_t copy_length = min(length, packet->length);
    memcpy(buffer, packet->data, copy_length);

    // Mettre à jour le buffer de réception
    socket->recv_buffer_head = (socket->recv_buffer_head + 1) % 32;

    // Envoyer un accusé de réception
    tcp_header_t header;
    memset(&header, 0, sizeof(tcp_header_t));
    header.source_port = socket->local_port;
    header.dest_port = socket->remote_port;
    header.sequence_number = htonl(socket->sequence_number);
    header.acknowledgment_number = htonl(socket->acknowledgment_number + copy_length);
    header.data_offset = sizeof(tcp_header_t) / 4;
    header.flags = TCP_FLAG_ACK;
    header.window_size = htons(socket->window_size);
    header.checksum = calculate_tcp_checksum(&header, NULL, 0);

    send_packet(socket, &header, NULL, 0);

    // Mettre à jour le numéro d'acquittement
    socket->acknowledgment_number += copy_length;

    return copy_length;
}

void handle_tcp_packet(ip_header_t* ip_header, tcp_header_t* tcp_header, uint8_t* data, uint16_t length) {
    // Trouver le socket correspondant
    tcp_socket_t* socket = NULL;
    for (uint32_t i = 0; i < network_manager.socket_count; i++) {
        tcp_socket_t* s = &network_manager.sockets[i];
        if (s->local_port == ntohs(tcp_header->dest_port) &&
            s->remote_port == ntohs(tcp_header->source_port)) {
            socket = s;
            break;
        }
    }

    if (!socket) {
        // Nouvelle connexion
        if (tcp_header->flags & TCP_FLAG_SYN) {
            socket = create_socket();
            if (!socket) return;

            socket->remote_ip = ip_header->source_ip;
            socket->remote_port = ntohs(tcp_header->source_port);
            socket->sequence_number = 0;
            socket->acknowledgment_number = ntohl(tcp_header->sequence_number) + 1;

            // Envoyer SYN+ACK
            tcp_header_t response;
            memset(&response, 0, sizeof(tcp_header_t));
            response.source_port = socket->local_port;
            response.dest_port = socket->remote_port;
            response.sequence_number = htonl(socket->sequence_number);
            response.acknowledgment_number = htonl(socket->acknowledgment_number);
            response.data_offset = sizeof(tcp_header_t) / 4;
            response.flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
            response.window_size = htons(socket->window_size);
            response.checksum = calculate_tcp_checksum(&response, NULL, 0);

            send_packet(socket, &response, NULL, 0);
        }
        return;
    }

    // Traiter les données reçues
    if (length > 0) {
        // Vérifier si le buffer de réception est plein
        uint8_t next_tail = (socket->recv_buffer_tail + 1) % 32;
        if (next_tail != socket->recv_buffer_head) {
            // Copier les données dans le buffer de réception
            packet_t* packet = &socket->recv_buffer[socket->recv_buffer_tail];
            memcpy(packet->data, data, length);
            packet->length = length;
            packet->timestamp = get_current_time();
            socket->recv_buffer_tail = next_tail;

            // Mettre à jour le numéro d'acquittement
            socket->acknowledgment_number = ntohl(tcp_header->sequence_number) + length;
        }
    }

    // Traiter les flags
    if (tcp_header->flags & TCP_FLAG_FIN) {
        // Fermer la connexion
        close_socket(socket);
    } else if (tcp_header->flags & TCP_FLAG_ACK) {
        // Mettre à jour le numéro de séquence
        socket->sequence_number = ntohl(tcp_header->acknowledgment_number);
    }
}

void send_packet(tcp_socket_t* socket, tcp_header_t* tcp_header, uint8_t* data, uint16_t length) {
    // Créer l'en-tête IP
    ip_header_t ip_header;
    memset(&ip_header, 0, sizeof(ip_header_t));
    ip_header.version = 4;
    ip_header.ihl = sizeof(ip_header_t) / 4;
    ip_header.total_length = htons(sizeof(ip_header_t) + sizeof(tcp_header_t) + length);
    ip_header.identification = htons(1);
    ip_header.flags = htons(0x4000); // Don't fragment
    ip_header.ttl = 64;
    ip_header.protocol = IP_PROTOCOL_TCP;
    ip_header.source_ip = network_manager.local_ip;
    ip_header.dest_ip = socket->remote_ip;
    ip_header.checksum = calculate_ip_checksum(&ip_header);

    // Envoyer le paquet
    uint8_t packet[MAX_PACKET_SIZE];
    memcpy(packet, &ip_header, sizeof(ip_header_t));
    memcpy(packet + sizeof(ip_header_t), tcp_header, sizeof(tcp_header_t));
    if (data && length > 0) {
        memcpy(packet + sizeof(ip_header_t) + sizeof(tcp_header_t), data, length);
    }

    // Envoyer le paquet via le pilote réseau
    device_t* network_device = get_network_device();
    if (network_device) {
        write_device(network_device, packet, sizeof(ip_header_t) + sizeof(tcp_header_t) + length);
    }
} 