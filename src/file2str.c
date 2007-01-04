/***************************************************************************

    file2str.c

    Simple file to string converter.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdio.h>
#include "mamecore.h"


/*-------------------------------------------------
    main - primary entry point
-------------------------------------------------*/

int main(int argc, char *argv[])
{
	const char *srcfile, *dstfile, *varname;
	FILE *src, *dst;
	char input[1024];
	char output[1024];

	/* needs at least three arguments */
	if (argc < 4)
	{
		fprintf(stderr,
			"Usage:\n"
			"  laytostr <source.lay> <output.h> <varname>\n"
		);
		return 0;
	}

	/* extract arguments */
	srcfile = argv[1];
	dstfile = argv[2];
	varname = argv[3];

	/* open source file */
	src = fopen(srcfile, "r");
	if (src == NULL)
	{
		fprintf(stderr, "Unable to open source file '%s'\n", srcfile);
		return 1;
	}

	/* open dest file */
	dst = fopen(dstfile, "w");
	if (dst == NULL)
	{
		fprintf(stderr, "Unable to open output file '%s'\n", dstfile);
		return 1;
	}

	/* write the initial header */
	fprintf(dst, "const char %s[] = ""\n", varname);

	/* translate line-by-line */
	while (fgets(input, sizeof(input), src) != NULL)
	{
		static const char *hex = "0123456789abcdef";
		char *d = output;
		char *s;

		/* translate each character */
		for (s = input; *s != 0; s++)
			switch (*s)
			{
				case '\t':	*d++ = '\\'; *d++ = 't';	break;
				case '\n':	*d++ = '\\'; *d++ = 'n';	break;
				case '"':	*d++ = '\\'; *d++ = '"';	break;
				default:
					if (*s < 32)
					{
						*d++ = '\\';
						*d++ = 'x';
						*d++ = hex[(UINT8)*s >> 4];
						*d++ = hex[*s & 0xf];
					}
					else
						*d++ = *s;
					break;
			}

		/* output the string */
		*d = 0;
		fprintf(dst, "\"%s\"\n", output);
	}
	fprintf(dst, ";\n");

	/* close the files */
	fclose(dst);
	fclose(src);
	return 0;
}
