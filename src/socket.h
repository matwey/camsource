#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>

#include "config.h"

struct peer
{
	int fd;
	struct sockaddr_in sin;
	char tbuf[32];
};

/* Opens listening socket on specified port and ip (may be 0) and returns
 * the socket's fd or -1 on error */
int socket_listen(unsigned short, unsigned long);

/* Accepts one new connection on given listening socket and fills the peer
 * struct with info. Returns -1 on error. */
int socket_accept(int, struct peer *);

/* Creates a new socket and connects it to the specified host/port. The
 * peer struct will be filled with info. On error, a negative value is
 * returned: -1 == invalid values passed; -2 == resolve of host failed;
 * -3 == other resolver error; -4 == socket creation failed; -5 ==
 * connect failed. errno may not be meaningful in all cases. */
int socket_connect(struct peer *, char *, int);

/* Printf's to a socket. Auto-closes it on error and returns -1. */
int socket_printf(struct peer *, char *, ...)
	__attribute__ ((format (printf, 2, 3)));
int socket_vprintf(struct peer *, char *, va_list);

/* Self descriptive */
void socket_close(struct peer *);

/* Fills the peer struct with info for local socket fd (getsockname) */
void socket_fill(int, struct peer *);

/* Same as above, but creates a new thread when a new connection is accepted.
 * The pointer that gets passed to the thread func should points to a
 * dynamically allocated context structure, which should contain the peer
 * structure containing the connection info. Example:
 *
 * struct ctx {
 *   struct peer;
 *   other_data;
 * };
 * struct ctx *ctx;
 * for (;;) {
 *   ctx = malloc(sizeof(*ctx));
 *   ctx->other_data = stuff;
 *   if (socket_accept_thread(fd, &ctx->peer, thread_func, ctx) == -1)
 *     break;
 * }
 *
 * Of course, if you don't need any additional data, you can omit the custom
 * struct and only use the peer struct. The thread func is responsible for
 * free'ing ctx/peer when it's done with it. */
int socket_accept_thread(int, struct peer *, void *(*)(void *), void *);

/* Reads one line from fd into buf of given size. Returns -1 on error (such as eof) */
int socket_readline(struct peer *, char *, unsigned int);

/* Returns the ip (1.2.3.4) from the peer struct. It is stored in peer->tbuf */
char *socket_ip(struct peer *);
/* Returns port number */
unsigned int socket_port(struct peer *);

#endif

