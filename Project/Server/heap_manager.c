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



// Funkcija hesiranje
static int hash_function(void* address) {
    return ((uintptr_t)address) % segment_map->capacity;
}

// Inicijalizacija hes mape
static void init_hash_map(HashMap* map, int capacity) {
    map->capacity = capacity;
    map->buckets = (Segment**)calloc(capacity, sizeof(Segment*));
}

// Dodavanje segmenta u hes mapu
static void add_segment_to_map(HashMap* map, Segment* segment) {
    int index = hash_function(segment->address);
    segment->next = map->buckets[index];
    map->buckets[index] = segment;
}

// Pronalazenje segmenta u hes mapi
static Segment* find_segment_in_map(HashMap* map, void* address) {
    int index = hash_function(address);
    Segment* current = map->buckets[index];
    while (current != NULL) {
        if (current->address == address) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Uklanjanje segmenta iz hes mape
static void remove_segment_from_map(HashMap* map, void* address) {
    int index = hash_function(address);
    Segment* current = map->buckets[index];
    Segment* prev = NULL;

    while (current != NULL) {
        if (current->address == address) {
            if (prev == NULL) {
                map->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current->address);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Inicijalizacija Heap Manager-a
void init_heap_manager() {
    segment_size = SEGMENT_SIZE;
    num_segments = 0;
    segment_map = (HashMap*)malloc(sizeof(HashMap));
    init_hash_map(segment_map, HASHMAP_BUCKETS);
    pthread_mutex_init(&heap_lock, NULL);
}


void* allocate_memory(int size) {
    pthread_mutex_lock(&heap_lock);

    int segments_needed = (size + segment_size - 1) / segment_size;
    Segment* best_fit = NULL;

    // Traženje slobodnih segmenata
    for (int i = 0; i < segment_map->capacity; ++i) {
        Segment* current = segment_map->buckets[i];
        while (current != NULL) {
            if (current->is_free && current->size >= size) {
                best_fit = current;
                break;
            }
            current = current->next;
        }
        if (best_fit) break;
    }

    // Ako nema slobodnih segmenata, alociraj nove
    if (!best_fit) {
        for (int i = 0; i < segments_needed; ++i) {
            Segment* new_segment = (Segment*)malloc(sizeof(Segment));
            if (new_segment == NULL) {
                printf("Greska: Neuspesna alokacija za novi segment!\n");
                pthread_mutex_unlock(&heap_lock);
                return NULL;
            }
            new_segment->is_free = 0;
            new_segment->size = segment_size;
            new_segment->address = malloc(segment_size);
            if (new_segment->address == NULL) {
                printf("Greska: Neuspesna alokacija memorije za segment!\n");
                free(new_segment);
                pthread_mutex_unlock(&heap_lock);
                return NULL;
            }
            add_segment_to_map(segment_map, new_segment);
            ++num_segments;

            if (i == 0) {
                best_fit = new_segment;
            }
        }
    }

    if (best_fit == NULL) {
        printf("Greska: Nije pronadjen ili kreiran odgovarajuci segment!\n");
        pthread_mutex_unlock(&heap_lock);
        return NULL;
    }

    best_fit->is_free = 0;
    pthread_mutex_unlock(&heap_lock);

    return best_fit->address;
}


void free_memory(void* address) {
    pthread_mutex_lock(&heap_lock);

    Segment* segment = find_segment_in_map(segment_map, address);
    if (segment != NULL) {
        segment->is_free = 1;
        freed_segments_count++;

        if (freed_segments_count > MAX_FREE_SEGMENTS) {
            printf("\nPresli smo prag oslobodjenih segmenata. Pokrecemo ciscenje...\n");
            pthread_cond_signal(&cleanup_cond);
        }
    }

    pthread_mutex_unlock(&heap_lock);
}



void* cleanup_segments(void* arg) {
    while (1) {
        pthread_mutex_lock(&heap_lock);

        while (freed_segments_count <= MAX_FREE_SEGMENTS) {
            pthread_cond_wait(&cleanup_cond, &heap_lock); 
        }

        printf("Pokrecemo ciscenje memorije...\n");
        cleanup_free_segments();  

        freed_segments_count = 0; 

        pthread_mutex_unlock(&heap_lock);
    }
    return NULL;
}

void cleanup_free_segments() {
    for (int i = 0; i < segment_map->capacity; ++i) {
        Segment* current = segment_map->buckets[i];
        while (current != NULL) {
            if (current->is_free) {
                remove_segment_from_map(segment_map, current->address);
                --num_segments;  // Smanjivanje broja segmenata                
            }
            current = current->next;
        }
    }
}




// Oslobađanje svih resursa
void cleanup_heap_manager() {
    pthread_mutex_lock(&heap_lock);

    for (int i = 0; i < segment_map->capacity; ++i) {
        Segment* current = segment_map->buckets[i];
        while (current != NULL) {
            Segment* to_free = current;
            current = current->next;
            free(to_free->address);
            free(to_free);
        }
    }

    free(segment_map->buckets);
    free(segment_map);

    pthread_mutex_unlock(&heap_lock);
    pthread_mutex_destroy(&heap_lock);
}


// Ispis statusa memorije
void print_memory_status() {
    pthread_mutex_lock(&heap_lock);

    printf("Total segments: %d\n", num_segments);
    for (int i = 0; i < segment_map->capacity; ++i) {
        Segment* current = segment_map->buckets[i];
        while (current != NULL) {
            printf("Segment @ %p | Size: %zu | Free: %d\n",
                   current->address, current->size, current->is_free);
            current = current->next;
        }
    }
    pthread_mutex_unlock(&heap_lock);
}