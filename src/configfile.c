#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <libxml/parser.h>

#include "config.h"

#include "configfile.h"
#include "rwlock.h"

struct config config;
struct rwlock config_lock;

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
	rwlock_init(&config_lock);
	
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
	if (!node || !xmlStrEqual(node->name, "camsourceconfig"))
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

void
config_load_modules()
{
	struct config newconfig;
	xmlDocPtr doc;
	xmlNodePtr node;
	char *modname, *prop;
	int modidx;
	int ret;
	
	memset(&newconfig, 0, sizeof(newconfig));
	
	rwlock_rlock(&configdoc_lock);
	doc = xmlCopyDoc(configdoc, 1);
	rwlock_runlock(&configdoc_lock);
	if (!doc)
	{
		printf("xmlCopyDoc failed\n");
		return;
	}
	
	modidx = 0;
	node = xmlDocGetRootElement(doc);
	for (node = node->children; node; node = node->next)
	{
		if (!xmlStrEqual(node->name, "module"))
			continue;
		modname = xmlGetProp(node, "name");
		if (!modname)
		{
			printf("<module> tag without name property\n");
			continue;
		}
		prop = xmlGetProp(node, "load");
		if (!prop || (strcmp(prop, "yes") && strcmp(prop, "1") && strcmp(prop, "on")))
		{
			if (prop)
				free(prop);
			free(modname);
			continue;
		}
		free(prop);
		newconfig.modules[modidx] = config_load_module(modname);
		if (!newconfig.modules[modidx])
		{
			printf("Failed to load module %s\n", modname);
			free(modname);
			continue;
		}
		
		ret = config_activate_module(newconfig.modules[modidx]);
		if (ret)
		{
			printf("Failed to activate module %s\n", modname);
			free(modname);
			newconfig.modules[modidx] = NULL;
			continue;
		}

		free(modname);
		
		modidx++;
		if (modidx >= NUM_MODULES)
		{
			printf("Max num of modules exceeded\n");
			break;
		}
	}
	
	xmlFreeDoc(doc);
	
	rwlock_wlock(&config_lock);
	memcpy(&config, &newconfig, sizeof(config));
	rwlock_wunlock(&config_lock);
}

void *
config_load_module(const char *mod)
{
	char modname[256];
	void *dlh;
	
	snprintf(modname, sizeof(modname) - 1, "lib%s.so", mod);
	dlh = dlopen(modname, RTLD_NOW);
	if (dlh)
		return dlh;

	snprintf(modname, sizeof(modname) - 1, ".libs/lib%s.so", mod);
	dlh = dlopen(modname, RTLD_NOW);
	if (dlh)
		return dlh;

	snprintf(modname, sizeof(modname) - 1, "src/.libs/lib%s.so", mod);
	dlh = dlopen(modname, RTLD_NOW);
	if (dlh)
		return dlh;
	
	printf("Last dlopen error: %s\n", dlerror());
	
	return NULL;
}

int
config_activate_module(void *h)
{
	int (*init)(void);
	int ret;
	
	init = dlsym(h, "init");
	if (!init || !dlsym(h, "thread") || !dlsym(h, "name") || !dlsym(h, "tid"))
	{
		dlclose(h);
		return -1;
	}
	
	ret = init();
	if (ret)
	{
		dlclose(h);
		return -1;
	}
	
	return 0;
}

void
config_start_threads()
{
	int i;
	void *(*thread)(void *);
	pthread_t *tid;
	
	rwlock_rlock(&config_lock);
	
	for (i = 0; i < NUM_MODULES; i++)
	{
		if (!config.modules[i])
			continue;
		thread = dlsym(config.modules[i], "thread");
		tid = dlsym(config.modules[i], "tid");
		pthread_create(tid, NULL, thread, NULL);
	}

	rwlock_runlock(&config_lock);
}

xmlNodePtr
config_find_mod_section(xmlDocPtr doc, const char *mod)
{
	xmlNodePtr node;
	char *modname;
	
	node = xmlDocGetRootElement(doc);
	for (node = node->children; node; node = node->next)
	{
		if (!xmlStrEqual(node->name, "module"))
			continue;
		modname = xmlGetProp(node, "name");
		if (!modname)
			continue;
		if (strcmp(modname, mod))
		{
			free(modname);
			continue;
		}
		free(modname);
		return node->children;
	}
	return NULL;
}

