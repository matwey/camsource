#ifndef _FTPUP_H_
#define _FTPUP_H_

struct ftpup_ctx
{
	char *host;
	int port;
	char *user, *pass;
	char *dir, *file;
	int interval;
	
	int passive:1,
	    safemode:1;
};

#endif

