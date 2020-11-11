#include "tls.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // memset()
#include <sys/mman.h>  // mmap() and mprotect()
#include <unistd.h>    // NULL and getpagesize()

unsigned int page_size = getpagesize();
bool initialized = false;

/* 
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    addr=NULL for page aligned
    length in bytes
    fd=-1 if using MAP_ANONYMOUS
    


int mprotect(void *addr, size_t len, int prot);
*/

static void tls_init(void) {
    // set up signal handling for SIGSEGV and SIGBUS
    struct sigaction act;
    memset(&act, )
}

int tls_create(unsigned int size) {
    /* TODO:
    create LSA for current thread
    return 0 - success | -1 - thread already has LSA w size > 0 bytes
    */
    if (!initialized) {
        // init TLS management struct
        initialized = true;
    }
}

int tls_write(unsigned int offset, unsigned int length, char *buffer) {
    /* TODO:
    read /length bytes from mem location pointed to by /buffer and writes to LSA of currently executing thread starting from position /offset
    0 - success | -1 - more data asked to be written than LSA can hold - offset + length > size(LSA) OR if current thread has no LSA
    */
}

int tls_read(unsigned int offset, unsigned int length, char *buffer) {
    /* TODO:
    read /length bytes from LSA starting at position /offset and write to memory location /buffer
    0 - success | -1 - asked to read past end of LSA (offset+length>size(LSA)) or current thread has no LSA
    */
}

int tls_destroy(void) {
    /* TODO:
    free previously allocated storage area of currently executing thread
    0 - success | -1 - thread doesn't have a LSA
    */
}

int tls_clone(pthread_t tid) {
    /* TODO:
    'clones' LSA of target thread identified by /tid. 
    NOT simply copied -> storage areas of both threads INITIALlY refer to same memory location
    -> when one thread tls_writes to its own LSA, TLS library makes private copy of region written (!!!) remaining untouched regions remain shared
    0 - success | -1 - target thread has no LSA OR WHEN currently executing thread already has LSA
    */
}