#include <stdio.h>
#include <dlfcn.h>
#include <libxml/parser.h>

#include "config.h"

#include "filter.h"
#include "image.h"
#include "mod_handle.h"
#include "rwlock.h"
#include "xmlhelp.h"

void
filter_apply(struct image *img, xmlNodePtr node)
{
	struct module *mod;
	char *filtername;
	int (*filter)(struct image *, xmlNodePtr);
	
	for (node = node->children; node; node = node->next)
	{
		if (!xml_isnode(node, "filter"))
			continue;
		filtername = xml_attrval(node, "name");
		if (!filtername)
		{
			printf("<filter> without name\n");
			continue;
		}
		rwlock_rlock(&modules_lock);
		mod = mod_find(filtername);
		rwlock_runlock(&modules_lock);
		if (mod)
		{
			filter = dlsym(mod->dlhand, "filter");
			if (filter)
				filter(img, node);
			else
				printf("Module %s has no \"filter\" routine\n", filtername);
		}
	}
}

