#ifndef _FILTER_H_
#define _FILTER_H_

#include <libxml/parser.h>

struct image;

/* Modifies an image in-place by applying all filters found under the xml tree.
 * May print error messages */
void filter_apply(struct image *, xmlNodePtr);

#endif

