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
	unsigned char *text;
	int x, y;
	int subx, suby;
	int idx;
	unsigned char *imgptr;
	
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
	
	x = 0;
	y = img->y - 11;
	
	while (*text)
	{
		printf("letter %c\n", *text);
		idx = *text * 11;
		printf("index %i\n", idx);
		printf("fontdata %x\n", fontdata[idx]);
		for (suby = 0; suby < 11; suby++)
		{
			printf("  suby %i\n", suby);
			imgptr = img->buf + ((y + suby) * img->x + x) * 3;
			printf("  ptr diff %i\n", imgptr - img->buf);
			for (subx = 0; subx < 8; subx++)
			{
				printf("    subx %i\n", subx);
				printf("    fontdata %x\n", fontdata[idx + suby]);
				printf("    comp %x\n", (0x80 >> subx));
				if (fontdata[idx] & (0x80 >> subx))
					imgptr[0] = imgptr[1] = imgptr[2] = 0xff;
				else
					imgptr[0] = imgptr[1] = imgptr[2] = 0x00;
				imgptr += 3;
			}
			idx++;
		}
		text++;
		x += 8;
	}
	
	return 0;
}

