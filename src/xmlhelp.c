#include <stdlib.h>
#include <libxml/parser.h>

#include "config.h"

#include "xmlhelp.h"

int
xml_isnode(xmlNodePtr node, const char *str)
{
	if (!node)
		return 0;
	if (node->type != XML_ELEMENT_NODE)
		return 0;
	if (xmlStrEqual(node->name, str))
		return 1;
	return 0;
}

char *
xml_getcontent(xmlNodePtr node)
{
	if (!node
		|| node->type != XML_ELEMENT_NODE
		|| !node->children
		|| node->children->type != XML_TEXT_NODE
		|| !node->children->content)
		return NULL;
	return node->children->content;
}

char *
xml_getcontent_def(xmlNodePtr node, char *def)
{
	char *ret;
	
	ret = xml_getcontent(node);
	if (!ret)
		return def;
	return ret;
}

int
xml_atoi(xmlNodePtr node, int def)
{
	char *ret;
	
	ret = xml_getcontent(node);
	if (ret)
		return atoi(ret);
	return def;
}

char *
xml_attrval(xmlNodePtr node, char *attr)
{
	xmlAttrPtr ap;
	
	if (!node || node->type != XML_ELEMENT_NODE)
		return NULL;
	ap = xmlHasProp(node, attr);
	if (!ap || !ap->children || !ap->children->content)
		return NULL;
	return ap->children->content;
}

