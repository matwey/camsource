#ifndef _FTPUP_H_
#define _FTPUP_H_

#include "socket.h"

struct ftpup_ctx
{
	/* connection context */
	char lastcmd[5];
	struct peer *peer;
	struct peer datapeer;
	
	union
	{
		struct
		{
			int listen_fd;
		}
		act;
		
		struct
		{
			char ip[32];
			int port;
		}
		pasv;
	}
	actpasv;
	
	/* config */
	char *host;
	int port;
	char *user, *pass;
	char *dir, *file;
	int interval;
	int seqnum;
	
	int passive:1;
	int safemode:1;
	int dosprintf:1;
	int dontdostrftime:1;
	int persistant:1;
};

#endif

