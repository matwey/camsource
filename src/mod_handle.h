#ifndef _MOD_HANDLE_H_
#define _MOD_HANDLE_H_

#include <pthread.h>
#include <libxml/parser.h>

#define MAX_MODULES 32

struct module_ctx
{
	xmlNodePtr node;
	pthread_t tid;
	void *custom;
};

struct module
{
	void *dlhand;
	char *name;	/* if == NULL, slot is unused */
	char *alias;
	struct module_ctx ctx;
};

/* Built once on startup, then never modified and readonly */
extern struct module modules[MAX_MODULES];

void mod_init(void);
void mod_load_all(void);
int mod_load(char *, xmlNodePtr);
char *mod_get_aliasname(xmlNodePtr, char *);
void *mod_try_dlopen(char *);
int mod_validate(void *, char *);
int mod_load_deps(struct module *);
int mod_init_mod(struct module *);
void mod_close(struct module *);

void mod_start_all(void);

struct module *mod_find(char *, char *);

#endif

