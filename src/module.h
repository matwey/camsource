#ifndef _MODULE_H_
#define _MODULE_H_

#include <pthread.h>

extern char name[];		/* Module name */
extern pthread_t tid;	/* Thread id/handle, filled in by camsource */

int init(void);			/* Pre-thread init */
void *thread(void *);	/* Thread proc */

#endif

