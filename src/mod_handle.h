#ifndef _MOD_HANDLE_H_
#define _MOD_HANDLE_H_

#include "rwlock.h"

#define MAX_MODULES 32

struct module
{
	void *dlhand;
	const char *name;
};

extern struct module modules[MAX_MODULES];
extern struct rwlock modules_lock;

void mod_init(void);
void mod_load_all(void);
void mod_load(const char *);
int mod_try_load(const char *, const char *);

#endif

