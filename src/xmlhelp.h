#ifndef _XMLHELP_H_
#define _XMLHELP_H_

#include <libxml/parser.h>

/* Is the given node an <element> of the given type? */
int xml_isnode(xmlNodePtr, const char *);
/* Return the <node>text content</node> of the given node. Return NULL on error */
char *xml_getcontent(xmlNodePtr);
/* Same as above, but return the given default instead of NULL on error */
char *xml_getcontent_def(xmlNodePtr, char *);
/* Returns the content of a node as int, or the given default on error */
int xml_atoi(xmlNodePtr, int);

/* Returns the value of the given attribute of the given node */
char *xml_attrval(xmlNodePtr, char *);

/* Returns the root node of a doc, provided for compatibility */
xmlNodePtr xml_root(xmlDocPtr);

#endif

