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

struct text_ctx
{
	unsigned char *text;
	unsigned char color[3];
	unsigned char bgcolor[3];
	int trans:1;
};

static int text_conf(struct text_ctx *, xmlNodePtr);
static int text_color(unsigned char *, unsigned char *);
static int text_hexdig(unsigned char);

#define MODNAME "text"

char *name = MODNAME;

int
filter(struct image *img, xmlNodePtr node)
{
	struct text_ctx ctx;
	int x, y;
	int subx, suby;
	int idx;
	unsigned char *imgptr;
	
	idx = text_conf(&ctx, node);
	if (idx)
		return -1;
	
	x = 0;
	y = img->y - 11;
	
	while (*ctx.text)
	{
		idx = *ctx.text * 11;
		for (suby = 0; suby < 11; suby++)
		{
			imgptr = img->buf + ((y + suby) * img->x + x) * 3;
			for (subx = 0; subx < 6; subx++)
			{
				if (fontdata[idx] & (0x80 >> subx))
					memcpy(imgptr, ctx.color, 3);
				else
				{
					if (!ctx.trans)
						memcpy(imgptr, ctx.bgcolor, 3);
				}
				imgptr += 3;
			}
			idx++;
		}
		ctx.text++;
		x += 6;
	}
	
	return 0;
}

static
int
text_conf(struct text_ctx *ctx, xmlNodePtr node)
{
	int ret;
	unsigned char *text;
	
	memset(ctx, 0, sizeof(*ctx));
	memset(ctx->color, 0xff, sizeof(ctx->color));
	ctx->trans = 1;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "text"))
			ctx->text = xml_getcontent(node);
		else if (xml_isnode(node, "color"))
		{
			ret = text_color(ctx->color, xml_getcontent(node));
			if (ret)
			{
				log_log(MODNAME, "Invalid contents of <color> tag\n");
				return -1;
			}
		}
		else if (xml_isnode(node, "bgcolor") || xml_isnode(node, "background"))
		{
			text = xml_getcontent(node);
			if (!text)
			{
invbg:
				log_log(MODNAME, "Invalid contents of <bgcolor> tag\n");
				return -1;
			}
			if (!strcmp(text, "trans") || !strcmp(text, "transparent") || !strcmp(text, "none"))
				ctx->trans = 1;
			else
			{
				ctx->trans = 0;
				ret = text_color(ctx->bgcolor, text);
				if (ret)
					goto invbg;
			}
		}
	}
	if (!ctx->text)
	{
		log_log(MODNAME, "No <text> tag defined\n");
		return -1;
	}
	
	return 0;
}

static
int
text_color(unsigned char *buf, unsigned char *text)
{
	int idx, reth, retl;
	
	if (!text)
		return -1;
	if (*text == '#')
		text++;
	if (strlen(text) < 6)
		return -1;
	for (idx = 0; idx < 3; idx++)
	{
		reth = text_hexdig(*text++);
		retl = text_hexdig(*text++);
		if (reth < 0 || retl < 0)
			return -1;
		buf[idx] = (reth << 4) | retl;
	}
	return 0;
}

static
int
text_hexdig(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		return -1;
}

