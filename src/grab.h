#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>

#include "rwlock.h"
#include "image.h"

struct grabimage
{
	struct image img;
	unsigned int idx;
};

extern struct grabimage current_img;
extern pthread_cond_t current_img_cond;
extern pthread_mutex_t current_img_cond_mutex;
extern struct rwlock current_img_lock;

void grab_thread_init(void);
void *grab_thread(void *);

void grab_glob_filters(struct image *);

#endif

