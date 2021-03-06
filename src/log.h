#ifndef _LOG_H_
#define _LOG_H_

/* $Id$ */

int log_open(void);
void log_replace_bg(int);

/* writes a prefixed (with module name and timestamp) log message to stderr */
void log_log(char *modname, char *format, ...)
	__attribute__ ((format (printf, 2, 3)));

#endif

