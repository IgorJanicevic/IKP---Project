
#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include "heap_manager.h"
#include <windows.h>

#define SERVER_PORT 8080
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 5

// Struktura za zahtev
typedef struct Request {
    int type;       // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;    // Velicina za alokaciju (ako je tip 1)
    void* block_id; // Adresa bloka za dealokaciju (ako je tip 2)
} Request;

void enqueue(Request req);

Request dequeue();

void process_request(Request req);
void send_message(SOCKET sock);
void* thread_pool_worker(void* arg);
void* handle_client(void* client_socket_ptr);
void* accept_clients(void* server_fd_ptr);

#endif // SERVER_H
