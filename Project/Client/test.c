#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <pthread.h>


void* allocated_blocks[MAX_BLOCKS];
int block_count = 0;
pthread_mutex_t mutex;


void* client_thread_function(void* arg) {
    int client_id = (int)(size_t)arg;
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Klijent %d: Neuspesno inicijalizovanje Winsock-a. Greska: %d\n", client_id, WSAGetLastError());
        return NULL;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Klijent %d: Neuspesno kreiranje soketa. Greska: %d\n", client_id, WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Klijent %d: Neuspesno povezivanje na server. Greska: %d\n", client_id, WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    printf("Klijent %d: Uspesno povezivanje sa serverom.\n", client_id);

    // Simulacija random operacija
    test_random_operations(sock, client_id);

    closesocket(sock);
    WSACleanup();
    return NULL;
}

void extract_block_address(const char* message) {
    void* address;
    sscanf(message, "Memorija alocirana: %*d bajtova\nMemorija alocirana na adresi: %p\n", &address);
    if (block_count < MAX_BLOCKS) {
        allocated_blocks[block_count++] = address;
    } else {
        printf("Dostignut maksimalan broj blokova.\n");
    }
}

void test_allocation(SOCKET sock, int client_id, int num_blocks) {
    for (int i = 0; i < num_blocks; i++) {
        Request req = { .type = 1, .size = (rand() % 512) + 1, .block_id = NULL };
        send_request(sock, &req);
        
        char* message = receive_message(sock);
        if (message) {
            printf("Klijent %d: Primljena poruka: %s\n", client_id, message);
            extract_block_address(message);
            free(message);
        }
    }
}



void test_deallocation(SOCKET sock, int client_id) {
    pthread_mutex_lock(&mutex); // Lock the mutex to ensure thread safety

    for (int i = 0; i < block_count; i++) {
        Request req = { .type = 2, .size = 0, .block_id = allocated_blocks[i] };
        send_request(sock, &req);
        receive_message(sock);
        allocated_blocks[i] = NULL;
    }

    pthread_mutex_unlock(&mutex); // Unlock the mutex

    block_count = 0;
}

void test_random_operations(SOCKET sock, int client_id) {
    int operations = (rand() % 10) + 5;
    for (int i = 0; i < operations; i++) {
        int action = rand() % 2; // 0: alociraj, 1: delociraj
        if (action == 0 && block_count < MAX_BLOCKS) {
            test_allocation(sock, client_id, 1);
        } else if (action == 1 && block_count > 0) {
            int index = rand() % block_count;
            Request req = { .type = 2, .size = 0, .block_id = allocated_blocks[index] };
            send_request(sock, &req);
            receive_message(sock);
            allocated_blocks[index] = allocated_blocks[--block_count];
        }
    }
}



int main() {
   srand((unsigned int)time(NULL));
    printf("Pokretanje testa...\n");

    // Kreiranje i pokretanje klijent nitova
    pthread_t threads[NUM_CLIENTS];
    for (int i = 0; i < NUM_CLIENTS; i++) {
        if (pthread_create(&threads[i], NULL, client_thread_function, (void*)(size_t)i) != 0) {
            printf("Neuspesno kreiranje niti za klijenta %d.\n", i);
            exit(1);
        }
    }

    // Cekanje da sve niti zavrse
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Test zavrsen.\n");
    return 0;
}