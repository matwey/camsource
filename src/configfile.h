#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <libxml/parser.h>

#include "mod_handle.h"

/* Created once on startup, then never modified and readonly */
extern xmlDocPtr configdoc;

/* The config file we've used to build the xml tree */
extern char *ourconfig;

/* Figures out which config file to use, and sets up the "ourconfig" pointer. Returns -1 on error */
int config_init(void);

/* Opens "ourconfig" and builds the xml tree (configdoc) from it. Returns -1 on error */
int config_load(void);

#endif

