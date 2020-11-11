#ifndef TLS_H
#define TLS_H

#include <pthread.h>

#define MAX_THREADS 128

typedef struct page_frame {
    uintptr_t address;
    unsigned ref_count;  //TODO: signed or unsigned
} page_frame;

typedef struct TLS {
    pthread_t tid;
    unsigned int size;       // in bytes
    unsigned int num_pages;  // number of pages
    struct page_frame **pages;
} TLS;

int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy(void);  // void instead of empty arglist
int tls_clone(pthread_t tid);

#endif /* TLS_H */