/***************************************************************************

    xmlfile.c

    XML file parsing code.

***************************************************************************/

#include "xmlfile.h"
#include "expat.h"

#define TEMP_BUFFER_SIZE		4096



/*************************************
 *
 *  Utility function for copying a
 *  string
 *
 *************************************/

static const char *copystring(const char *input)
{
	char *newstr;

	/* NULL just passes through */
	if (!input)
		return NULL;

	/* make a lower-case copy if the allocation worked */
	newstr = malloc(strlen(input) + 1);
	if (newstr)
		strcpy(newstr, input);

	return newstr;
}



/*************************************
 *
 *  Utility function for copying a
 *  string and converting to lower
 *  case
 *
 *************************************/

static const char *copystring_lower(const char *input)
{
	char *newstr;
	int i;

	/* NULL just passes through */
	if (!input)
		return NULL;

	/* make a lower-case copy if the allocation worked */
	newstr = malloc(strlen(input) + 1);
	if (newstr)
	{
		for (i = 0; input[i] != 0; i++)
			newstr[i] = tolower(input[i]);
		newstr[i] = 0;
	}

	return newstr;
}



/*************************************
 *
 *  Add a new child node
 *
 *************************************/

static struct xml_data_node *add_child(struct xml_data_node *parent, const char *name, const char *value)
{
	struct xml_data_node **pnode;
	struct xml_data_node *node;

	/* new element: create a new node */
	node = malloc(sizeof(*node));
	if (!node)
		return NULL;

	/* initialize the members */
	node->next = NULL;
	node->parent = parent;
	node->child = NULL;
	node->name = copystring_lower(name);
	if (!node->name)
	{
		free(node);
		return NULL;
	}
	node->value = copystring(value);
	if (!node->value && value)
	{
		free((void *)node->name);
		free(node);
		return NULL;
	}
	node->attribute = NULL;

	/* add us to the end of the list of siblings */
	for (pnode = &parent->child; *pnode; pnode = &(*pnode)->next) ;
	*pnode = node;

	return node;
}



/*************************************
 *
 *  Add a new attribute node
 *
 *************************************/

static struct xml_attribute_node *add_attribute(struct xml_data_node *node, const char *name, const char *value)
{
	struct xml_attribute_node *anode, **panode;

	/* allocate a new attribute node */
	anode = malloc(sizeof(*anode));
	if (!anode)
		return NULL;

	/* fill it in */
	anode->next = NULL;
	anode->name = copystring_lower(name);
	if (!anode->name)
	{
		free(anode);
		return NULL;
	}
	anode->value = copystring(value);
	if (!anode->value)
	{
		free((void *)anode->name);
		free(anode);
		return NULL;
	}

	/* add us to the end of the list of attributes */
	for (panode = &node->attribute; *panode; panode = &(*panode)->next) ;
	*panode = anode;

	return anode;
}



/*************************************
 *
 *  XML callback for a new element
 *
 *************************************/

static void xml_element_start(void *data, const XML_Char *name, const XML_Char **attributes)
{
	struct xml_data_node **curnode = data;
	struct xml_data_node *newnode;
	int attr;

	/* add a new child node to the current node */
	newnode = add_child(*curnode, name, NULL);
	if (!newnode)
		return;

	/* add all the attributes as well */
	for (attr = 0; attributes[attr]; attr += 2)
		add_attribute(newnode, attributes[attr+0], attributes[attr+1]);

	/* set us up as the current node */
	*curnode = newnode;
}



/*************************************
 *
 *  XML callback for element data
 *
 *************************************/

static void xml_data(void *data, const XML_Char *s, int len)
{
	struct xml_data_node **curnode = data;
	int oldlen = 0;
	char *newdata;

	/* if no data, skip */
	if (len == 0)
		return;

	/* determine how much data we currently have */
	if ((*curnode)->value)
		oldlen = strlen((*curnode)->value);

	/* realloc */
	newdata = realloc((void *)(*curnode)->value, oldlen + len + 1);
	if (!newdata)
		return;

	/* copy in the new data a NULL-terminate */
	memcpy(&newdata[oldlen], s, len);
	newdata[oldlen + len] = 0;
	(*curnode)->value = newdata;
}



/*************************************
 *
 *  XML callback for element end
 *
 *************************************/

static void xml_element_end(void *data, const XML_Char *name)
{
	struct xml_data_node **curnode = data;
	char *orig;

	/* strip leading/trailing spaces from the value data */
	orig = (char *)(*curnode)->value;
	if (orig)
	{
		char *start = orig;
		char *end = start + strlen(start);

		/* first strip leading spaces */
		while (*start && isspace(*start))
			start++;

		/* then strip trailing spaces */
		while (end > start && isspace(end[-1]))
			end--;

		/* if nothing left, just free it */
		if (start == end)
		{
			free(orig);
			(*curnode)->value = NULL;
		}

		/* otherwise, memmove the data */
		else
		{
			memmove(orig, start, end - start);
			orig[end - start] = 0;
		}
	}

	/* back us up a node */
	*curnode = (*curnode)->parent;
}



/*************************************
 *
 *  XML file create
 *
 *************************************/

struct xml_data_node *xml_file_create(void)
{
	struct xml_data_node *rootnode;

	/* create a root node */
	rootnode = malloc(sizeof(*rootnode));
	if (!rootnode)
		return NULL;
	memset(rootnode, 0, sizeof(*rootnode));
	return rootnode;
}



/*************************************
 *
 *  XML file read
 *
 *************************************/

struct xml_data_node *xml_file_read(mame_file *file)
{
	struct xml_data_node *rootnode, *curnode;
	XML_Parser parser;
	int done;

	/* create a root node */
	rootnode = xml_file_create();
	if (!rootnode)
		return NULL;

	/* create the XML parser */
	parser = XML_ParserCreate(NULL);
	if (!parser)
	{
		free(rootnode);
		return NULL;
	}

	/* configure the parser */
	XML_SetElementHandler(parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parser, xml_data);
	XML_SetUserData(parser, &curnode);
	curnode = rootnode;

	/* loop through the file and parse it */
	do
	{
		char tempbuf[TEMP_BUFFER_SIZE];

		/* read as much as we can */
		int bytes = mame_fread(file, tempbuf, sizeof(tempbuf));
		done = mame_feof(file);

		/* parse the data */
		if (XML_Parse(parser, tempbuf, bytes, done) == XML_STATUS_ERROR)
		{
			xml_file_free(rootnode);
			return NULL;
		}

	} while (!done);

	/* free the parser */
	XML_ParserFree(parser);

	/* return the root node */
	return rootnode;
}



/*************************************
 *
 *  Recursive XML node writer
 *
 *************************************/

static void xml_write_node_recursive(struct xml_data_node *node, int indent, mame_file *file)
{
	struct xml_attribute_node *anode;
	struct xml_data_node *child;

	/* output this tag */
	mame_fprintf(file, "%*s<%s", indent, "", node->name);

	/* output any attributes */
	for (anode = node->attribute; anode; anode = anode->next)
		mame_fprintf(file, " %s=\"%s\"", anode->name, anode->value);

	/* if there are no children and no value, end the tag here */
	if (!node->child && !node->value)
		mame_fprintf(file, " />\n");

	/* otherwise, close this tag and output more stuff */
	else
	{
		mame_fprintf(file, ">\n");

		/* if there is a value, output that here */
		if (node->value)
			mame_fprintf(file, "%*s%s\n", indent + 4, "", node->value);

		/* loop over children and output them as well */
		if (node->child)
		{
			for (child = node->child; child; child = child->next)
				xml_write_node_recursive(child, indent + 4, file);
		}

		/* write a closing tag */
		mame_fprintf(file, "%*s</%s>\n", indent, "", node->name);
	}
}



/*************************************
 *
 *  XML file write
 *
 *************************************/

void xml_file_write(struct xml_data_node *node, mame_file *file)
{
	/* ensure this is a root node */
	if (node->name)
		osd_die("xml_file_write called with a non-root node");

	/* output a simple header */
	mame_fprintf(file, "<?xml version=\"1.0\"?>\n");
	mame_fprintf(file, "<!-- This file is autogenerated; comments and unknown tags will be stripped -->\n");

	/* loop over children of the root and output */
	for (node = node->child; node; node = node->next)
		xml_write_node_recursive(node, 0, file);
}



/*************************************
 *
 *  Recursive XML node freeing
 *
 *************************************/

static void xml_free_node_recursive(struct xml_data_node *node)
{
	struct xml_attribute_node *anode, *nanode;
	struct xml_data_node *child, *nchild;

	/* free name/value */
	if (node->name)
		free((void *)node->name);
	if (node->value)
		free((void *)node->value);

	/* free attributes */
	for (anode = node->attribute; anode; anode = nanode)
	{
		/* free name/value */
		if (anode->name)
			free((void *)anode->name);
		if (anode->value)
			free((void *)anode->value);

		/* note the next node and free this node */
		nanode = anode->next;
		free(anode);
	}

	/* free the children */
	for (child = node->child; child; child = nchild)
	{
		/* note the next node and free this node */
		nchild = child->next;
		xml_free_node_recursive(child);
	}

	/* finally free ourself */
	free(node);
}



/*************************************
 *
 *  XML file free
 *
 *************************************/

void xml_file_free(struct xml_data_node *node)
{
	/* ensure this is a root node */
	if (node->name)
		osd_die("xml_file_free called with a non-root node");

	xml_free_node_recursive(node);
}



/*************************************
 *
 *  Child node counter
 *
 *************************************/

int xml_count_children(struct xml_data_node *node)
{
	int count = 0;

	/* loop over children and count */
	for (node = node->child; node; node = node->next)
		count++;
	return count;
}



/*************************************
 *
 *  Sibling node finder
 *
 *************************************/

struct xml_data_node *xml_get_sibling(struct xml_data_node *node, const char *name)
{
	/* loop over siblings and find a matching name */
	for ( ; node; node = node->next)
		if (!strcmp(node->name, name))
			return node;
	return NULL;
}



/*************************************
 *
 *  Sibling node finder via attributes
 *
 *************************************/

struct xml_data_node *xml_find_matching_sibling(struct xml_data_node *node, const char *name, const char *attribute, const char *matchval)
{
	/* loop over siblings and find a matching attribute */
	for ( ; node; node = node->next)
	{
		/* can pass NULL as a wildcard for the node name */
		if (!name || !strcmp(name, node->name))
		{
			/* find a matching attribute */
			struct xml_attribute_node *attr = xml_get_attribute(node, attribute);
			if (attr && !strcmp(attr->value, matchval))
				return node;
		}
	}
	return NULL;
}



/*************************************
 *
 *  Attribute node finder
 *
 *************************************/

struct xml_attribute_node *xml_get_attribute(struct xml_data_node *node, const char *attribute)
{
	struct xml_attribute_node *anode;

	/* loop over attributes and find a match */
	for (anode = node->attribute; anode; anode = anode->next)
		if (!strcmp(anode->name, attribute))
			return anode;
	return NULL;
}


const char *xml_get_attribute_string(struct xml_data_node *node, const char *attribute, const char *defvalue)
{
	struct xml_attribute_node *attr = xml_get_attribute(node, attribute);
	return attr ? attr->value : defvalue;
}


int xml_get_attribute_int(struct xml_data_node *node, const char *attribute, int defvalue)
{
	const char *string = xml_get_attribute_string(node, attribute, NULL);
	int value;

	if (!string)
		return defvalue;
	sscanf(string, "%d", &value);
	return value;
}


float xml_get_attribute_float(struct xml_data_node *node, const char *attribute, float defvalue)
{
	const char *string = xml_get_attribute_string(node, attribute, NULL);
	float value;

	if (!string)
		return defvalue;
	sscanf(string, "%f", &value);
	return value;
}



/*************************************
 *
 *  Add a new child node
 *
 *************************************/

struct xml_data_node *xml_add_child(struct xml_data_node *node, const char *name, const char *value)
{
	/* just a standard add child */
	return add_child(node, name, value);
}



/*************************************
 *
 *  Find a child node; if not there,
 *  add a new one
 *
 *************************************/

struct xml_data_node *xml_get_or_add_child(struct xml_data_node *node, const char *name, const char *value)
{
	struct xml_data_node *child;

	/* find the child first */
	child = xml_get_sibling(node->child, name);
	if (child)
		return child;

	/* if not found, do a standard add child */
	return add_child(node, name, value);
}



/*************************************
 *
 *  Set an attribute on a node
 *
 *************************************/

struct xml_attribute_node *xml_set_attribute(struct xml_data_node *node, const char *name, const char *value)
{
	struct xml_attribute_node *anode;

	/* first find an existing one to replace */
	anode = xml_get_attribute(node, name);

	/* if we found it, free the old value and replace it */
	if (anode)
	{
		if (anode->value)
			free((void *)anode->value);
		anode->value = copystring(value);
	}

	/* otherwise, create a new node */
	else
		anode = add_attribute(node, name, value);

	return anode;
}


struct xml_attribute_node *xml_set_attribute_int(struct xml_data_node *node, const char *name, int value)
{
	char buffer[100];
	sprintf(buffer, "%d", value);
	return xml_set_attribute(node, name, buffer);
}


struct xml_attribute_node *xml_set_attribute_float(struct xml_data_node *node, const char *name, float value)
{
	char buffer[100];
	sprintf(buffer, "%f", value);
	return xml_set_attribute(node, name, buffer);
}



/*************************************
 *
 *  Delete a node
 *
 *************************************/

void xml_delete_node(struct xml_data_node *node)
{
	struct xml_data_node **pnode;

	/* first unhook us from the list of children of our parent */
	for (pnode = &node->parent->child; *pnode; pnode = &(*pnode)->next)
		if (*pnode == node)
		{
			*pnode = node->next;
			break;
		}

	/* now free ourselves and our children */
	xml_free_node_recursive(node);
}
