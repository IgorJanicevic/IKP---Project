#include "heap_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>


int freed_segments_count = 0;  // Broj oslovodjenih segmenata
pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER; // Za zakljucavanje pristupa globalnim resursima
pthread_cond_t cleanup_cond = PTHREAD_COND_INITIALIZER;  // Signalizacija za pokretanje ciscenja
static int segment_size;       // Velicina jednog segmenta
static int num_segments;       // Broj alociranih segmenata


Segment* segment_map[NUM_BUCKETS] = { NULL };  // Inicijalizacija segment_map-a


//Ovo je da se generise za bucket ovo ne diraj
unsigned int hash(void* address) {
    return ((uintptr_t)address) % NUM_BUCKETS; 
}

//Ovo da se implementira i kada se kreira novi blok da dobije ovu vrednost
unsigned int hash_address(void* address){
    return -1;
}

void* allocate_block(size_t size) {

    if(size>SEGMENT_SIZE){
        printf("Blok koji korisnik zeli da alocira je preveliki blok!");
        return NULL;
    }

    Block* new_block = (Block*)malloc(sizeof(Block));

    unsigned int bucket = hash(new_block);
    Segment* segment = segment_map[bucket];

    // Pronadji segment sa dovoljno slobodnog prostora
    while (segment != NULL) {
        if (segment->used_size + size <= SEGMENT_SIZE) {
            // Ima dovoljno prostora u ovom segmentu, alociraj blok
            new_block->address = new_block;
            new_block->size = size;
            new_block->next = segment->blocks;
            segment->blocks = new_block;

            segment->used_size += size;  // Azuriraj iskoriscen prostor u segmentu
            return new_block->address;
        }
        segment = segment->next;
    }

    // Ako nema dovoljno prostora u postojecim segmentima, kreiraj novi segment
    Segment* new_segment = (Segment*)malloc(sizeof(Segment));
    new_segment->base_address = malloc(SEGMENT_SIZE);
    new_segment->used_size = size;
    new_segment->blocks = new_block;
    new_segment->blocks->address = new_segment->blocks;
    new_segment->blocks->size = size;
    new_segment->blocks->next = NULL;
    new_segment->next = segment_map[bucket];

    segment_map[bucket] = new_segment;  // Dodaj segment u hashmapu

    return new_segment->blocks->address;
}


void free_block(void* address) {
    unsigned int bucket = hash(address);
    Segment* segment = segment_map[bucket];

    while (segment != NULL) {
        Block* prev = NULL;
        Block* current = segment->blocks;

        // Trazenje bloka koji treba da se oslobodi
        while (current != NULL) {
            if (current->address == address) {
                segment->used_size -= current->size;
                
                // Ako se pronadje blok, ukloni ga iz liste blokova
                if (prev != NULL) {
                    prev->next = current->next;
                } else {
                    segment->blocks = current->next;
                }
                free(current); // Oslobadjanje memorije za blok


                // Ako je segment sada prazan, povecati broja slobodnih segmenata
                if (segment->used_size == 0) {
                    printf("Segment postao prazan!\n");
                    freed_segments_count++;
                }

                if(freed_segments_count>5){
                    pthread_cond_signal(&cleanup_cond);
                }
                return;
            }
            prev = current;
            current = current->next;
        }
        segment = segment->next;
    }

    printf("Blok nije pronadjen!\n");
}



void print_memory_status() {
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        Segment* segment = segment_map[i];
        while (segment != NULL) {
            printf("Segment @ %p | Korisni prostor: %zu / %zu\n", segment->base_address, segment->used_size, SEGMENT_SIZE);
            Block* block = segment->blocks;
            while (block != NULL) {
                printf("  Blok @ %p | Velicina: %zu\n", block->address, block->size);
                block = block->next;
            }
            segment = segment->next;
        }
    }
}


void* cleanup_segments(void* arg) {
    while (1) {
        pthread_mutex_lock(&heap_lock);

        while (freed_segments_count < MAX_FREE_SEGMENTS) {
            pthread_cond_wait(&cleanup_cond, &heap_lock); 
        }

        cleanup_free_segments();  

        freed_segments_count = 0; 

        pthread_mutex_unlock(&heap_lock);
    }
    return NULL;
}



void cleanup_free_segments() {
    printf("\nPokrenuto je ciscenje memorije..\n\n");
    // Prolazak kroz sve segmente
    for (int i = 0; i < NUM_BUCKETS; ++i) {
        Segment* prev_segment = NULL;
        Segment* current_segment = segment_map[i];

        while (current_segment != NULL) {
            if (current_segment->used_size == 0) {

                // Uklanjanje praznog segmenta
                if (prev_segment == NULL) {
                    segment_map[i] = current_segment->next;
                } else {
                    prev_segment->next = current_segment->next;
                }

                free(current_segment->base_address);
                free(current_segment);
                freed_segments_count++;
            } else {
                prev_segment = current_segment;
            }
            current_segment = current_segment->next;
        }
    }

}





