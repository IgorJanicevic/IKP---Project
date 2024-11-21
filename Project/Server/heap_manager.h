#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h> 

#define SEGMENT_SIZE 1000
#define HASHMAP_SIZE 10

// Struktura koja predstavlja memorijski blok
typedef struct Block {
    int size;          // Velicina bloka
    int is_allocated;     // Status (1 - alociran, 0 - slobodan)
    Block* next;
} Block;

// Struktura koja predstavlja hashmapu za cuvanje blokova
typedef struct HashMap {
    Block* blocks[HASHMAP_SIZE];  // Prepoznavanje blokova sa kljucem
} HashMap;

// Funkcija za kreiranje hash mape
HashMap* create_hashmap();

// Funkcija za kreiranje bloka memorije
Block* create_block(int size);

// Funkcija za dodavanje bloka u hashmapu
void add_block(HashMap* hashmap, Block* block);

// Funkcija za alokaciju memorije
Block* allocate_memory(HashMap* hashmap, int size);

// Funkcija za oslobadjanje memorije
void free_memory(HashMap* hashmap, int key);

#endif // HEAP_MANAGER_H

