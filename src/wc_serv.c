#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libxml/parser.h>

#include "config.h"

#include "module.h"
#include "wc_serv.h"
#include "grab.h"
#include "configfile.h"
#include "rwlock.h"
#include "jpeg.h"

char name[] = "wc_serv";
pthread_t tid;

struct wc_config wc_config;
int listen_fd;

int
init()
{
	int ret;
	
	wc_config.port = 8888;
	listen_fd = -1;
	
	ret = load_config();
	if (ret)
		return ret;
		
	ret = open_socket();
	if (ret)
	{
		printf("Failed to open listen socket: %s\n", strerror(errno));
		return ret;
	}
	
	return 0;
}

void *
thread(void *arg)
{
	struct peer *peer;
	int i;
	pthread_t tid;
	
	for (;;)
	{
		peer = malloc(sizeof(*peer));
		i = sizeof(peer->sin);
		peer->fd = accept(listen_fd, (struct sockaddr *) &peer->sin, &i);
		if (peer->fd < 0)
		{
			printf("accept() error: %s\n", strerror(errno));
			free(peer);
			sleep(1);
			continue;
		}
		/* TODO: keep a list of accepted connections? */
		pthread_create(&tid, NULL, handle_conn, peer);
	}
}

int
load_config()
{
	struct wc_config newconfig;
	xmlDocPtr doc;
	xmlNodePtr node;
	char *s;
	
	memcpy(&newconfig, &wc_config, sizeof(newconfig));
	
	rwlock_rlock(&configdoc_lock);
	doc = xmlCopyDoc(configdoc, 1);
	rwlock_runlock(&configdoc_lock);
	if (!doc)
	{
		printf("xmlCopyDoc failed\n");
		return -1;
	}
	
	node = config_find_mod_section(doc, name);
	if (!node)
	{
		printf("Config section not found\n");
		xmlFreeDoc(doc);
		return -1;
	}
	
	for (; node; node = node->next)
	{
		if (xmlStrEqual(node->name, "port"))
		{
			s = xmlNodeListGetString(doc, node->children, 1);
			newconfig.port = atoi(s);
			free(s);
			if (newconfig.port <= 0 || newconfig.port > 0xffff)
			{
				printf("Invalid port: %i\n", newconfig.port);
				goto error;
			}
		}
	}
	
	xmlFreeDoc(doc);
	memcpy(&wc_config, &newconfig, sizeof(wc_config));
	return 0;
	
error:
	xmlFreeDoc(doc);
	return -1;
}

int
open_socket()
{
	int fd;
	struct sockaddr_in sin;
	int ret;
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
		
	ret = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(wc_config.port);
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
	listen_fd = fd;
	return 0;
}

void *
handle_conn(void *arg)
{
	struct peer peer;
	int ret;
	char c;
	char buf[1024];
	struct image curimg;
	struct image jpegimg;
	static int i;
	
	memcpy(&peer, arg, sizeof(peer));
	free(arg);
	
	/* TODO: timeout */
	ret = read(peer.fd, &c, 1);
	if (ret != 1)
	{
		close(peer.fd);
		return NULL;
	}

	rwlock_rlock(&image_lock);
	copy_image(&curimg, &current_image);
	rwlock_runlock(&image_lock);
	
	jpeg_compress(&jpegimg, &curimg);

	snprintf(buf, sizeof(buf) - 1,
		"HTTP/1.0 200 OK\n"
		"Content-type: image/jpeg\n"
		"Content-Length: %i\n"
		"Connection: close\n\n",
		jpegimg.bufsize);
	ret = write(peer.fd, buf, strlen(buf));
	ret = write(peer.fd, jpegimg.buf, jpegimg.bufsize);
	
	sleep(1);
	close(peer.fd);
	free(curimg.buf);
	free(jpegimg.buf);
	
	return NULL;
}

