#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>
#include <sys/time.h>

#include "image.h"

struct camdev;

struct grab_ctx
{
	unsigned int idx;
	struct timeval tv;
};



void grab_thread_init(void);
struct camdev *grab_open(void);
void *grab_thread(void *);

/* Does the necessary locking and checking to get a _new_ frame out of
 * current_img. A copy of the new frame is stored into the first argument
 * (which must be image_destroy()'d by the caller). The second argument
 * is a pointer to a local index var, holding the index of the last
 * grabbed image. Init this var to 0 to make sure a new frame is grabbed
 * at the beginning. If the index in current_img is larger than the value
 * of the pointer, the current frame is copied without signalling the
 * grabbing thread.
 * (The above description is a bit outdated now that we use a struct
 * grab_ctx instead of a simple int idx.)
 *
 * Example code:
 *
 * struct image img;
 * struct grabimage_ctx ctx;
 * memset(&ctx, 0, sizeof(ctx));
 * for (;;) {
 *   grab_get_image(&img, &ctx);
 *   do_something_with(&img);
 * }
 *
 * If you only want a single image (after which the thread exits), pass
 * NULL as the second argument.
 */
void grab_get_image(struct image *, struct grab_ctx *);

#endif

