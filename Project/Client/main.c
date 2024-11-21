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
    size_t size;        // Velicina za alokaciju (ako je tip 1)
    void* block_id;       // ID bloka za dealokaciju (ako je tip 2)
} Request;

// Funkcija za slanje zahteva serveru
void send_request(SOCKET sock, Request* req) {
    if (send(sock, (char*)req, sizeof(Request), 0) == SOCKET_ERROR) {
        printf("Neuspesno slanje zahteva. Greska: %d\n", WSAGetLastError());
        exit(1);
    }
    printf("Zahtev poslat serveru.\n");
}

void menu(SOCKET socket){
    do{
    printf("\n\n****************************\n");
    printf("1. Alociraj memoriju\n");
    printf("2. Delociraj memoriju\n");
    printf("0. Izlaz\n");
    int action;
    scanf("%d",&action);
    Request req;
    switch (action)
    {
    case 1:
        printf("Unesi broj bajtova\n");
        req.type=1;
        int size;
        scanf("%d",&size);
        req.size=size;
        send_request(socket,&req);
        break;
    case 2:
        printf("Unesi pocetnu adresu\n");
        req.type=2;
        scanf("%p", &req.block_id); 
        send_request(socket,&req);
        break;
    case 0:
        printf("Izasli ste\n");
        return;
    default:
        printf("Nevalidna komanda\n");
        break;
    }}while(1);
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;

    // Inicijalizacija Winsock-a
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Neuspesna inicijalizacija Winsock-a. Greska: %d\n", WSAGetLastError());
        return 1;
    }

    // Kreiranje soketa
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Neuspesno kreiranje soketa. Greska: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Povezivanje na server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Neuspesno povezivanje sa serverom. Greska: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Uspesno ste se povezali");
    menu(sock);

    // Zatvaranje soketa
    closesocket(sock);
    WSACleanup();

    return 0;
}
