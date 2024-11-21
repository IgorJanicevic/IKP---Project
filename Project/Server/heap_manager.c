#include "heap_manager.h"
#include <stdlib.h>
#include <stdio.h>

// Funkcija za kreiranje hash mape
HashMap* create_hashmap() {
    HashMap* hashmap = (HashMap*)malloc(sizeof(HashMap));
    for (int i = 0; i < HASHMAP_SIZE; i++) {
        hashmap->blocks[i] = NULL;
    }
    return hashmap;
}

// Funkcija za kreiranje bloka memorije
Block* create_block(int size) {
    Block* block = (Block*)malloc(sizeof(Block) + calculate_segments(size));
    block->size = size;
    block->is_allocated = 0; // Blok je slobodan
    block->next = NULL;
    return block;
}

// Funkcija za dodavanje bloka u hashmapu
void add_block(HashMap* hashmap, Block* block) {
    hashmap->blocks[hash_key(block->size)] = block;
}

int calculate_segments(int size){
    return (size + SEGMENT_SIZE-1) / SEGMENT_SIZE;
}

int hash_key(int size){
    return size % HASHMAP_SIZE;
}

// Funkcija za alokaciju memorije
Block* allocate_memory(HashMap* hashmap, int size) {
    for (int i = 0; i < HASHMAP_SIZE; i++) {
        if (!hashmap->blocks[i]->is_allocated && hashmap->blocks[i]->size >= size) { //hashmap->blocks[i] != NULL
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
