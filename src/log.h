#ifndef _LOG_H_
#define _LOG_H_

int log_open(void);
void log_replace_bg(int);
void log_log(char *, char *, ...)
	__attribute__ ((format (printf, 2, 3)));

#endif

