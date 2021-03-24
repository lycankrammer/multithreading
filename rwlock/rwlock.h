/*
 * C header file: rwlock.h
 */
#ifndef _RWLOCK_GUARD_
#define _RWLOCK_GUARD_ 1

#include "../threading_present.h"

#if defined(THREADING_USE_C11THREADS)
#  include <threads.h>
#else
#  error "C11 threads not found."
#endif

#if defined(THREADING_USE_ATOMICS)
#  include <stdatomic.h>
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @file
 * @brief Implementation of a read/write lock with read precedence.
 *
 * @remark This implementation is based on Butenhof's.
 */

typedef struct rwlock {
    size_t readerswait;          //*< count of readers waiting
    size_t writerswait;          //*< count of writers waiting
    size_t readersactive;        //*< count of readers active
    bool writersactive;          //*< there's a writer active or not
#if defined(THREADING_USE_ATOMICS)
    _Atomic(unsigned) valid;     //*< set to a distinguishable value when valid
#else
    unsigned valid;              //*< set to a distinguishable value when valid
#endif
    mtx_t mutex;                 //*< serializes access to structure
    cnd_t read;                  //*< wait for read
    cnd_t write;                 //*< wait for write
} rwlock;

#define _RWLOCK_VALID 0xfacade

/**
 * @brief The following interfaces have the following possible
 * return values in addition to the ones specified:
 *
 * @retval thrd_success  indicates successful return value
 * @retval thrd_timedout indicates timed out return value
 * @retval thrd_busy     indicates unsuccessful return value due to resource temporary unavailable
 * @retval thrd_nomem    indicates unsuccessful return value due to out of memory condition
 * @retval thrd_error    indicates unsuccessful return value
 *
 * @see C11 Standard (ISO/IEC 9899:2011), 7.26.1/5
 */

/**
 * @brief Initializes a read/write lock.
 *
 * @retval EFAULT if @a rwl is null
 * @see above for more possible return values.
 */
extern int rwlock_init(rwlock rwl[restrict static 1]);

/**
 * @brief Destroys a read/write lock.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_destroy(rwlock rwl[restrict static 1]);

/**
 * @brief Lock a read/write lock for read acess.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_readlock(rwlock rwl[restrict static 1]);

/**
 * @brief Attempt to lock a read/write lock for read access. Do
 * not block if unavailable.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_readtrylock(rwlock rwl[restrict static 1]);

/**
 * @brief Unlock a read/write lock from read access.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 *
 * @warning Note that there is a possible race here. If another
 * thread that is interested in read access calls rwl_readlock
 * or rwl_tryreadlock before the awakened writer can run, the
 * reader may "win", despite the fact that we just selected a
 * writer.
 */
extern int rwlock_readunlock(rwlock rwl[restrict static 1]);

/**
 * @brief Lock a read/write lock for write access.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_writelock(rwlock rwl[restrict static 1]);

/**
 * @brief Attempt to lock a read/write lock for write access. Do
 * not block if unavailable.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_writetrylock(rwlock rwl[restrict static 1]);

/**
 * @brief Unlock a read/write lock from write access.
 *
 * @retval EFAULT if @a rwl is null
 * @retval EINVAL if the structure is perceived as invalid
 * @see above for more possible return values.
 */
extern int rwlock_writeunlock(rwlock rwl[restrict static 1]);


#include <limits.h>
#include <errno.h>
#if !defined(EFAULT)
#  define EFAULT EDOM
#endif
#if !defined(EINVAL)
#  define EINVAL (EFAULT - EOF)
#  if EINVAL > INT_MAX
#    error "EINVAL constant is too large."
#  endif
#endif

#endif
