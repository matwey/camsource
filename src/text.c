#include <stdlib.h>
#include <time.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "text.h"
#include "image.h"
#include "xmlhelp.h"
#include "log.h"
#include "mod_handle.h"

#include "font_6x11.h"

struct text_ctx
{
	unsigned char *text;
	unsigned char color[3];
	unsigned char bgcolor[3];
	int solid:1,
	    top:1,
	    right:1;
};

static int text_conf(struct text_ctx *, xmlNodePtr);
static int text_color(unsigned char *, unsigned char *);
static int text_hexdig(unsigned char);

static struct text_ctx text_gctx;

#define MODNAME "text"

char *name = MODNAME;

int
init(struct module_ctx *mctx)
{
	int ret;
	
	memset(text_gctx.color, 0xff, sizeof(text_gctx.color));
	ret = text_conf(&text_gctx, mctx->node);
	if (ret)
		return -1;
	return 0;
}

int
filter(struct image *img, xmlNodePtr node)
{
	struct text_ctx ctx;
	int x, y;
	int subx, suby;
	int idx;
	unsigned char *imgptr;
	unsigned char buf[1024], *text;
	struct tm tm;
	time_t now;
	
	memcpy(&ctx, &text_gctx, sizeof(ctx));
	idx = text_conf(&ctx, node);
	if (idx || !ctx.text)
		return -1;

	time(&now);
	localtime_r(&now, &tm);
	strftime(buf, sizeof(buf) - 1, ctx.text, &tm);
	text = buf;
	
	if (img->y < 11)
		return 0;

	if (ctx.right)
	{
		x = img->x - strlen(text) * 6 - 1;
		if (x < 0)
			x = 0;
	}
	else
		x = 0;
	
	if (ctx.top)
		y = 0;
	else
		y = img->y - 11;
	
	while (*text)
	{
		if (x + 6 > img->x)
			break;
		
		idx = *text * 11;
		for (suby = 0; suby < 11; suby++)
		{
			imgptr = img->buf + ((y + suby) * img->x + x) * 3;
			for (subx = 0; subx < 6; subx++)
			{
				if (fontdata[idx] & (0x80 >> subx))
					memcpy(imgptr, ctx.color, 3);
				else
				{
					if (ctx.solid)
						memcpy(imgptr, ctx.bgcolor, 3);
				}
				imgptr += 3;
			}
			idx++;
		}
		text++;
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
	
	if (!node)
		return 0;
	
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
				ctx->solid = 0;
			else
			{
				ctx->solid = 1;
				ret = text_color(ctx->bgcolor, text);
				if (ret)
					goto invbg;
			}
		}
		else if (xml_isnode(node, "pos") || xml_isnode(node, "position"))
		{
			text = xml_getcontent_def(node, "bl");
			while (*text)
			{
				if (*text == 'b')
					ctx->top = 0;
				else if (*text == 't')
					ctx->top = 1;
				else if (*text == 'r')
					ctx->right = 1;
				else if (*text == 'l')
					ctx->right = 0;
				text++;
			}
		}
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

