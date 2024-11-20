#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h> 

// Struktura koja predstavlja memorijski blok
typedef struct Block {
    size_t size;          // Velicina bloka
    int is_allocated;     // Status (1 - alociran, 0 - slobodan)
} Block;

// Struktura koja predstavlja hashmapu za cuvanje blokova
typedef struct HashMap {
    Block* blocks[1000];  // Prepoznavanje blokova sa kljucem
} HashMap;

// Funkcija za kreiranje hash mape
HashMap* create_hashmap();

// Funkcija za kreiranje bloka memorije
Block* create_block(size_t size);

// Funkcija za dodavanje bloka u hashmapu
void add_block(HashMap* hashmap, int key, Block* block);

// Funkcija za alokaciju memorije
Block* allocate_memory(HashMap* hashmap, size_t size);

// Funkcija za oslobadjanje memorije
void free_memory(HashMap* hashmap, int key);

#endif // HEAP_MANAGER_H

