#ifndef _FILEWRITE_H_
#define _FILEWRITE_H_

#include <libxml/parser.h>

struct fw_ctx
{
	char *path;
	int interval;
	int chmod;
	char *cmd;
	int seqnum;
	int dosprintf:1;
	int dontdostrftime:1;
};

#endif

