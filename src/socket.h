#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"

struct peer
{
	int fd;
	struct sockaddr_in sin;
};

/* Opens listening socket on specified port and ip (may be 0) and returns
 * the socket's fd or -1 on error */
int socket_listen(unsigned short, unsigned long);

/* Accepts one new connection on given listening socket and fills the peer
 * struct with info. Returns -1 on error. */
int socket_accept(int, struct peer *);

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
int socket_readline(int, char *, unsigned int);

/* Prints the ip (1.2.3.4) from the peer struct to stdout */
void socket_print_ip(struct peer *);
/* Same as above, but includes port (1.2.3.4:1234) */
void socket_print_ipport(struct peer *);

#endif

