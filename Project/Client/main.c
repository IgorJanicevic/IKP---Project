#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Linkovanje Winsock biblioteke

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Struktura za zahtev
typedef struct Request {
    int type;           // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;        // Velièina za alokaciju (ako je tip 1)
    int block_id;       // ID bloka za dealokaciju (ako je tip 2)
} Request;

// Funkcija za slanje zahteva serveru
void send_request(SOCKET sock, Request* req) {
    if (send(sock, (char*)req, sizeof(Request), 0) == SOCKET_ERROR) {
        printf("Neuspešno slanje zahteva. Greška: %d\n", WSAGetLastError());
        exit(1);
    }
    printf("Zahtev poslat serveru.\n");
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspešna inicijalizacija Winsock-a. Greška: %d\n", WSAGetLastError());
        return 1;
    }

    // Kreiranje soketa
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Neuspešno kreiranje soketa. Greška: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Povezivanje na server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Neuspešno povezivanje sa serverom. Greška: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Kreiranje i slanje zahteva za alokaciju memorije
    Request req;
    req.type = 1; // Alokacija
    req.size = 100; // Alociraj 100 bajtova
    send_request(sock, &req);

    // Kreiranje i slanje zahteva za dealokaciju memorije
    req.type = 2; // Dealokacija
    req.block_id = 0; // ID bloka za dealokaciju
    send_request(sock, &req);

    // Zatvaranje soketa
    closesocket(sock);
    WSACleanup();

    return 0;
}
