#ifndef _FILTER_H_
#define _FILTER_H_

#include <libxml/parser.h>

struct image;

void filter_apply(struct image *, xmlNodePtr);

#endif

