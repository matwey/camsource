#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <libxml/parser.h>

#include "rwlock.h"
#include "mod_handle.h"

extern xmlDocPtr configdoc;
extern struct rwlock configdoc_lock;
extern char *ourconfig;

int config_init(void);
int config_load(void);
xmlNodePtr config_find_mod_section(xmlDocPtr, const char *);

#endif

