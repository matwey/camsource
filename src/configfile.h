#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <libxml/parser.h>

#include "mod_handle.h"

/* Created once on startup, then never modified and readonly */
extern xmlDocPtr configdoc;
extern char *ourconfig;

int config_init(void);
int config_load(void);

#endif

