/***************************************************************************

    options.h

    Options file and command line management.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "mamecore.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define OPTION_BOOLEAN		0x0001			/* option is a boolean value */
#define OPTION_DEPRECATED	0x0002			/* option is deprecated */
#define OPTION_COMMAND		0x0004			/* option is a command */
#define OPTION_HEADER		0x0008			/* text-only header */



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef int (*options_parser)(const char *arg, const char *valid, void *resultdata);


typedef struct _options_entry options_entry;
struct _options_entry
{
	const char *		name;				/* name on the command line */
	const char *		defvalue;			/* default value of this argument */
	UINT32				flags;				/* flags to describe the option */
	const char *		description;		/* description for -showusage */
};



/***************************************************************************
    PROTOTYPES
***************************************************************************/

void options_add_entries(const options_entry *entrylist);
void options_free_entries(void);

int options_parse_command_line(int argc, char **argv);
int options_parse_xml_file(mame_file *xmlfile);
int options_parse_ini_file(mame_file *inifile);

void options_output_xml_file(FILE *xmlfile);
void options_output_ini_file(FILE *inifile);

void options_output_help(FILE *output);

const char *options_get_string(const char *name, int report_error);
int options_get_bool(const char *name, int report_error);
int options_get_int(const char *name, int report_error);
float options_get_float(const char *name, int report_error);

void options_set_string(const char *name, const char *value);
void options_set_bool(const char *name, int value);
void options_set_int(const char *name, int value);
void options_set_float(const char *name, float value);

#endif	/* __OPTIONS_H__ */
