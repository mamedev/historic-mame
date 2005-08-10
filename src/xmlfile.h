/***************************************************************************

    xmlfile.h

    XML file parsing code.

***************************************************************************/

#pragma once

#ifndef __XMLFILE_H__
#define __XMLFILE_H__

#include "mamecore.h"
#include "fileio.h"



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct xml_attribute_node
{
	struct xml_attribute_node *next;		/* pointer to next attribute node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
};


struct xml_data_node
{
	struct xml_data_node *next;				/* pointer to next sibling node */
	struct xml_data_node *parent;			/* pointer to parent node */
	struct xml_data_node *child;			/* pointer to first child node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
	struct xml_attribute_node *attribute;	/* pointer to array of attribute nodes */
};



/*************************************
 *
 *  Function prototypes
 *
 *************************************/

struct xml_data_node *xml_file_create(void);
struct xml_data_node *xml_file_read(mame_file *file);
void xml_file_write(struct xml_data_node *node, mame_file *file);
void xml_file_free(struct xml_data_node *node);

int xml_count_children(struct xml_data_node *node);
struct xml_data_node *xml_get_sibling(struct xml_data_node *node, const char *name);
struct xml_data_node *xml_find_matching_sibling(struct xml_data_node *node, const char *name, const char *attribute, const char *matchval);
struct xml_attribute_node *xml_get_attribute(struct xml_data_node *node, const char *attribute);
const char *xml_get_attribute_string(struct xml_data_node *node, const char *attribute, const char *defvalue);
int xml_get_attribute_int(struct xml_data_node *node, const char *attribute, int defvalue);
float xml_get_attribute_float(struct xml_data_node *node, const char *attribute, float defvalue);

struct xml_data_node *xml_add_child(struct xml_data_node *node, const char *name, const char *value);
struct xml_data_node *xml_get_or_add_child(struct xml_data_node *node, const char *name, const char *value);
struct xml_attribute_node *xml_set_attribute(struct xml_data_node *node, const char *name, const char *value);
struct xml_attribute_node *xml_set_attribute_int(struct xml_data_node *node, const char *name, int value);
struct xml_attribute_node *xml_set_attribute_float(struct xml_data_node *node, const char *name, float value);
void xml_delete_node(struct xml_data_node *node);

#endif	/* __XMLFILE_H__ */
