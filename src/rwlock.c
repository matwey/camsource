#include <pthread.h>

#include "config.h"

#include "rwlock.h"

void
rwlock_init(struct rwlock *lock)
{
	lock->readers = 0;
	pthread_mutex_init(&lock->rmutex, NULL);
	pthread_mutex_init(&lock->wmutex, NULL);
	pthread_cond_init(&lock->cond, NULL);
}

void
rwlock_destroy(struct rwlock *lock)
{
	pthread_mutex_destroy(&lock->rmutex);
	pthread_mutex_destroy(&lock->wmutex);
	pthread_cond_destroy(&lock->cond);
}

void
rwlock_rlock(struct rwlock *lock)
{
	/* If there's a writer active or waiting, stop and wait until writer is finished */
	pthread_mutex_lock(&lock->wmutex);
	pthread_mutex_lock(&lock->rmutex);
	lock->readers++;
	pthread_mutex_unlock(&lock->rmutex);
	pthread_mutex_unlock(&lock->wmutex);
}

void
rwlock_runlock(struct rwlock *lock)
{
	pthread_mutex_lock(&lock->rmutex);
	lock->readers--;
	/* If a writer is waiting, signal it that a reader is done */
	pthread_cond_signal(&lock->cond);
	pthread_mutex_unlock(&lock->rmutex);
}

void
rwlock_wlock(struct rwlock *lock)
{
	pthread_mutex_lock(&lock->wmutex);
	/* Check if there are any readers */
	pthread_mutex_lock(&lock->rmutex);
	if (lock->readers)
	{
		/* Wait until a reader is done */
		pthread_cond_wait(&lock->cond, &lock->rmutex);
	}
}

void
rwlock_wunlock(struct rwlock *lock)
{
	pthread_mutex_unlock(&lock->rmutex);
	pthread_mutex_unlock(&lock->wmutex);
}

