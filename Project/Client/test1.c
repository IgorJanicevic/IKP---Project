#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>  // Za visestruke niti

#pragma comment(lib, "ws2_32.lib") // Linkovanje Winsock biblioteke

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Struktura za zahtev
typedef struct Request {
    int type;           // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;        // Velicina za alokaciju (ako je tip 1)
    void* block_id;     // ID bloka za dealokaciju (ako je tip 2)
} Request;

// Funkcija za slanje zahteva serveru
void send_request(SOCKET sock, Request* req) {
    if (send(sock, (char*)req, sizeof(Request), 0) == SOCKET_ERROR) {
        printf("Neuspesno slanje zahteva. Greska: %d\n", WSAGetLastError());
        exit(1);
    }
    printf("Zahtev poslat serveru.\n");
}

// Funkcija koja simulira korisnika koji salje nekoliko zahteva serveru
void* simulate_user(void* arg) {
    SOCKET sock;
    struct sockaddr_in server_addr;

    // Kreiranje soketa
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Neuspesno kreiranje soketa. Greska: %d\n", WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Povezivanje na server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Neuspesno povezivanje sa serverom. Greska: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return NULL;
    }

    printf("Uspesno ste se povezali sa serverom.\n");

    // Simulacija slanja nekoliko zahteva
    for (int i = 0; i < 10; ++i) {
        Request req;
        req.type = (i % 2 == 0) ? 1 : 2;  // Alternira izmedju alokacije i dealokacije
        //req.type = 1;
        req.size = 1024 * (i + 1);         // Velicina za alokaciju
        req.block_id = (void*)(i + 100);   // Jedinstveni ID za dealokaciju

        send_request(sock, &req);  // Slanje zahteva serveru
    }

    // Zatvaranje soketa
    closesocket(sock);
    return NULL;
}

int main() {
    WSADATA wsa;
    pthread_t users[5];  // 5 korisnika

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspesna inicijalizacija Winsock-a. Greska: %d\n", WSAGetLastError());
        return 1;
    }

    // Kreiranje i pokretanje niti (korisnika)
    for (int i = 0; i < 5; ++i) {
        if (pthread_create(&users[i], NULL, simulate_user, NULL) != 0) {
            printf("Greska prilikom kreiranja niti za korisnika %d\n", i + 1);
            WSACleanup();
            return 1;
        }
    }

    // Cekanje da sve niti završe
    for (int i = 0; i < 5; ++i) {
        pthread_join(users[i], NULL);
    }

    // Ciscenjes Winsock-a
    WSACleanup();

    return 0;
}
