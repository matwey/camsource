#include <stdio.h>
#include <dlfcn.h>
#include <libxml/parser.h>

#include "config.h"

#include "mod_handle.h"
#include "rwlock.h"
#include "configfile.h"

struct module modules[MAX_MODULES];
struct rwlock modules_lock;

void
mod_init()
{
	rwlock_init(&modules_lock);
}

void
mod_load_all()
{
	xmlDocPtr doc;
	xmlNodePtr node;
	char *modname, *prop;
	int modidx;
	
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
		
		mod_load(modname);

		free(modname);
	}
	
	xmlFreeDoc(doc);
}

void
mod_load(const char *mod)
{
	char modname[256];
	int i;
	
	/* check if mod is already loaded */
	rwlock_rlock(&modules_lock);
	for (i = 0; i < MAX_MODULES; i++)
	{
		if (!modules[i].dlhand)
			continue;
		if (!strcmp(modules[i].name, mod))
		{
			rwlock_runlock(&modules_lock);
			printf("Mod already loaded: %s\n", mod);
			return;
		}
	}
	rwlock_runlock(&modules_lock);
	
	snprintf(modname, sizeof(modname) - 1, "lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return;

	snprintf(modname, sizeof(modname) - 1, ".libs/lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return;

	snprintf(modname, sizeof(modname) - 1, "src/.libs/lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return;
	
	printf("Failed to dlopen module %s\n", mod);
	printf("Last dlopen error: %s\n", dlerror());
}

int
mod_try_load(const char *mod, const char *file)
{
	void *dlh;
	int modidx;
	char **name;
	char **deps;
	void (*init)(void);
	
	/* We have to use lazy symbol resolving, otherwise we couldn't
	 * load dependency libs */
	dlh = dlopen(file, RTLD_LAZY | RTLD_GLOBAL);
	if (!dlh)
		return -1;
	
	rwlock_wlock(&modules_lock);
	for (modidx = 0; modidx < MAX_MODULES; modidx++)
	{
		if (modules[modidx].dlhand)
			continue;
		goto found;
	}
	printf("Max num of modules exceeded when trying to load %s\n", mod);
loaderr:
	rwlock_wunlock(&modules_lock);
	dlclose(dlh);
	return -1;
	
found:
	name = dlsym(dlh, "name");
	deps = dlsym(dlh, "deps");
	init = dlsym(dlh, "init");
	if (!name || !deps || !init || !dlsym(dlh, "tid") || !dlsym(dlh, "thread"))
	{
		printf("Necessary module symbol not found in %s\n", mod);
		goto loaderr;
	}
	if (strcmp(*name, mod))
	{
		printf("Module name doesn't match filename (%s != %s), not loaded\n", mod, *name);
		goto loaderr;
	}
	printf("Mod loaded: %s\n", *name);
	modules[modidx].dlhand = dlh;
	modules[modidx].name = *name;

	rwlock_wunlock(&modules_lock);

	for (; *deps; deps++)
		mod_load(*deps);
	
	return 0;
}

