/***************************************************************************

    options.c

    Options file and command line management.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "mamecore.h"
#include "mame.h"
#include "fileio.h"
#include "options.h"

#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_ENTRY_NAMES		4



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _options_data options_data;
struct _options_data
{
	options_data *			next;				/* link to the next data */
	const char *			names[MAX_ENTRY_NAMES]; /* array of possible names */
	UINT32					flags;				/* flags from the entry */
	const char *			data;				/* data for this item */
	const char *			defdata;			/* default data for this item */
	const char *			description;		/* description for this item */
	void					(*callback)(const char *arg);	/* callback to be invoked when parsing */
};



/***************************************************************************
    GLOBALS
***************************************************************************/

static options_data *		datalist;
static options_data **		datalist_nextptr = &datalist;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static options_data *find_entry_data(const char *string, int is_command_line);
static void update_data(options_data *data, const char *newdata);



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    copy_string - allocate a copy of a string
-------------------------------------------------*/

static const char *copy_string(const char *start, const char *end)
{
	char *result;

	/* copy of a NULL is NULL */
	if (start == NULL)
		return NULL;

	/* if no end, figure it out */
	if (end == NULL)
		end = start + strlen(start);

	/* allocate, copy, and NULL-terminate */
	result = malloc_or_die((end - start) + 1);
	memcpy(result, start, end - start);
	result[end - start] = 0;

	return result;
}


/*-------------------------------------------------
    separate_names - separate a list of semicolon
    separated names into individual strings
-------------------------------------------------*/

static int separate_names(const char *srcstring, const char *results[], int maxentries)
{
	const char *start;
	int curentry;

	/* start with the original string and loop over entries */
	start = srcstring;
	for (curentry = 0; curentry < maxentries; curentry++)
	{
		/* find the end of this entry and copy the string */
		const char *end = strchr(start, ';');
		results[curentry] = copy_string(start, end);

		/* if we hit the end of the source, stop */
		if (end == NULL)
			break;
		start = end + 1;
	}
	return curentry;
}


/*-------------------------------------------------
    options_add_entries - add entries to the
    current options sets
-------------------------------------------------*/

void options_add_entries(const options_entry *entrylist)
{
	options_data *data;

	/* loop over entries until we hit a NULL name */
	for ( ; entrylist->name != NULL || (entrylist->flags & OPTION_HEADER); entrylist++)
	{
		/* allocate a new item */
		data = malloc_or_die(sizeof(*data));
		memset(data, 0, sizeof(*data));

		/* separate the names, copy the flags, and set the value equal to the default */
		if (entrylist->name != NULL)
			separate_names(entrylist->name, data->names, ARRAY_LENGTH(data->names));
		data->flags = entrylist->flags;
		data->data = copy_string(entrylist->defvalue, NULL);
		data->defdata = entrylist->defvalue;
		data->description = entrylist->description;

		/* fill it in and add to the end of the list */
		*datalist_nextptr = data;
		datalist_nextptr = &data->next;
	}
}


/*-------------------------------------------------
    options_set_option_callback - specifies a
    callback to be invoked when parsing options
-------------------------------------------------*/

void options_set_option_callback(const char *name, void (*callback)(const char *arg))
{
	options_data *data;

	/* find our entry */
	data = find_entry_data(name, TRUE);
	assert(data);

	data->callback = callback;
}


/*-------------------------------------------------
    options_free_entries - free all the entries
    that were added
-------------------------------------------------*/

void options_free_entries(void)
{
	/* free all of the registered data */
	while (datalist != NULL)
	{
		options_data *temp = datalist;
		int i;

		datalist = temp->next;

		/* free names and data, and finally the element itself */
		for (i = 0; i < ARRAY_LENGTH(temp->names); i++)
			if (temp->names[i] != NULL)
				free((void *)temp->names[i]);
		if (temp->data)
			free((void *)temp->data);
		free(temp);
	}

	/* reset the nextptr */
	datalist_nextptr = &datalist;
}


/*-------------------------------------------------
    options_parse_command_line - parse a series
    of command line arguments
-------------------------------------------------*/

int options_parse_command_line(int argc, char **argv)
{
	int arg;

	/* loop over commands, looking for options */
	for (arg = 1; arg < argc; arg++)
	{
		options_data *data;
		const char *optionname;
		const char *newdata;

		/* determine the entry name to search for */
		if (argv[arg][0] == '-')
			optionname = &argv[arg][1];
		else
			optionname = "";

		/* find our entry */
		data = find_entry_data(optionname, TRUE);
		if (data == NULL)
		{
			fprintf(stderr, "Error: unknown option: %s\n", argv[arg]);
			return 1;
		}
		if ((data->flags & OPTION_DEPRECATED) != 0)
			continue;

		/* get the data for this argument, special casing booleans */
		if ((data->flags & (OPTION_BOOLEAN | OPTION_COMMAND)) != 0)
			newdata = (strncmp(&argv[arg][1], "no", 2) == 0) ? "0" : "1";
		else if (optionname[0] == 0)
			newdata = argv[arg];
		else if (arg + 1 < argc)
			newdata = argv[++arg];
		else
		{
			fprintf(stderr, "Error: option %s expected a parameter\n", argv[arg]);
			return 1;
		}

		/* invoke callback, if present */
		if (data->callback)
			(*data->callback)(newdata);

		/* allocate a new copy of data for this */
		update_data(data, newdata);
	}
	return 0;
}


/*-------------------------------------------------
    options_parse_ini_file - parse a series
    of entries in an INI file
-------------------------------------------------*/

int options_parse_ini_file(mame_file *inifile)
{
	/* loop over data */
	while (mame_fgets(giant_string_buffer, GIANT_STRING_BUFFER_SIZE, inifile) != NULL)
	{
		char *optionname, *temp;
		options_data *data;

		/* find the name */
		for (optionname = giant_string_buffer; *optionname != 0; optionname++)
			if (!isspace(*optionname))
				break;

		/* skip comments */
		if (*optionname == 0 || *optionname == '#')
			continue;

		/* scan forward to find the first space */
		for (temp = optionname; *temp != 0; temp++)
			if (isspace(*temp))
				break;

		/* if we hit the end early, print a warning and continue */
		if (*temp == 0)
		{
			fprintf(stderr, "Warning: invalid line in INI: %s", giant_string_buffer);
			continue;
		}

		/* NULL-terminate */
		*temp++ = 0;

		/* find our entry */
		data = find_entry_data(optionname, FALSE);
		if (data == NULL)
		{
			fprintf(stderr, "Warning: unknown option in INI: %s\n", optionname);
			continue;
		}
		if ((data->flags & OPTION_DEPRECATED) != 0)
			continue;

		/* allocate a new copy of data for this */
		update_data(data, temp);
	}
	return 0;
}


/*-------------------------------------------------
    options_output_ini_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_ini_file(FILE *inifile)
{
	options_data *data;

	/* loop over all items */
	for (data = datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			fprintf(inifile, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated and non-command items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_COMMAND)) == 0 && data->names[0][0] != 0)
		{
			if (data->data != NULL)
				fprintf(inifile, "%-25s %s\n", data->names[0], data->data);
			else
				fprintf(inifile, "# %-23s <NULL> (not set)\n", data->names[0]);
		}
	}
}


/*-------------------------------------------------
    options_output_ini_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_help(FILE *output)
{
	options_data *data;

	/* loop over all items */
	for (data = datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			fprintf(output, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated items */
		else if ((data->flags & OPTION_DEPRECATED) == 0 && data->description != NULL)
			fprintf(output, "-%-20s%s\n", data->names[0], data->description);
	}
}


/*-------------------------------------------------
    options_get_string - return data formatted
    as a string
-------------------------------------------------*/

const char *options_get_string(const char *name, int report_error)
{
	options_data *data = find_entry_data(name, FALSE);
	assert(data != NULL);
	return (data == NULL) ? "" : data->data;
}


/*-------------------------------------------------
    options_get_bool - return data formatted as
    a boolean
-------------------------------------------------*/

int options_get_bool(const char *name, int report_error)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = FALSE;

	if (data == NULL || data->data == NULL || sscanf(data->data, "%d", &value) != 1 || value < 0 || value > 1)
	{
		if (data != NULL && data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (report_error)
			fprintf(stderr, "Illegal boolean value for %s; reverting to %d\n", (data == NULL) ? "??" : data->names[0], value);
	}
	return value;
}


/*-------------------------------------------------
    options_get_int - return data formatted as
    an integer
-------------------------------------------------*/

int options_get_int(const char *name, int report_error)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = 0;

	if (data == NULL || data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data != NULL && data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (report_error)
			fprintf(stderr, "Illegal integer value for %s; reverting to %d\n", (data == NULL) ? "??" : data->names[0], value);
	}
	return value;
}


/*-------------------------------------------------
    options_get_float - return data formatted as
    a float
-------------------------------------------------*/

float options_get_float(const char *name, int report_error)
{
	options_data *data = find_entry_data(name, FALSE);
	float value = 0;

	if (data == NULL || data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data != NULL && data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			sscanf(data->data, "%f", &value);
		}
		if (report_error)
			fprintf(stderr, "Illegal float value for %s; reverting to %f\n", (data == NULL) ? "??" : data->names[0], (double)value);
	}
	return value;
}


/*-------------------------------------------------
    options_get_int_range - return data formatted
    as an integer and clamped to within the given
    range
-------------------------------------------------*/

int options_get_int_range(const char *name, int report_error, int minval, int maxval)
{
	options_data *data = find_entry_data(name, FALSE);
	int value = 0;

	if (data == NULL || data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data != NULL && data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			value = options_get_int(name, FALSE);
		}
		if (report_error)
			fprintf(stderr, "Illegal integer value for %s; reverting to %d\n", (data == NULL) ? "??" : data->names[0], value);
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(name, data->defdata);
		value = options_get_int(name, FALSE);
		if (report_error)
			fprintf(stderr, "Invalid %s value (must be between %d and %d); reverting to %d\n", (data == NULL) ? "??" : data->names[0], minval, maxval, value);
	}

	return value;
}


/*-------------------------------------------------
    options_get_float_range - return data formatted
    as a float and clamped to within the given
    range
-------------------------------------------------*/

float options_get_float_range(const char *name, int report_error, float minval, float maxval)
{
	options_data *data = find_entry_data(name, FALSE);
	float value = 0;

	if (data == NULL || data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data != NULL && data->defdata != NULL)
		{
			options_set_string(name, data->defdata);
			value = options_get_float(name, FALSE);
		}
		if (report_error)
			fprintf(stderr, "Illegal float value for %s; reverting to %f\n", (data == NULL) ? "??" : data->names[0], (double)value);
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(name, data->defdata);
		value = options_get_float(name, FALSE);
		if (report_error)
			fprintf(stderr, "Invalid %s value (must be between %f and %f); reverting to %f\n", (data == NULL) ? "??" : data->names[0], (double)minval, (double)maxval, (double)value);
	}
	return value;
}


/*-------------------------------------------------
    options_set_string - set a string value
-------------------------------------------------*/

void options_set_string(const char *name, const char *value)
{
	options_data *data = find_entry_data(name, FALSE);
	assert(data != NULL);
	update_data(data, value);
}


/*-------------------------------------------------
    options_set_bool - set a boolean value
-------------------------------------------------*/

void options_set_bool(const char *name, int value)
{
	char temp[4];
	assert(value == TRUE || value == FALSE);
	sprintf(temp, "%d", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    options_set_int - set an integer value
-------------------------------------------------*/

void options_set_int(const char *name, int value)
{
	char temp[20];
	sprintf(temp, "%d", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    options_set_float - set a float value
-------------------------------------------------*/

void options_set_float(const char *name, float value)
{
	char temp[100];
	sprintf(temp, "%f", value);
	options_set_string(name, temp);
}


/*-------------------------------------------------
    find_entry_data - locate an entry whose name
    matches the given string
-------------------------------------------------*/

static options_data *find_entry_data(const char *string, int is_command_line)
{
	options_data *data;
	int has_no_prefix;

	/* determine up front if we should look for "no" boolean options */
	has_no_prefix = (is_command_line && strncmp(string, "no", 2) == 0);

	/* scan all entries */
	for (data = datalist; data != NULL; data = data->next)
		if (!(data->flags & OPTION_HEADER))
		{
			const char *compareme = (has_no_prefix && (data->flags & OPTION_BOOLEAN)) ? &string[2] : &string[0];
			int namenum;

			/* loop over names */
			for (namenum = 0; namenum < ARRAY_LENGTH(data->names); namenum++)
				if (data->names[namenum] != NULL && strcmp(compareme, data->names[namenum]) == 0)
					return data;
		}

	/* didn't find it at all */
	return NULL;
}


/*-------------------------------------------------
    update_data - update the data value for a
    given entry
-------------------------------------------------*/

static void update_data(options_data *data, const char *newdata)
{
	const char *dataend = newdata + strlen(newdata) - 1;
	const char *datastart = newdata;

	/* strip off leading/trailing spaces */
	while (isspace(*datastart) && datastart <= dataend)
		datastart++;
	while (isspace(*dataend) && datastart <= dataend)
		dataend--;

	/* strip off quotes */
	if (datastart != dataend && *datastart == '"' && *dataend == '"')
		datastart++, dataend--;

	/* allocate a copy of the data */
	if (data->data)
		free((void *)data->data);
	data->data = copy_string(datastart, dataend + 1);
}
