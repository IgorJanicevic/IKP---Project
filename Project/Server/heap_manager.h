#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h> // Za void* i size_t

#define NUM_BUCKETS 10        
#define SEGMENT_SIZE 1024     
#define MAX_BLOCK_SIZE 512   
#define MAX_FREE_SEGMENTS 5

// Struktura koja predstavlja blok
typedef struct Block {
    void* address;            // Adresa bloka u segmentu
    size_t size;              // Veličina bloka
    struct Block* next;       // Sledeći blok u listi
} Block;

// Struktura koja predstavlja segment
typedef struct Segment {
    void* base_address;       // Adresa početka segmenta
    size_t used_size;         // Količina iskorišćenog prostora u segmentu
    Block* blocks;            // Lista blokova unutar segmenta
    struct Segment* next;     // Sledeći segment u listi
} Segment;

// Globalna hashmapa sa segmentima
extern Segment* segment_map[NUM_BUCKETS];  // Deklaracija, ne inicijalizacija



void* cleanup_segments(void* arg);
unsigned int hash(void* address);
void cleanup_free_segments();
unsigned int hash_address(void* address);



void print_memory_status();
void free_block(void* address);
void* allocate_block(size_t size);

// void cleanup_heap_manager();


#endif // HEAP_MANAGER_H
