#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>

#include "rwlock.h"

extern struct image current_image;
extern pthread_cond_t image_cond;
extern pthread_mutex_t image_cond_mutex;
extern struct rwlock image_lock;

void grab_thread_init(void);
void *grab_thread(void *);

#endif

