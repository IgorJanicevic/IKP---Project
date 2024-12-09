#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include "client.h"  

#pragma comment(lib, "ws2_32.lib") 

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_BLOCKS 25  
#define NUM_CLIENT 10

pthread_mutex_t mutex;

void* allocated_blocks[MAX_BLOCKS];
int block_count = 0;

void extract_address_from_message(const char* message, void** address) {
    sscanf(message, "Memorija alocirana: %*d bajtova\nMemorija alocirana na adresi: %p\n", address);
}

void* simulate_user(void* arg) {
    SOCKET sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Neuspesno kreiranje soketa. Greska: %d\n", WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Neuspesno povezivanje sa serverom. Greska: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    printf("Uspesno ste se povezali sa serverom.\n");

    for (int i = 0; i < 10; ++i) {
        Request req;
        req.type = (rand() % 2) + 1;  
        req.size = rand() % 1024; 
        req.block_id = NULL;

        if (block_count<2){
            req.type=1;
        }         

        pthread_mutex_lock(&mutex); 

        if (req.type == 2) {
            if (block_count > 0) {
                req.block_id = allocated_blocks[block_count - 1]; 
                block_count--;  
            }
        }

        send_request(sock, &req);
        char* response_message = receive_message(sock);  

        if (req.type == 1) { 
            void* allocated_address;
            extract_address_from_message(response_message, &allocated_address);

            if (block_count < MAX_BLOCKS) {
                allocated_blocks[block_count++] = allocated_address;
            } else {
                printf("Nema više mesta za čuvanje adresa.\n");
            }
        }

        pthread_mutex_unlock(&mutex);  
    }

    closesocket(sock);
    return NULL;
}

int main() {
    WSADATA wsa;
    pthread_t users[NUM_CLIENT]; 

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspesna inicijalizacija Winsock-a. Greska: %d\n", WSAGetLastError());
        return 1;
    }

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < NUM_CLIENT; ++i) {
        if (pthread_create(&users[i], NULL, simulate_user, NULL) != 0) {
            printf("Greska prilikom kreiranja niti za korisnika %d\n", i + 1);
            WSACleanup();
            return 1;
        }
    }

    for (int i = 0; i < NUM_CLIENT; ++i) {
        pthread_join(users[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
