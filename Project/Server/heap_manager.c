#include "heap_manager.h"
#include <stdlib.h>
#include <stdio.h>

// Funkcija za kreiranje hash mape
HashMap* create_hashmap() {
    HashMap* hashmap = (HashMap*)malloc(sizeof(HashMap));
    for (int i = 0; i < 1000; i++) {
        hashmap->blocks[i] = NULL;
    }
    return hashmap;
}

// Funkcija za kreiranje bloka memorije
Block* create_block(size_t size) {
    Block* block = (Block*)malloc(sizeof(Block));
    block->size = size;
    block->is_allocated = 0; // Blok je slobodan
    return block;
}

// Funkcija za dodavanje bloka u hashmapu
void add_block(HashMap* hashmap, int key, Block* block) {
    hashmap->blocks[key] = block;
}

// Funkcija za alokaciju memorije
Block* allocate_memory(HashMap* hashmap, size_t size) {
    for (int i = 0; i < 1000; i++) {
        if (hashmap->blocks[i] != NULL && !hashmap->blocks[i]->is_allocated && hashmap->blocks[i]->size >= size) {
            hashmap->blocks[i]->is_allocated = 1;
            return hashmap->blocks[i];
        }
    }
    return NULL; // Ako nije pronadjen odgovarajuci blok
}

// Funkcija za oslobadjanje memorije
void free_memory(HashMap* hashmap, int key) {
    if (hashmap->blocks[key] != NULL) {
        hashmap->blocks[key]->is_allocated = 0;
    }
}
