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
		
		rwlock_wlock(&modules_lock);
		mod_load(modname);
		rwlock_wunlock(&modules_lock);

		free(modname);
	}
	
	xmlFreeDoc(doc);
}

/* caller must hold modules_lock rw */
int
mod_load(const char *mod)
{
	char modname[256];
	int i;
	
	/* check if mod is already loaded */
	for (i = 0; i < MAX_MODULES; i++)
	{
		if (!modules[i].dlhand)
			continue;
		if (!strcmp(modules[i].name, mod))
			return 0;
	}
	
	snprintf(modname, sizeof(modname) - 1, "lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return 0;

	snprintf(modname, sizeof(modname) - 1, ".libs/lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return 0;

	snprintf(modname, sizeof(modname) - 1, "src/.libs/lib%s.so", mod);
	if (!mod_try_load(mod, modname))
		return 0;
	
	printf("Failed to load module %s\n", mod);
	printf("Last dlopen error: %s\n", dlerror());
	
	return -1;
}

/* caller must hold modules_lock rw */
int
mod_try_load(const char *mod, const char *file)
{
	void *dlh;
	int modidx;
	char **name;
	char **deps;
	int (*init)(void);
	int ret;
	
	/* We have to use lazy symbol resolving, otherwise we couldn't
	 * load dependency libs */
	dlh = dlopen(file, RTLD_LAZY | RTLD_GLOBAL);
	if (!dlh)
		return -1;
	
	for (modidx = 0; modidx < MAX_MODULES; modidx++)
	{
		if (modules[modidx].dlhand)
			continue;
		goto found;
	}
	printf("Max num of modules exceeded when trying to load %s\n", mod);
	dlclose(dlh);
	return -1;
	
found:
	name = dlsym(dlh, "name");
	deps = dlsym(dlh, "deps");
	init = dlsym(dlh, "init");
	if (!name || !deps || !init || !dlsym(dlh, "tid") || !dlsym(dlh, "thread"))
	{
		printf("Necessary module symbol not found in %s\n", mod);
		dlclose(dlh);
		return -1;
	}
	if (strcmp(*name, mod))
	{
		printf("Module name doesn't match filename (%s != %s), not loaded\n", mod, *name);
		dlclose(dlh);
		return -1;
	}
	modules[modidx].dlhand = dlh;
	modules[modidx].name = *name;

	for (; *deps; deps++)
	{
		if (mod_load(*deps))
		{
			printf("Module %s depends on %s, which failed to load. %s not loaded.\n", *name, *deps, *name);
			modules[modidx].dlhand = NULL;
			dlclose(dlh);
			return -1;
		}
	}
	
	ret = init();
	if (ret)
	{
		printf("Module %s failed to initialize, not loaded.\n", *name);
		modules[modidx].dlhand = NULL;
		dlclose(dlh);
		return -1;
	}

	return 0;
}

void
mod_start_all()
{
	int i;
	void *(*thread)(void *);
	pthread_t *tid;
	pthread_attr_t attr;
	
	rwlock_rlock(&modules_lock);
	
	for (i = 0; i < MAX_MODULES; i++)
	{
		if (!modules[i].dlhand)
			continue;
		thread = dlsym(modules[i].dlhand, "thread");
		tid = dlsym(modules[i].dlhand, "tid");
		
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(tid, &attr, thread, NULL);
		pthread_attr_destroy(&attr);
	}

	rwlock_runlock(&modules_lock);
}

