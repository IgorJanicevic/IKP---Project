#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include "heap_manager.h"

#pragma comment(lib, "ws2_32.lib") // Linkovanje Winsock biblioteke

#define SERVER_PORT 8080
#define MAX_CLIENTS 10
#define THREAD_POOL_SIZE 5

// Struktura za zahtev
typedef struct Request {
    int type;       // Tip zahteva: 1 - alokacija, 2 - dealokacija
    size_t size;    // Velicina za alokaciju (ako je tip 1)
    void* block_id; // Adresa bloka za dealokaciju (ako je tip 2)
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

static char message[1024]; // Pretpostavljena veličina niza za poruku

// Funkcija za obradu zahteva
void process_request(Request req) {
    if (req.type == 1) { // Alokacija
        Segment* allocated_block = allocate_memory(req.size);
        if (allocated_block != NULL) {
            snprintf(message, sizeof(message), "Memorija alocirana: %d bajtova\nMemorija alocirana na adresi: %p\n", req.size, allocated_block->address);
            printf("%s", message);
            print_memory_status();
        } else {
            snprintf(message, sizeof(message), "Nema dovoljno slobodne memorije za alokaciju %d bajtova.\n", req.size);
            printf("%s", message);
        }
    } else if (req.type == 2) { // Dealokacija
       free_memory(req.block_id);
        printf("Memorija sa ID %d je oslobodjena.\n", req.block_id);
        print_memory_status();
        
    }
}

void send_message(SOCKET sock, const char* message) {
    // Dobijamo dužinu poruke
    size_t message_length = strlen(message);
    printf("Ovo je poruka koja se salje: ");
    printf("%s", message);
    printf("----------------------------------------------------------------\n");
    // Šaljemo poruku
    if (send(sock, message, 1000, 0) == SOCKET_ERROR) {
        printf("Neuspesno slanje poruke. Greska: %d\n", WSAGetLastError());
        exit(1);
    }

    printf("Poruka poslata klijentu: %s\n--------------\n", message);
}


// Funkcija za rad niti u thread pool-u
void* thread_pool_worker(void* arg) {
    while (1) {
        Request req = dequeue();
        process_request(req);
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
            int error_code = WSAGetLastError();
            printf("Greska pri primanju podataka. Greska: %d\n", error_code);
            break;
        } else if (bytes_received == 0) {
            printf("Klijent je zatvorio vezu.\n");
            break;
        }

        if (bytes_received == sizeof(Request)) {
            printf("Primljen zahtev od klijenta.\n");
            enqueue(req);
        } else if (bytes_received > 0) {
            printf("Nepotpun zahtev: primljeno %d bajtova od %zu potrebnih.\n", bytes_received, sizeof(Request));
        } else {
            printf("Nepoznata greska u primanju.\n");
        }
        send_message(client_socket, message);
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
    // Kreiranje hash mape za upravljanje memorijom
    
    init_heap_manager();

    snprintf(message, sizeof(message), "INICIJALIZVOANA\n");

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

    // Kreiranje niti za ciscenje memorije
    pthread_t cleanup_thread;
    pthread_create(&cleanup_thread, NULL, cleanup_segments, NULL);

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


    // cleanup_heap_manager();

    for (int i = 0; i < HASHMAP_BUCKETS; i++) {
    if (segment_map->buckets[i] != NULL) {
        free(segment_map->buckets[i]);
    }
    }
    free(segment_map);


    return 0;
}
