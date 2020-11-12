#ifndef TLS_H
#define TLS_H

#include <pthread.h>

// #define HASH_SIZE 128
#define MAX_THREADS 128

typedef struct page {
    unsigned int address; /* start address of page */
    int ref_count;        /* counter for shared pages */
    // struct page *next;    // POINTER TO NEXT PAGE
} page;

typedef struct TLS {
    pthread_t tid;
    unsigned int size;     /* size in bytes */
    unsigned int page_num; /* number of pages */
    page **pages;   /* array of pointers to pages */
    // struct page *pages;  // head of linked list of pages
} TLS;

// typedef struct hash_element {
//     pthread_t tid;
//     TLS *tls;
//     struct hash_element *next;
// } hash_element;

int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy(void);  // void instead of empty arglist
int tls_clone(pthread_t tid);

#endif /* TLS_H */