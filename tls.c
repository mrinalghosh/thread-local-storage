#include "tls.h"

#include <signal.h>  // sigaction() and siginfo_t
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // memset()
#include <sys/mman.h>  // mmap() and mprotect()
#include <unistd.h>    // NULL and getpagesize()

/* 
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    addr=NULL for page aligned
    length in bytes
    fd=-1 if using MAP_ANONYMOUS
    flags -
        PROT_NONE when init
        PROT_WRITE when read
    offset can be 0
    flags - MAP_ANONYMOUS, MAP_PRIVATE(?)

int mprotect(void *addr, size_t len, int prot);
    changes protection for calling process's memory pages containing any addresses in range [addr, addr+len-1]
    addr must be aligned to page boundary
    -> if calling process violates mem protections - SIGSEGV generated
    prot= combination of PROT_NONE, PROT_READ, PROT_WRITE (,PROT_EXEC...)

void  <signal_handler>(int signal, siginfo_t *info, void *context);
    takes three params - enabled by using SA_SIGINFO flag
    signal - signum
    info - signal delivery info
    context - thread context info
*/

const int page_size = getpagesize();
bool initialized = false;
int i;                               // for iteration
struct TLS *tls_array[MAX_THREADS];  // can we still assume maxthreads in this assignment?

static void tls_handle_page_fault(int signal, siginfo_t *info, void *context) {
    printf("Inside signal handler");
    uintptr_t page_fault = (uintptr_t)(info->si_addr) & ~(page_size - 1);  // find page address (aligned) of page fault - page_size always power of 2

    // pthread_exit wrapper or pthread_exit once done?
}

static void tls_init(void) {
    /* SIGNAL HANDLING */
    struct sigaction act;
    // memset(&act, 0, sizeof(act)); // don't really need to zero entire struct
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;  // for verbose signal handling (int signal, siginfo_t *info, void *context)
    act.sa_handler = tls_handle_page_fault;
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS, &act, NULL);  // on some machines SIGBUS traps memory - Camden OH

    for (i = 0; i < MAX_THREADS; ++i) {
        // tls_array[i]->id = -1; // do I even need to set this?
        tls_array[i]->id = (pthread_t)i;
        // tls_array[i]->size = 0; // are these even necessary?
        // tls_array[i]->num_pages = 0;
        // tls_array[i]->pages = NULL;
    }

    initialized = true;
}

int tls_create(unsigned int size) {
    /* TODO:
    create LSA for current thread
    return 0 - success | -1 - thread already has LSA w size > 0 bytes
    */
    if (!initialized) {
        // init TLS management struct
        tls_init();
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