#ifndef _XMLHELP_H_
#define _XMLHELP_H_

#include <libxml/parser.h>

int xml_isnode(xmlNodePtr, const char *);
char *xml_getcontent(xmlNodePtr);
char *xml_getcontent_def(xmlNodePtr, char *);
int xml_atoi(xmlNodePtr, int);

char *xml_attrval(xmlNodePtr, char *);

#endif

