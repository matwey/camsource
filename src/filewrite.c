#include "config.h"

#define MODULE_THREAD
#include "module.h"
#include "filewrite.h"
#include "mod_handle.h"

char *name = "filewrite";
char *deps[] =
{
	"jpeg_comp",
	NULL
};

int
init(struct module_ctx *mctx)
{
	return 0;
}

void *
thread(void *arg)
{
	return NULL;
}

