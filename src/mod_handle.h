#ifndef _MOD_HANDLE_H_
#define _MOD_HANDLE_H_

#include <pthread.h>
#include <libxml/parser.h>

#define MAX_MODULES 32

/* Module context.
 * node:   Pointer to the config section for the module
 *         in the xml tree. May be NULL if module was
 *         loaded as dependency and no config section
 *         for it could be found.
 * tid:    Thread-id, filled in by camsource when thread
 *         is started. Meaningless for non-thread mods.
 * custom: Custom module data, usually set by module's
 *         init() to hold context/config data.
 */
struct module_ctx
{
	xmlNodePtr node;
	pthread_t tid;
	void *custom;
};

struct module
{
	void *dlhand;	/* as returned by dlopen(). The same handle may appear several times in the list */
	char *name;	/* if == NULL, slot is unused */
	char *alias;
	struct module_ctx ctx;	/* Passed to init() and thread() */
};

/* Built once on startup, then never modified and readonly */
extern struct module modules[MAX_MODULES];

/* None of these functions may be used from threads. All module loading/initing
 * must happen before threads are started */
void mod_init(void);
/* Loads all "active" modules as given in configdoc */
void mod_load_all(void);
/* Loads one module by name, optionally with an xml config */
int mod_load(char *, xmlNodePtr);

/* Private functions */
/* Given an xml node, return alias name if present, otherwise module name */
char *mod_get_aliasname(xmlNodePtr, char *);
/* Given a module name, try to dlopen its lib. Lib names are created from predefined patterns */
void *mod_try_dlopen(char *);
/* Make sure the filename matches the module's built in name. Return -1 on error. */
int mod_validate(void *, char *);
/* Look at the module's dep list, and load each mod. Return -1 on error */
int mod_load_deps(struct module *);
/* Calls module's init(). Returns init()'s return value (0 == success) */
int mod_init_mod(struct module *);
/* Cleans up module struct and dlclose()s lib if not used any more */
void mod_close(struct module *);
xmlNodePtr mod_find_config(char *);

void mod_start_all(void);

struct module *mod_find(char *, char *);

#endif

