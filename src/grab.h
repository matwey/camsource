#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>

#include "image.h"

/* The grabimage struct and its only instance current_img.
 * This is where the grabbing thread stores the frames.
 * Every time a new frame is grabbed, the index is
 * incremented by one. The function grab_get_image is
 * provided to make it easy for a worker thread to read
 * images out of this.
 *
 * The grabber thread works like this:
 *  1) Acquire mutex.
 *  2) If request != 0, skip to step 5.
 *  3) Wait on request_cond, releasing mutex.
 *  4) Wake up from request_cond, reacquiring the mutex.
 *  5) Release mutex.
 *  6) Read frame from device into private local buffer,
 *     convert it to rgb palette, apply any global filters.
 *  7) Acquire mutex.
 *  8) Put new frame into current_img, increase index and
 *     set request = 0;
 *  9) Broadcast signal on ready_cond.
 * 10) Release mutex.
 * 11) Jump back to step 1.
 */
struct grabimage
{
	pthread_mutex_t mutex;

	struct image img;
	unsigned int idx;
	
	int request;
	pthread_cond_t request_cond;
	pthread_cond_t ready_cond;
};
extern struct grabimage current_img;

void grab_thread_init(void);
void *grab_thread(void *);

/* Applies global filters (as found in configdoc xml tree) to image in-place */
void grab_glob_filters(struct image *);

/* Does the necessary locking and checking to get a _new_ frame out of
 * current_img. A copy of the new frame is stored into the first argument
 * (which must be image_destroy()'d by the caller). The second argument
 * is a pointer to a local index var, holding the index of the last
 * grabbed image. Init this var to 0 to make sure a new frame is grabbed
 * at the beginning. If the index in current_img is larger than the value
 * of the pointer, the current frame is copied without signalling the
 * grabbing thread.
 *
 * Example code:
 *
 * struct image img;
 * unsigned int idx = 0;
 * for (;;) {
 *   grab_get_image(&img, &idx);
 *   do_something_with(&img);
 * }
 *
 * If you only want a single image (after which the thread exits), pass
 * NULL as the second argument.
 */
void grab_get_image(struct image *, unsigned int *);

#endif

