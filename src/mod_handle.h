#ifndef _MOD_HANDLE_H_
#define _MOD_HANDLE_H_

#include "rwlock.h"

#define MAX_MODULES 32

struct module
{
	void *dlhand;	/* if == NULL, slot is unused */
	const char *name;
};

extern struct module modules[MAX_MODULES];
extern struct rwlock modules_lock;
/* The lock not only protects the "modules" array, but also
 * the status of the modules themselves. That is, don't
 * dlclose any modules without holding the lock. */

void mod_init(void);
void mod_load_all(void);
/* caller must hold modules_lock rw for: */
int mod_load(const char *);
int mod_try_load(const char *, const char *);
/* . */

void mod_start_all(void);

/* caller must hold module_lock r for: */
struct module *mod_find(const char *);
/* . */

#endif

