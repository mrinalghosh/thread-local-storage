#ifndef TLS_H
#define TLS_H

#include <pthread.h>

typedef struct Page {
    uintptr_t address;
    unsigned ref_count;  //TODO: signed or unsigned - can this be negative?
} Page;

typedef struct TLS {
    pthread_t tid;
    unsigned size;
    unsigned num_pages;
    Page **pages;
} TLS;

int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy(void);  // void instead of empty arglist
int tls_clone(pthread_t tid);

#endif /* TLS_H */