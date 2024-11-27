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


void* allocate_block(size_t size) {
    if (size > SEGMENT_SIZE) {
        printf("Blok koji korisnik zeli da alocira je preveliki blok!\n");
        return NULL;
    }

    Block* new_block = (Block*)malloc(sizeof(Block));
    unsigned int bucket = hash(new_block);
    Segment* segment = segment_map[bucket];
    Segment* prev_segment = NULL;


    // Pronadji segment sa dovoljno slobodnog prostora
    while (segment != NULL) {
        if (segment->used_size + size <= SEGMENT_SIZE) {
            // Ima dovoljno prostora u ovom segmentu, aloiraj blok
            new_block->address = new_block;
            new_block->size = size;
            new_block->next = segment->blocks;
            segment->blocks = new_block;
            segment->used_size += size; 

            // Prebaci ovaj segment na pocetak bucket-a
            if (prev_segment != NULL) {
                prev_segment->next = segment->next; 
                segment->next = segment_map[bucket]; 
                segment_map[bucket] = segment;
            }

            return new_block->address;
        }
        prev_segment = segment;
        segment = segment->next;
    }

    // Ako nema dovoljno prostora u postojecim segmentima, kreiraj novi segment
    Segment* new_segment = (Segment*)malloc(sizeof(Segment));
    new_segment->base_address = malloc(SEGMENT_SIZE);
    new_segment->used_size = size;
    new_segment->blocks = new_block;
    new_block->address = new_block;
    new_block->size = size;
    new_block->next = NULL;
    new_segment->next = segment_map[bucket];

    segment_map[bucket] = new_segment; // Dodaj novi segment na pocetak bucket-a

    return new_block->address;
}



int free_block(void* address) {
    unsigned int bucket = hash(address);
    Segment* segment = segment_map[bucket];
    pthread_mutex_lock(&heap_lock);

    while (segment != NULL) {
        Block* prev = NULL;
        Block* current = segment->blocks;
        printf(segment->base_address);


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
                pthread_mutex_unlock(&heap_lock);
                return 1;
            }
            prev = current;
            current = current->next;
        }
        segment = segment->next;
    }
    pthread_mutex_unlock(&heap_lock);
    printf("Blok nije pronadjen!\n");
    return 0;
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





