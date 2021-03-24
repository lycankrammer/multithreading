/*
 * NOTE: When compiled with -fsanitize=thread (either gcc or clang), the implementation
 * of C11 threads on my platform seems to suffer with an issue (or there's a subtle bug
 * in this code). It seems that the `arg` to the thread routine happens to be optimized
 * out, causing a null pointer dereference.
 */
#include <stdio.h>
#include <stdlib.h>
#include "rwlock.h"

enum { nthreads = 5u, datasize = 15u, iters = 10000u };

typedef struct {
    unsigned threadnum;
    thrd_t thread;
    size_t updates;
    size_t reads;
    size_t interval;
} thread;

typedef struct {
    rwlock lock;
    unsigned data;
    size_t updates;
} data;

static data d[datasize];

int thread_routine(void *arg)
{
    thread *self = (thread *) arg;

    size_t repeats = 0;
    for (size_t i = 0, e = 0; i < iters; i++) {
        /* Each self->interval iterations, perform an update operation. */
        if (!(i % self->interval)) {
            int status = rwlock_writelock(&d[e].lock);
            if (status != thrd_success) return status;

            d[e].data = self->threadnum;
            d[e].updates++;
            self->updates++;

            status = rwlock_writeunlock(&d[e].lock);
            if (status != thrd_success) return status;
        }
        else {
            int status = rwlock_readlock(&d[e].lock);
            if (status != thrd_success) return status;

            self->reads++;
            if (d[e].data == self->threadnum) ++repeats;

            status = rwlock_readunlock(&d[e].lock);
            if (status != thrd_success) return status;
        }
        e++;
        if (e >= datasize) e = 0;
    }
    if (repeats > 0) printf("Thread %u found unchanged elements %zu times.\n", self->threadnum, repeats);
    return thrd_success;
}

int main(void)
{
    thread threads[nthreads];

    for (size_t i = 0; i < datasize; i++) {
        d[i].data = 0;
        d[i].updates = 0;
        int status = rwlock_init(&d[i].lock);
        if (status != thrd_success) return EXIT_FAILURE;
    }

    unsigned seed = 1u;
    for (size_t i = 0; i < nthreads; i++) {
        threads[i].threadnum = i;
        threads[i].updates = 0;
        threads[i].reads = 0;
        threads[i].interval = rand_r(&seed) % 71;
        int status = thrd_create(&threads[i].thread, thread_routine, &threads[i]);
        if (status != thrd_success) return EXIT_FAILURE;
    }

    size_t threadupdates = 0;
    for (size_t i = 0; i < nthreads; i++) {
        int status = thrd_join(threads[i].thread, 0);
        if (status != thrd_success) return EXIT_FAILURE;
    }
    for (size_t i = 0; i < nthreads; i++) {
        threadupdates += threads[i].updates;
        printf("Thread %02zu: interval %zu, updates %zu, reads %zu.\n", i, threads[i].interval,
               threads[i].updates, threads[i].reads);
    }

    size_t dataupdates = 0;
    for (size_t i = 0; i < datasize; i++) {
        dataupdates += d[i].updates;
        printf("data %02zu: value %u, %zu updates.\n", i, d[i].data, d[i].updates);
        rwlock_destroy(&d[i].lock);
    }

    printf("%zu thread updates, %zu data updates.\n", threadupdates, dataupdates);
    return EXIT_SUCCESS;
}
