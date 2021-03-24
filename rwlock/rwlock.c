/*
 * Implementation of rwlock.
 */
#include "rwlock.h"

int rwlock_init(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;

    rwl->readerswait   = 0;
    rwl->writerswait   = 0;
    rwl->readersactive = 0;
    rwl->writersactive = false;
    int status = mtx_init(&rwl->mutex, mtx_plain);
    if (status != thrd_success) return status;

    status = cnd_init(&rwl->read);
    if (status != thrd_success) goto CLEANUP_LV2;
    status = cnd_init(&rwl->write);
    if (status != thrd_success) goto CLEANUP_LV1;

    rwl->valid = _RWLOCK_VALID;
    return thrd_success;

CLEANUP_LV1:
    cnd_destroy(&rwl->read);
CLEANUP_LV2:
    mtx_destroy(&rwl->mutex);
    return status;
}

int rwlock_destroy(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    /* Check whether any threads own the lock. */
    if (rwl->readersactive > 0 || rwl->writersactive) {
        mtx_unlock(&rwl->mutex);
        return thrd_busy;
    }
    /* Check whether any threads are waiting. */
    if (rwl->readerswait > 0 || rwl->writerswait > 0) {
        mtx_unlock(&rwl->mutex);
        return thrd_busy;
    }

    rwl->valid = 0;
    status = mtx_unlock(&rwl->mutex);
    if (status != thrd_success) return status;

    mtx_destroy(&rwl->mutex);
    cnd_destroy(&rwl->read);
    cnd_destroy(&rwl->write);
    return thrd_success;
}

int rwlock_readlock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    if (rwl->writersactive) {
        rwl->readerswait++;
        while (rwl->writersactive) {
            status = cnd_wait(&rwl->read, &rwl->mutex);
            if (status != thrd_success) break;
        }
        rwl->readerswait--;
    }
    if (status == thrd_success) rwl->readersactive++;

    int statusunlck = mtx_unlock(&rwl->mutex);
    if (statusunlck != thrd_success) return statusunlck;
    return status;
}

int rwlock_readtrylock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    if (!rwl->writersactive) rwl->readersactive++;
    else return thrd_busy;

    status = mtx_unlock(&rwl->mutex);
    return status;
}

int rwlock_readunlock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    rwl->readersactive--;
    if (!rwl->readersactive && rwl->writerswait > 0) status = cnd_signal(&rwl->write);

    int statusunlck = mtx_unlock(&rwl->mutex);
    if (statusunlck != thrd_success) return statusunlck;
    return status;
}

int rwlock_writelock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    if (rwl->readersactive > 0 || rwl->writersactive) {
        rwl->writerswait++;
        while (rwl->readersactive > 0 || rwl->writersactive) {
            status = cnd_wait(&rwl->write, &rwl->mutex);
            if (status != thrd_success) break;
        }
        rwl->writerswait--;
    }
    if (status == thrd_success) rwl->writersactive = true;

    int statusunlck = mtx_unlock(&rwl->mutex);
    if (statusunlck != thrd_success) return statusunlck;
    return status;
}

int rwlock_writetrylock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    if (!rwl->readersactive && !rwl->writersactive) rwl->writersactive = true;
    else return thrd_busy;

    status = mtx_unlock(&rwl->mutex);
    return status;
}

int rwlock_writeunlock(rwlock rwl[restrict static 1])
{
    if (!rwl) return EFAULT;
    if (rwl->valid != _RWLOCK_VALID) return EINVAL;

    int status = mtx_lock(&rwl->mutex);
    if (status != thrd_success) return status;

    rwl->writersactive = false;
    if (rwl->readerswait) {
        status = cnd_broadcast(&rwl->read);
        if (status != thrd_success) goto UNLOCK_RETURN;
    }
    else if (rwl->writerswait) {
        status = cnd_signal(&rwl->write);
        if (status != thrd_success) goto UNLOCK_RETURN;
    }

    int statusunlck;
UNLOCK_RETURN:
    statusunlck = mtx_unlock(&rwl->mutex);
    if (statusunlck != thrd_success) return statusunlck;
    return status;
}
