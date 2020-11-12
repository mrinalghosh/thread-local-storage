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

bool initialized = false;
int page_size;
int i, cnt;  // for iteration

struct TLS *tls_table[MAX_THREADS];

/* HELPER FUNCTIONS GIVEN */

static void tls_init(void) {
    /* get the size of a page */
    page_size = getpagesize();
    /* install the signal handler for page faults (SIGSEGV, SIGBUS) */
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO; /* use extended signal handling */
    sigact.sa_sigaction = tls_handle_page_fault;
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);

    for (i = 0; i < MAX_THREADS; ++i) {
        tls_table[i] = NULL;  // will calloc each TLS when it is first created
    }

    initialized = true;
}

static void tls_protect(struct page *p) {
    if (mprotect((void *)p->address, page_size, 0)) {
        fprintf(stderr, "tls_protect: could not protect page\n");
        exit(1);
    }
}

static void tls_unprotect(struct page *p) {
    if (mprotect((void *)p->address, page_size,
                 PROT_READ | PROT_WRITE)) {
        fprintf(stderr, "tls_unprotect: could not unprotect page\n");
        exit(1);
    }
}

static void tls_handle_page_fault(int sig, siginfo_t *si, void *context) {
    unsigned int p_fault = ((unsigned int)si->si_addr) & ~(page_size - 1);
}

// static void insert_pages(TLS *tls, int n) {
//     struct page *temp = calloc(1, sizeof(page));  // allocate first page
//     tls->pages = temp;                            // store this head of LL in tls
//     for (i = 0; i < n; ++i) {
//         // TODO: is this prot_none or 0?
//         temp->address = (unsigned int)mmap(NULL, page_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);  // store address of page
//         temp->ref_count = 1;
//         temp->next = calloc(1, sizeof(page));
//         temp = temp->next;  // should be null;
//     }
// }

int tls_create(unsigned int size) {
    /* TODO:
    create LSA for current thread
    return 0 - success | -1 - thread already has LSA w size > 0 bytes
    */

    if (!initialized) {
        tls_init();
    }

    unsigned int id = (unsigned int)pthread_self();  //current id

    if (tls_table[id] && tls_table[id]->size > 0) {  // current thread already has an LSA (ie TLS not NULL) and size is > 0
        return -1;
    }

    tls_table[id] = calloc(1, sizeof(TLS));  // allocate TLS member
    tls_table[id]->tid = pthread_self();
    tls_table[id]->size = size;
    tls_table[id]->page_num = 1 + (size - 1) / page_size;  // 4096 -> 1 page

    tls_table[id]->pages = calloc(1, sizeof(page*))
    // void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

    return 0;
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