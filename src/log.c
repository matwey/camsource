#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libxml/parser.h>
#include <stdarg.h>

#include "config.h"

#include "log.h"
#include "configfile.h"
#include "xmlhelp.h"

int
log_open()
{
	int fd;
	xmlNodePtr node;
	char *file, *env;
	char fullfile[1024];
	
	node = xmlDocGetRootElement(configdoc);
	if (!node)
		return -1;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "logfile"))
			goto found;	/* break */
	}
	return -1;
	
found:
	file = xml_getcontent(node);
	if (!file || !*file)
		return -1;
	
	if (file[0] == '~' && file[1] == '/')
	{
		env = getenv("HOME");
		if (!env)
		{
			printf("Logfile pathname \"%s\" invalid, no HOME environment var set\n", file);
			return -1;
		}
		snprintf(fullfile, sizeof(fullfile) - 1, "%s%s", env, file + 1);
		file = fullfile;
	}
	
	fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd < 0)
		return -1;
	return fd;
}

void
log_replace_bg(int fd)
{
	char buf[256];
	struct tm tm;
	time_t now;
	
	printf("Main init done and logfile opened.\n");
	printf("Closing stdout and going into background...\n");
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);
	if (!fork())
		if (!fork())
		{
			setsid();
			setpgrp();
			time(&now);
			localtime_r(&now, &tm);
			strftime(buf, sizeof(buf) - 1, "%b %d %Y %T", &tm);
			printf("-----\nLog file opened at %s\n", buf);
			return;
		}
	exit(0);
}

void
log_log(char *modname, char *format, ...)
{
	va_list vl;
	char buf[64];
	struct tm tm;
	time_t now;
	
	time(&now);
	localtime_r(&now, &tm);
	strftime(buf, sizeof(buf) - 1, "%b %d %Y %T", &tm);
	if (modname)
		printf("[%s / %s] ", buf, modname);
	else
		printf("[%s] ", buf);
	
	va_start(vl, format);
	vprintf(format, vl);
	va_end(vl);
}

