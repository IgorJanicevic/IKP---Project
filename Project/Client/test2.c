#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include "client.h"
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_BLOCKS 25
#define NUM_CLIENT 5

void* allocated_blocks[NUM_CLIENT][MAX_BLOCKS];
int block_count[NUM_CLIENT] = {0};
pthread_mutex_t global_mutex;

void extract_address_from_message(const char* message, void** address) {
    if (sscanf(message, "Memorija alocirana: %*d bajtova\nMemorija alocirana na adresi: %p\n", address) != 1) {
        *address = NULL;
        printf("Greska: Neuspela ekstrakcija adrese iz poruke: %s\n", message);
    }
}

void connect_clients(SOCKET* socks) {
    struct sockaddr_in server_addr;

    for (int i = 0; i < NUM_CLIENT; ++i) {
        if ((socks[i] = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            printf("Klijent %d: neuspesno kreiranje soketa. Greska: %d\n", i, WSAGetLastError());
            WSACleanup();
            exit(1);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(socks[i], (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            printf("Klijent %d: neuspesno povezivanje sa serverom. Greska: %d\n", i, WSAGetLastError());
            closesocket(socks[i]);
            WSACleanup();
            exit(1);
        }

        printf("Klijent %d: uspesno povezan sa serverom.\n", i);
    }
}

void* simulate_user(void* arg) {
    int client_id = *(int*)arg;
    SOCKET sock;
    struct sockaddr_in server_addr;
    srand((unsigned int)time(NULL) + client_id);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Klijent %d: neuspesno kreiranje soketa. Greska: %d\n", client_id, WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Klijent %d: neuspesno povezivanje sa serverom. Greska: %d\n", client_id, WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    printf("Klijent %d: uspesno povezan sa serverom.\n", client_id);

    // Alokacija memorije
    for (int i = 0; i < 6; ++i) {
        pthread_mutex_lock(&global_mutex);
        Request req;
        req.type = 1; 
        req.size = rand() % 1024 + 1; 
        req.block_id = NULL;

        send_request(sock, &req);
        char* response_message = receive_message(sock);

        void* allocated_address;
        extract_address_from_message(response_message, &allocated_address);

        if (block_count[client_id] < MAX_BLOCKS) {
            allocated_blocks[client_id][block_count[client_id]++] = allocated_address;
            printf("Klijent %d: alociran blok na adresi %p\n", client_id, allocated_address);
        } else {
            printf("Klijent %d: nema vise mesta za cuvanje adresa.\n", client_id);
        }
        pthread_mutex_unlock(&global_mutex);
    }

    // Delokacija memorije
    pthread_mutex_lock(&global_mutex);
    for (int i = 0; i < 5; ++i) {
        if (block_count[client_id] > 0) {
            Request req;
            req.type = 2; // Delokacija
            req.size = 0;
            req.block_id = allocated_blocks[client_id][--block_count[client_id]];

            pthread_mutex_unlock(&global_mutex);
            send_request(sock, &req);
            char* response_message = receive_message(sock);

            printf("Klijent %d: delokacija bloka na adresi %p - odgovor: %s\n", client_id, req.block_id, response_message);
            pthread_mutex_lock(&global_mutex);
        }
    }
    pthread_mutex_unlock(&global_mutex);

    closesocket(sock);
    return NULL;
}

int main() {
    WSADATA wsa;
    pthread_t users[NUM_CLIENT];
    int client_ids[NUM_CLIENT];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspesna inicijalizacija Winsock-a. Greska: %d\n", WSAGetLastError());
        return 1;
    }

    pthread_mutex_init(&global_mutex, NULL);

    for (int i = 0; i < NUM_CLIENT; ++i) {
        client_ids[i] = i;
        if (pthread_create(&users[i], NULL, simulate_user, &client_ids[i]) != 0) {
            printf("Greska prilikom kreiranja niti za klijenta %d\n", i);
            WSACleanup();
            return 1;
        }
    }

    for (int i = 0; i < NUM_CLIENT; ++i) {
        pthread_join(users[i], NULL);
    }

    pthread_mutex_destroy(&global_mutex);
    WSACleanup();
    return 0;
}