#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib") 

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define ADDRESS_LENGTH 16

typedef struct Request {
    int type;           // 1 - Alokacija, 2 - Delokacija
    size_t size;        // Velicina bloka za alociranje
    void* block_id;     // Adresa bloka za delociranje
} Request;

void send_request(SOCKET sock, Request* req);
char* receive_message(SOCKET sock);
int is_valid_hex_address(const char* input);

#endif // CLIENT_H
