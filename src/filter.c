#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <unistd.h>
#include <libxml/parser.h>

#include "config.h"

#include "filter.h"
#include "image.h"
#include "mod_handle.h"
#include "xmlhelp.h"
#include "grab.h"

static struct instance_ctx instances[MAX_INSTANCES];

int
filter_apply(struct image *img, xmlNodePtr node)
{
	struct module *mod;
	char *filtername;
	int (*filter)(struct image *, xmlNodePtr, void **);
	int ret;
	int i;
	
	for (node = node->xml_children; node; node = node->next)
	{
		if (!xml_isnode(node, "filter"))
			continue;
		filtername = xml_attrval(node, "name");
		if (!filtername)
		{
			printf("<filter> without name\n");
			continue;
		}
		mod = mod_find(filtername, xml_attrval(node, "alias"));
		if (mod)
		{
			filter = dlsym(mod->dlhand, "filter");
			if (filter) {
				for (i = 0; i < MAX_INSTANCES; i++) {
					if (!instances[i].node)
						goto foundfree;
					if (instances[i].node == node)
						goto found;
				}
				
				printf("Out of filter instance storage slots (%s)\n", filtername);
				return -1;
				
foundfree:
				instances[i].node = node;
found:
				ret = filter(img, node, &instances[i].ctx);
				if (ret > 0)
					return ret;
			}
			else
				printf("Module %s has no \"filter\" routine\n", filtername);
		}
	}
	
	return 0;
}

void
filter_get_image(struct image *img, struct grab_ctx *ctx, ...)
{
	int ret;
	xmlNodePtr node;
	va_list val;
	char *threadname;
	
	threadname = NULL;
	
	va_start(val, ctx);
	while ((node = va_arg(val, xmlNodePtr))) {
		for (node = node->xml_children; node; node = node->next) {
			if (!xml_isnode(node, "grabdev"))
				continue;
			threadname = xml_getcontent(node);
			goto donefind;
		}
	}
donefind:
	va_end(val);
	
	if (!threadname)
		threadname = "default";

	for (;;) {
		grab_get_image(img, ctx, threadname);
		
		ret = 0;
		va_start(val, ctx);
		while ((node = va_arg(val, xmlNodePtr))) {
			ret = filter_apply(img, node);
			if (ret > 0)
				break;
		}
		va_end(val);
		
		if (ret <= 0)
			break;
		
		image_destroy(img);
		
		usleep(ret);
	}
}
