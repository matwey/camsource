#include <stdlib.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "image.h"
#include "xmlhelp.h"
#include "log.h"

#define MODNAME "motiondetect"

char *name = MODNAME;
char *version = VERSION;

static struct {
	xmlNodePtr node;
	struct image img;
} refimages[16];

int
init(struct module_ctx *mctx)
{
	return 0;
}

int
filter(struct image *img, xmlNodePtr node)
{
	int i, firstfree;
	unsigned char *curbuf, *refbuf;
	int togo;
	int diff, diffs;
	
	if (!node) {
		log_log(MODNAME, "Can't do motion detection without referenced xml node\n");
		return -1;
	}
	
	firstfree = -1;
	for (i = 0; i < (sizeof(refimages) / sizeof(*refimages)); i++) {
		if (!refimages[i].node) {
			if (firstfree < 0)
				firstfree = i;
			continue;
		}
		if (refimages[i].node == node)
			goto found;
	}
	
	if (firstfree < 0) {
		log_log(MODNAME, "Maximum number of motion detection instanced exceeded\n");
		return -1;
	}
	
	i = firstfree;
	refimages[i].node = node;
	
found:
	if (!refimages[i].img.buf) {
		image_copy(&refimages[i].img, img);
		return 1000000;
	}
	
	if (refimages[i].img.bufsize != img->bufsize) {
		log_log(MODNAME, "Motion detect buffer size mismatch!?\n");
		return -1;
	}
	
	curbuf = img->buf;
	refbuf = refimages[i].img.buf;
	togo = img->bufsize;
	diffs = 0;
	while (togo > 0) {
		diff = *curbuf - *refbuf;
		if (diff < 0)
			diff *= -1;
		if (diff > 50)
			diffs++;
		togo--;
		curbuf++;
		refbuf++;
	}
	
	log_log(MODNAME, "diffs %i\n", diffs);
	if (diffs < 200)
		return 1000000;
	
	image_destroy(&refimages[i].img);
	image_copy(&refimages[i].img, img);
	
	return 0;
}

