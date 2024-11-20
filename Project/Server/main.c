#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib") // Linkovanje Winsock biblioteke

#define SERVER_PORT 8080
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 5

// Struktura za zahtev
typedef struct Request {
    int type;           // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;           // Velicina za alokaciju (ako je tip 1)
    int block_id;       // ID bloka za dealokaciju (ako je tip 2)
} Request;

// Red cekanja za zahteve
Request request_queue[MAX_CLIENTS];
int queue_front = 0;
int queue_rear = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Funkcija za dodavanje zahteva u red
void enqueue(Request req) {
    pthread_mutex_lock(&queue_mutex);
    request_queue[queue_rear] = req;
    queue_rear = (queue_rear + 1) % MAX_CLIENTS;
    pthread_cond_signal(&queue_cond); // Obavestenje da je novi zahtev dodat
    pthread_mutex_unlock(&queue_mutex);
}

// Funkcija za uzimanje zahteva iz reda
Request dequeue() {
    pthread_mutex_lock(&queue_mutex);
    while (queue_front == queue_rear) {
        pthread_cond_wait(&queue_cond, &queue_mutex); // Cekanje na novi zahtev
    }
    Request req = request_queue[queue_front];
    queue_front = (queue_front + 1) % MAX_CLIENTS;
    pthread_mutex_unlock(&queue_mutex);
    return req;
}

// Funkcija za obradu zahteva
void process_request(Request req) {
    if (req.type == 1) { // Alokacija
        printf("Obrada zahteva za alokaciju memorije: %zu bajtova\n", req.size);
        // Dodajte logiku za alokaciju memorije ovde
    } else if (req.type == 2) { // Dealokacija
        printf("Obrada zahteva za dealokaciju memorije: ID bloka %d\n", req.block_id);
        // Dodajte logiku za dealokaciju memorije ovde
    }
}

// Funkcija za rad niti u thread pool-u
void* thread_pool_worker(void* arg) {
    while (1) {
        Request req = dequeue(); // Uzimanje zahteva iz reda
        process_request(req);   // Obrada zahteva
    }
    return NULL;
}

// Funkcija za obradu pojedinacnog klijenta
void* handle_client(void* client_socket_ptr) {
    SOCKET client_socket = *(SOCKET*)client_socket_ptr;
    free(client_socket_ptr);

    while (1) {
        Request req;
        int bytes_received = recv(client_socket, (char*)&req, sizeof(Request), 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("Greska pri primanju podataka. Greska: %d\n", WSAGetLastError());
            break;
        } else if (bytes_received == 0) {
            printf("Klijent je zatvorio vezu.\n");
            break;
        }

        
        if (bytes_received == sizeof(Request)) {
            printf("Primljen zahtev od klijenta.\n");
            enqueue(req);
        } else {
            printf("Nepotpun zahtev ili greska u primanju.\n");
        }
    }

    closesocket(client_socket);
    return NULL;
}

// Funkcija za slusanje i prihvatanje klijenata
void* accept_clients(void* server_fd_ptr) {
    SOCKET server_fd = *(SOCKET*)server_fd_ptr;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    while (1) {
        SOCKET client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Greska pri prihvatanju klijenta. Greska: %d\n", WSAGetLastError());
            continue;
        }

        printf("Klijent povezan.\n");

        // Kreiranje niti za pojedinacnog klijenta
        pthread_t client_thread;
        SOCKET* client_socket_ptr = malloc(sizeof(SOCKET));
        *client_socket_ptr = client_socket;
        pthread_create(&client_thread, NULL, handle_client, client_socket_ptr);
        pthread_detach(client_thread); // Automatsko ciscenja niti nakon zavrsetka
    }
    return NULL;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd;
    struct sockaddr_in server_addr;

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
    if (listen(server_fd, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Neuspesno slusanje. Greska: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server je pokrenut. Cekam klijente...\n");

    // Kreiranje niti za slusanje klijenata
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_clients, &server_fd);

    // Kreiranje niti u thread pool-u
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&thread_pool[i], NULL, thread_pool_worker, NULL);
    }

    // Cekanje da se niti zavrse (nece se desiti, jer server radi neprekidno)
    pthread_join(accept_thread, NULL);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    // Ciscenje resursa
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
