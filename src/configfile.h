#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <libxml/parser.h>

#include "rwlock.h"

#define NUM_MODULES 32

extern xmlDocPtr configdoc;
extern struct rwlock configdoc_lock;
extern char *ourconfig;

struct config
{
	void *modules[NUM_MODULES];
};

extern struct config config;
extern struct rwlock config_lock;

int config_init(void);
int config_load(void);
void config_load_modules(void);
void *config_load_module(const char *);
int config_activate_module(void *);
void config_start_threads(void);
xmlNodePtr config_find_mod_section(xmlDocPtr, const char *);

#endif

