#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "config.h"

#define MODULE_GENERIC
#include "module.h"
#include "socket.h"

char *name = "socket";

int
socket_listen(unsigned short port, unsigned long ip)
{
	int fd;
	int ret;
	struct sockaddr_in sin;
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
		
	ret = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = ip;
	ret = bind(fd, (struct sockaddr *) &sin, sizeof(sin));
	if (ret)
	{
		close(fd);
		return -1;
	}
	ret = listen(fd, 5);
	if (ret)
	{
		close(fd);
		return -1;
	}
	
	return fd;
}

int
socket_accept(int fd, struct peer *peer)
{
	int newfd, socklen;
	struct sockaddr_in sin;
	
	socklen = sizeof(peer->sin);
	newfd = accept(fd, (struct sockaddr *) &sin, &socklen);
	if (newfd == -1)
		return -1;
	memset(peer, 0, sizeof(*peer));
	peer->fd = newfd;
	memcpy(&peer->sin, &sin, sizeof(peer->sin));
	return 0;
}

int
socket_accept_thread(int fd, struct peer *peer, void *(*func)(void *), void *arg)
{
	int newfd;
	pthread_attr_t attr;
	pthread_t tid;
	
	newfd = socket_accept(fd, peer);
	if (newfd == -1)
		return -1;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&tid, &attr, func, arg);
	pthread_attr_destroy(&attr);
	
	return 0;
}

int
socket_readline(int fd, char *buf, unsigned int bufsize)
{
	int ret;
	unsigned int count;
	
	count = 0;
	for (;;)
	{
		ret = read(fd, buf, 1);
		if (ret != 1)
			return -1;
		if (*buf == '\n')
			break;
		buf++;
		count++;
		if (count >= bufsize)
			return -1;
	}
	
	*buf-- = '\0';
	if (count >= 1 && *buf == '\r')
		*buf = '\0';
		
	return 0;
}

char *
socket_ip(struct peer *peer)
{
	snprintf(peer->tbuf, sizeof(peer->tbuf) - 1,
		"%u.%u.%u.%u",
		(peer->sin.sin_addr.s_addr >>  0) & 0xff,
		(peer->sin.sin_addr.s_addr >>  8) & 0xff,
		(peer->sin.sin_addr.s_addr >> 16) & 0xff,
		(peer->sin.sin_addr.s_addr >> 24) & 0xff);
	return peer->tbuf;
}

unsigned int
socket_port(struct peer *peer)
{
	return (unsigned int) ntohs(peer->sin.sin_port);
}

