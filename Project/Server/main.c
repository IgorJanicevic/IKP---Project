#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib") // Linkovanje Winsock biblioteke

#define SERVER_PORT 8080
#define MAX_CLIENTS 10

// Struktura za zahtev
typedef struct Request {
    int type;           // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;        // Velicina za alokaciju (ako je tip 1)
    int block_id;       // ID bloka za dealokaciju (ako je tip 2)
} Request;

// Red cekanja za zahteve
Request request_queue[MAX_CLIENTS];
int queue_front = 0;
int queue_rear = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcija za dodavanje zahteva u red
void enqueue(Request req) {
    pthread_mutex_lock(&queue_mutex);
    request_queue[queue_rear] = req;
    queue_rear = (queue_rear + 1) % MAX_CLIENTS;
    pthread_mutex_unlock(&queue_mutex);
}

// Funkcija za uzimanje zahteva iz reda
Request dequeue() {
    pthread_mutex_lock(&queue_mutex);
    Request req = request_queue[queue_front];
    queue_front = (queue_front + 1) % MAX_CLIENTS;
    pthread_mutex_unlock(&queue_mutex);
    return req;
}

// Funkcija za obradu zahteva
void process_request(Request req) {
    if (req.type == 1) { // Alokacija
        printf("Zahtev za alokaciju memorije: %zu bajtova\n", req.size);
        // Dodaj logiku za alokaciju memorije ovde
    } else if (req.type == 2) { // Dealokacija
        printf("Zahtev za dealokaciju memorije: ID bloka %d\n", req.block_id);
        // Dodaj logiku za dealokaciju memorije ovde
    }
}

// Funkcija koja obradjuje zahteve iz reda
void* process_requests(void* arg) {
    while (1) {
        if (queue_front != queue_rear) {
            Request req = dequeue();
            process_request(req);
        }
        Sleep(100); // Pauza kako bi se izbeglo bespotrebno koriscenje CPU-a
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspesna inicijalizacija Winsock-a. Greska: %d\n", WSAGetLastError());
        return 1;
    }

    // Kreiranje soketa
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Neuspesno kreiranje soketa. Greska: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bindovanje soketa
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Neuspesno bindovanje. Greska: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Slusanje za klijente
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Neuspesno slusanje. Greska: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server je pokrenut. Cekam klijente...\n");

    // Kreiranje niti za obradu zahteva
    pthread_t processing_thread;
    pthread_create(&processing_thread, NULL, process_requests, NULL);

    // Prihvatanje klijenata i dodavanje zahteva u red
    while ((client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) != INVALID_SOCKET) {
        Request req;
        int bytes_received = recv(client_socket, (char*)&req, sizeof(Request), 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("Greska pri primanju podataka. Greska: %d\n", WSAGetLastError());
        } else if (bytes_received > 0) {
            printf("Primljen zahtev od klijenta.\n");
            enqueue(req);
        }

        closesocket(client_socket); // Zatvaranje veze sa klijentom
    }

    // Ciscenje resursa
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
