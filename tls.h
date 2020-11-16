#ifndef TLS_H
#define TLS_H

#include <inttypes.h>
#include <pthread.h>

#define HASH_SIZE 32
#define MAX_THREADS 128

typedef struct page {
    // unsigned int address; /* start address of page */
    uintptr_t address;
    int ref_count; /* counter for shared pages */
} page;

typedef struct TLS {
    pthread_t tid;
    unsigned int size;     /* size in bytes */
    unsigned int page_num; /* number of pages */
    page **pages;          /* array of pointers to pages */
} TLS;

typedef struct hash_node {
    pthread_t tid;
    TLS *tls;
    struct hash_node *next;
} hash_node;

int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy(void);  // void instead of empty arglist
int tls_clone(pthread_t tid);

#endif /* TLS_H */