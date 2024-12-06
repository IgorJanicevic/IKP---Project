#include "client.h"

void send_request(SOCKET sock, Request* req) {
    if (send(sock, (char*)req, sizeof(Request), 0) == SOCKET_ERROR) {
        printf("Greska prilikom slanja zahteva serveru. Greska: %d\n", WSAGetLastError());
        exit(1);
    }
    printf("Zahtev poslat serveru.\n");
}

char* receive_message(SOCKET sock) {
    char buffer[1024];
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received == SOCKET_ERROR) {
        printf("Greska prilikom primanja poruke. Greska: %d\n", WSAGetLastError());
        return NULL;
    }
    buffer[bytes_received] = '\0';
    printf("Primljena poruka: %s\n", buffer);

    char* message = (char*)malloc((bytes_received + 1) * sizeof(char));
    if (message == NULL) {
        printf("Greska prilikom alokacije memorije.\n");
        return NULL;
    }

    strcpy(message, buffer);

    return message;
}

int is_valid_hex_address(const char* input) {
    if (strlen(input) != ADDRESS_LENGTH) {
        return 0;
    }
    for (int i = 0; i < ADDRESS_LENGTH; i++) {
        if (!isxdigit(input[i])) {
            return 0;
        }
    }
    return 1;
}
