#ifndef TEST_H
#define TEST_H

#include "client.h"
#include <windows.h>
#include <pthread.h>


#define NUM_CLIENTS 6
#define MAX_BLOCKS 100

// Globalna lista alociranih blokova
extern void* allocated_blocks[MAX_BLOCKS];
extern int block_count;
extern pthread_mutex_t mutex;

void* client_thread_function(void* arg);
void test_allocation(SOCKET sock, int client_id, int num_blocks);
void test_deallocation(SOCKET sock, int client_id);
void test_random_operations(SOCKET sock, int client_id);
void extract_block_address(const char* message);

#endif // TEST_H
