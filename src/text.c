#include <stdlib.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "text.h"
#include "image.h"
#include "xmlhelp.h"
#include "log.h"

#include "font_6x11.h"

#define MODNAME "text"

char *name = MODNAME;

int
filter(struct image *img, xmlNodePtr node)
{
	struct image work;
	char *text;
	
	text = NULL;

	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "text"))
			text = xml_getcontent(node);
	}
	if (!text)
	{
		log_log(MODNAME, "No \"text\" tag defined\n");
		return -1;
	}
	
	image_dup(&work, img);
	
	image_move(img, &work);
	
	return 0;
}

