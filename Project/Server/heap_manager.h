#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h> // Za void* i size_t

#define SEGMENT_SIZE 1024
#define HASHMAP_BUCKETS 10
#define MAX_FREE_SEGMENTS 5


// Struktura za segment
typedef struct Segment {
    int is_free;              // Da li je segment slobodan (1 = slobodan, 0 = zauzet)
    size_t size;              // Velicina segmenta
    void* address;            // Memorijska adresa
    struct Segment* next;     // Sledeci segment u lancu
} Segment;

// Hes mapa za segmente
typedef struct HashMap {
    int capacity;             // Kapacitet (broj kanti u hes mapi)
    Segment** buckets;        // Niz pokazivaca na segmente
} HashMap;


static HashMap* segment_map;   // Globalna hes mapa

// Funkcija koja pornalazi slobodne segmente i brise ih
void cleanup_free_segments();

// Funkcija koja osluskuje za cisecenje memorije
void* cleanup_segments(void* args);

// Inicijalizacija Heap Manager-a
void init_heap_manager();

// Alokacija memorijskog bloka
void* allocate_memory(int size);

// Oslobadjanje memorijskog bloka
void free_memory(void* address);

// Pracenje performansi
void print_memory_status();

// Oslobadjanje svih resursa
void cleanup_heap_manager();

#endif // HEAP_MANAGER_H
