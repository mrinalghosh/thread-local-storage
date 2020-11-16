#include "tls.h"

#include <signal.h>  // sigaction() and siginfo_t
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // memset()
#include <sys/mman.h>  // mmap() and mprotect()
#include <unistd.h>    // NULL and getpagesize()

bool initialized = false;
int page_size;

// struct TLS *tls_table[MAX_THREADS];
struct hash_element *hash_table[HASH_SIZE];

static void tls_handle_page_fault(int sig, siginfo_t *si, void *context) {
    uintptr_t p_fault = ((uintptr_t)si->si_addr) & ~(page_size - 1);  // page aligned address of segfault

    size_t i, j;
    for (i = 0; i < HASH_SIZE; ++i) {  // over hash table entries
        hash_element *node = hash_table[i];
        while (node != NULL) {                           // over LL in an entry
            for (j = 0; j < node->tls->page_num; ++j) {  // over pages
                if (node->tls->pages[j]->address == p_fault) {
                    pthread_exit(NULL);
                }
            }
            node = node->next;
        }
    }

    /* normal fault - install default handler and re-raise signal */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    raise(sig);
}

static unsigned hash(pthread_t tid) { return (unsigned)pthread_self() % HASH_SIZE; }

static void hash_init(void) {  // call when pthread_create first called
    size_t i;                  // iteration
    for (i = 0; i < HASH_SIZE; ++i) {
        hash_table[i] = NULL;
    }
}

static bool hash_exists(pthread_t tid) {
    unsigned idx = hash(tid);
    hash_element *element = hash_table[idx];

    while (element != NULL) {
        if (element->tid == tid) {
            return true;
        }
        element = element->next;
    }

    return false;
}

static TLS *hash_fetch(pthread_t tid) {
    unsigned idx = hash(tid);
    hash_element *element = hash_table[idx];

    while (element != NULL) {
        if (element->tid == tid) {
            return element->tls;
        }
        element = element->next;
    }

    return NULL; /* should query hash_exists first */
}

static void hash_insert(pthread_t tid, TLS *tls) {
    unsigned idx = hash(tid);  // LL to be appended to
    hash_element *element = (hash_element *)calloc(1, sizeof(hash_element));
    hash_element *temp = hash_table[idx];  // iter on hash table cell

    element->tid = tid;
    element->tls = tls;
    element->next = NULL;  // insert at end of list

    if (temp == NULL) {
        hash_table[idx] = element;
    } else {
        while (temp->next != NULL) {  // find first free position
            temp = temp->next;
        }
        temp->next = element;
    }
}

static void hash_destroy(pthread_t tid) {
    unsigned idx = hash(tid);
    hash_element *target = hash_table[idx];
    hash_element *ante;  // antedecent in LL to tid to delete
    size_t i;            // iteration

    if (target->tid == tid) {                             // DONE - relink LLs
        for (i = 0; i < target->tls->page_num; ++i) {     // remove pages
            if (target->tls->pages[i]->ref_count == 1) {  // free pages if no other page has a copy
                if (munmap((void *)target->tls->pages[i]->address, page_size) == -1) {
                    error("munmap");
                }
            }
            --target->tls->pages[i]->ref_count;  // doesn't matter if we decrement a freeing page
        }
        free(target->tls->pages);
        free(target->tls);
        hash_table[idx] = target->next;
        free(target);
    } else {
        while (target != NULL) {
            if (target->tid == tid) {                             // if thread to delete
                for (i = 0; i < target->tls->page_num; ++i) {     // remove pages
                    if (target->tls->pages[i]->ref_count == 1) {  // free pages if no other page has a copy
                        if (munmap((void *)target->tls->pages[i]->address, page_size) == -1) {
                            error("munmap");
                        }
                        free(target->tls->pages[i]);
                    }
                    --target->tls->pages[i]->ref_count; /* doesn't matter if we decrement a freeing page */
                }
                break;  // ante and target set
            }
            ante = target;
            target = target->next;
        }
        ante->next = target->next; /* remove target from LL and relink */
        free(target->tls);
        free(target);
    }
}

/* HELPER FUNCTIONS GIVEN */

static void tls_init(void) {
    page_size = getpagesize(); /* get the size of a page */

    /* install the signal handler for page faults (SIGSEGV, SIGBUS) */
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO; /* use extended signal handling */
    sigact.sa_sigaction = tls_handle_page_fault;
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);

    initialized = true;

    hash_init();
}

static void tls_protect(struct page *p) {
    if (mprotect((void *)p->address, page_size, 0)) {
        error("tls_protect: could not protect page");
    }
}

static void tls_unprotect(struct page *p) {
    if (mprotect((void *)p->address, page_size, PROT_READ | PROT_WRITE)) {
        error("tls_unprotect: could not unprotect page");
    }
}

int tls_create(unsigned int size) {
    if (!initialized) {
        tls_init();
    }

    if (size == 0 || hash_exists(pthread_self())) /* current already has an LSA (ie TLS not NULL) and size is > 0 */
        return -1;

    TLS *tls = (TLS *)calloc(1, sizeof(TLS));  // allocate new TLS
    tls->tid = pthread_self();
    tls->size = size;
    tls->page_num = 1 + (size - 1) / page_size;
    tls->pages = (page **)calloc(tls->page_num, sizeof(page *)); /* allocate all page pointer arrays - indexing bounded by page_num */

    size_t i;                             // iteration
    for (i = 0; i < tls->page_num; ++i) { /* Given */
        struct page *p = (page *)calloc(1, sizeof(page));
        p->address = (uintptr_t)mmap(0, page_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        p->ref_count = 1;
        tls->pages[i] = p;
    }

    hash_insert(tls->tid, tls);

    return 0;
}

int tls_write(unsigned int offset, unsigned int length, char *buffer) {
    if (!hash_exists(pthread_self()) || offset + length > hash_fetch(pthread_self())->size) /* current  already has an LSA and size is > 0 */
        return -1;

    TLS *tls = hash_fetch(pthread_self());

    size_t i;
    for (i = 0; i < tls->page_num; ++i) { /* unprotect for memory accesses */
        tls_unprotect(tls->pages[i]);
    }

    int cnt, idx;
    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) { /* CoW (Copy-on-Write semantics) */
        struct page *p, *copy;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = tls->pages[pn];
        if (p->ref_count > 1) {
            /* this page is shared, create a private copy (COW) */
            copy = (struct page *)calloc(1, sizeof(struct page));
            copy->address = (uintptr_t)mmap(0, page_size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
            copy->ref_count = 1;
            memcpy((void *)copy->address, (void *)p->address, page_size); /* make private copy - args are dst then src */
            tls->pages[pn] = copy;
            /* update original page */
            p->ref_count--;
            tls_protect(p);
            p = copy;
        }
        char *dst = ((char *)p->address) + poff;
        *dst = buffer[cnt];
    }

    for (i = 0; i < tls->page_num; ++i) {  // DOES THIS NEED TO BE ATOMIC?
        tls_protect(tls->pages[i]);
    }

    return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer) {
    if (!hash_exists(pthread_self()) || offset + length > hash_fetch(pthread_self())->size) /*  if LSA does not exist OR asked to read past end of LSA */
        return -1;

    TLS *tls = hash_fetch(pthread_self());

    size_t i;  // iteration
    for (i = 0; i < tls->page_num; ++i) {
        tls_unprotect(tls->pages[i]);
    }

    int cnt, idx;
    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
        struct page *p;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = tls->pages[pn];
        char *src = ((char *)p->address) + poff;
        buffer[cnt] = *src;
    }

    for (i = 0; i < tls->page_num; ++i) {
        tls_protect(tls->pages[i]);
    }

    return 0;
}

int tls_destroy(void) {
    if (!hash_exists(pthread_self()))
        return -1;

    hash_destroy(pthread_self());

    return 0;
}

int tls_clone(pthread_t tid) {
    if (!hash_exists(tid) || hash_exists(pthread_self()))  // check that current DOES NOT have LSA and target does - whoops :7
        return -1;

    TLS *self = (TLS *)calloc(1, sizeof(TLS));
    TLS *target = hash_fetch(tid);
    self->tid = pthread_self();
    self->size = target->size;
    self->page_num = target->page_num;
    self->pages = (page **)calloc(self->page_num, sizeof(page *));
    hash_insert(self->tid, self);

    size_t i;                               // iteration
    for (i = 0; i < self->page_num; ++i) {  // copy pages (not just content), adjust refs, ... (see slide 36)
        self->pages[i] = target->pages[i];
        ++target->pages[i]->ref_count;
    }

    return 0;
}