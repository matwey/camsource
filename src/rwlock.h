#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#include <pthread.h>

/*
 * Read/write lock/mutex.
 * Controls access to a shared resource that is occasionally updated
 * but frequently read by multiple threads.
 */

struct rwlock
{
	unsigned int readers;
	pthread_mutex_t rmutex;
	pthread_mutex_t wmutex;
	pthread_cond_t cond;
};

/* Constructs and destructs a rwlock */
void rwlock_init(struct rwlock *);
void rwlock_destroy(struct rwlock *);

/* For a reader: lock, read, unlock */
void rwlock_rlock(struct rwlock *);
void rwlock_runlock(struct rwlock *);

/* For a writer: lock, read/write, unlock */
void rwlock_wlock(struct rwlock *);
void rwlock_wunlock(struct rwlock *);	

#endif

