#include "server.h"

static char message[1024]; // Pretpostavljena veličina niza za poruku
void* address_alocated_block = NULL;

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
        address_alocated_block = allocate_block(req.size);
        if (address_alocated_block != NULL) {
            printf("Memorija alocirana: %zu bajtova\n", req.size);
            snprintf(message, sizeof(message), "Memorija alocirana: %d bajtova\nMemorija alocirana na adresi: %p\n", req.size, address_alocated_block);
        } else {
            printf("Nema dovoljno slobodne memorije za alokaciju %zu bajtova.\n", req.size);
            snprintf(message, sizeof(message), "Nema dovoljno slobodne memorije za alokaciju %d bajtova.\n", req.size);
        }
    } else if (req.type == 2) { // Dealokacija
        if(free_block(req.block_id)){
            snprintf(message, sizeof(message), "Memorija sa ID %p je oslobodjena.\n", req.block_id);
            printf("Memorija sa ID %p je oslobodjena.\n", req.block_id);
        } else {
            snprintf(message, sizeof(message), "Memorija sa ID %p nije pronadjena.\n", req.block_id);
            printf("Memorija sa ID %p nije pronadjena.\n", req.block_id);
        }
    }
    print_memory_status();
}

// Funkcija za slanje poruka klijentima
void send_message(SOCKET sock) {
    if (send(sock, message, 1000, 0) == SOCKET_ERROR) {
        printf("Neuspesno slanje poruke. Greska: %d\n", WSAGetLastError());
        exit(1);
    }
    printf("Poruka poslata klijentu!\n");
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
        Sleep(100);
        send_message(client_socket);
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
        pthread_detach(client_thread); // Automatsko ciscenje niti nakon zavrsetka
    }
    return NULL;
}
