#ifndef TLS_H
#define TLS_H

#include <inttypes.h>  // uintptr_t
#include <pthread.h>

#define HASH_SIZE 32
#define error(message)      \
    {                       \
        perror(message);    \
        exit(EXIT_FAILURE); \
    }

typedef struct page {
    uintptr_t address; /* uintptr_t supports bitwise ops and is portable */
    int ref_count;     /* counter for shared pages */
} page;

typedef struct TLS {
    pthread_t tid;
    unsigned int size;     /* size in bytes */
    unsigned int page_num; /* number of pages */
    page **pages;          /* array of pointers to pages */
} TLS;

typedef struct hash_element {
    pthread_t tid;
    TLS *tls;
    struct hash_element *next;
} hash_element;

int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy(void);  // void instead of empty arglist
int tls_clone(pthread_t tid);

#endif /* TLS_H */