/* Camsource module interface */

#ifndef _MODULE_H_
#define _MODULE_H_

/*
 * There are three kinds of modules:
 * .) MODULE_THREAD is a worker module which runs in its own thread.
 *    A thread module has a thread() function which will be run
 *    in its own thread. It also has the thread-id variable, which
 *    is filled in by camsource with the thread's id.
 * .) MODULE_FILTER is an image filter module. It provides a filter()
 *    function that takes an image as input and outputs another one.
 *    Filtering happens in-place.
 * .) MODULE_GENERIC is a module which doesn't do anything by itself,
 *    but provides special functionality for other modules. It is
 *    usually listed as dependency in other modules.
 * You must define one of the above 
 * Things common to all kinds of modules:
 * .) The module "name". It must match the filename, e.g. a module
 *    called "libhello.so" would have a name of "hello".
 * .) Dependency list ("deps"). It's a null terminated array of
 *    strings, each giving the name of another module which will
 *    be autoloaded before the current module is activated.
 * .) An optional init() function. If present, it will be called
 *    when the module is loaded. Returning anything other than 0
 *    means that the module init has failed, and the module will
 *    be unloaded.
 */

#ifdef MODULE_THREAD
# include <pthread.h>
#endif
#ifdef MODULE_FILTER
# include "grab.h"
#endif

extern char *name;
extern char *deps[];

#ifdef MODULE_THREAD
extern pthread_t tid;	/* Thread id/handle, filled in by camsource */
#endif

int init(void);			/* On-load (pre-thread) init (optional) */

#ifdef MODULE_THREAD
void *thread(void *);
#endif

#ifdef MODULE_FILTER
int filter(struct image *);	/* Returns 0 on success */
#endif

#if !defined(MODULE_THREAD) && !defined(MODULE_FILTER) && !defined(MODULE_GENERIC)
# error "Must define the module type prior to including module.h"
#endif

#endif

