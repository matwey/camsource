#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/parser.h>

#include "config.h"

#include "configfile.h"
#include "rwlock.h"
#include "xmlhelp.h"

char *globalconfigs[] =
{
	"/etc/camsource.conf",
	"/etc/defaults/camsource",
	"/usr/local/etc/camsource.conf",
	"/usr/local/etc/defaults/camsource",
	NULL
};
char localconfig[256];
char *ourconfig;

xmlDocPtr configdoc;
struct rwlock configdoc_lock;

int
config_init()
{
	char *s, **ss;
	
	rwlock_init(&configdoc_lock);
	
	s = getenv("HOME");
	if (s)
		snprintf(localconfig, sizeof(localconfig) - 1, "%s/.camsource", s);
	
	if (!access(localconfig, R_OK))
	{
		ourconfig = localconfig;
		goto found;
	}
	else
	{
		for (ss = globalconfigs; *ss; ss++)
		{
			if (access(*ss, R_OK))
				continue;
			ourconfig = *ss;
			goto found;
		}
	}
	return -1;

found:
	return 0;
}

int
config_load()
{
	xmlDocPtr doc;
	xmlNodePtr node;
	
	doc = xmlParseFile(ourconfig);
	if (!doc)
		return -1;
		
	node = xmlDocGetRootElement(doc);
	if (!xml_isnode(node, "camsourceconfig"))
	{
		printf("Root node isn't 'camsourceconfig'\n");
		xmlFreeDoc(doc);
		return -1;
	}
	
	rwlock_wlock(&configdoc_lock);
	configdoc = doc;
	rwlock_wunlock(&configdoc_lock);
	
	return 0;
}

xmlNodePtr
config_find_mod_section(xmlDocPtr doc, const char *mod)
{
	xmlNodePtr node;
	char *modname;
	
	node = xmlDocGetRootElement(doc);
	for (node = node->children; node; node = node->next)
	{
		if (!xml_isnode(node, "module"))
			continue;
		modname = xml_attrval(node, "name");
		if (!modname || strcmp(modname, mod))
			continue;
		return node;
	}
	return NULL;
}

