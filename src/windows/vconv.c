//============================================================
//
//  vconv.c - VC++ parameter conversion code
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>



//============================================================
//  TRANSLATION TABLES
//============================================================

static const char *gcc_translate[] =
{
	"-D*",				"/D*",
	"-U*",				"/U*",
	"-I*",				"/I*",
	"-o*",				"~*",
	"-include*",		"/FI*",
	"-c",				"/c~/Fo",
	"-E",				"/E",
	"-S",				"/FA~/Fa",
	"-O0",				"/Od",
	"-O3",				"/O2",
	"-g",				"/Zi",
	"-fno-strict-aliasing",	"/Oa",
	"-Werror",			"/WX",
	"-Wall",			"/Wall /W3 /wd4018 /wd4146 /wd4242 /wd4244 /wd4305 /wd4619 /wd4702 /wd4706 /wd4710 /wd4711 /wd4826",
	"-Wno-unused",		"/wd4100 /wd4101 /wd4102",
	"-W*",				"",
	"-march=pentium",	"/G5",
	"-march=athlon",	"/G7",
	"-march=pentiumpro","/G6",
	"-march=pentium4",	"/G7",
	"-march=athlon64",	"/G7",
	"-march=pentium3",	"/G6",
	"-msse2",			"/arch:SSE2",
	"-mwindows",		"",
	"-mno-cygwin",		"",
	"-std=gnu89",		"",
	NULL
};

static const char *ld_translate[] =
{
	"-l*",				"*.lib",
	"-o*",				"/out:*",
	"-Wl,-Map,*",		"/map:*",
 	"-Wl,--allow-multiple-definition", "/force:multiple",
	"-mno-cygwin",		"",
	"-s",				"",
	"-WO",				"",
	NULL
};

static const char *ar_translate[] =
{
	"-cr",				"",
	NULL
};


static const char *windres_translate[] =
{
	"-D*",				"/D*",
	"-U*",				"/U*",
	"--include-dir*",	"/I*",
	"-o*",				"/fo*",
	"-O*",				"",
	"-i*",				"*",
	NULL
};



//============================================================
//  GLOBALS
//============================================================

static char command_line[32768];



//============================================================
//  build_command_line
//============================================================

static void build_command_line(int argc, char *argv[])
{
	const char **transtable;
	const char *outstring = "";
	char *dst = command_line;
	int output_is_first = 0;
	int param;

	// if no parameters, show usage
	if (argc < 2)
	{
		fprintf(stderr, "Usage:\n  vconv {gcc|ar|ld} [param [...]]\n");
		exit(0);
	}

	// first parameter determines the type
	if (!strcmp(argv[1], "gcc"))
	{
		transtable = gcc_translate;
		dst += sprintf(dst, "cl /nologo ");
	}
	else if (!strcmp(argv[1], "windres"))
	{
		transtable = windres_translate;
		dst += sprintf(dst, "rc ");
	}
	else if (!strcmp(argv[1], "ld"))
	{
		transtable = ld_translate;
		dst += sprintf(dst, "link /nologo ");
	}
	else if (!strcmp(argv[1], "ar"))
	{
		transtable = ar_translate;
		dst += sprintf(dst, "lib /nologo ");
		outstring = "/out:";
		output_is_first = 1;
	}
	else
	{
		fprintf(stderr, "Error: unknown translation type '%s'\n", argv[1]);
		exit(-100);
	}

	// iterate over parameters
	for (param = 2; param < argc; param++)
	{
		const char *src = argv[param];
		int srclen = strlen(src);
		int i;

		// find a match
		if (src[0] == '-')
		{
			for (i = 0; transtable[i]; i += 2)
			{
				const char *compare = transtable[i];
				const char *replace;
				int j;

				// find a match
				for (j = 0; j < srclen; j++)
					if (src[j] != compare[j])
						break;

				// if we hit an asterisk, we're ok
				if (compare[j] == '*')
				{
					// if this is the end of the parameter, use the next one
					if (src[j] == 0)
						src = argv[++param];
					else
						src += j;

					// copy the replacement up to the asterisk
					replace = transtable[i+1];
					while (*replace && *replace != '*')
					{
						if (*replace == '~')
						{
							dst += sprintf(dst, "%s", outstring);
							replace++;
						}
						else
							*dst++ = *replace++;
					}

					// if we have an asterisk in the replacement, copy the rest of the source
					if (*replace == '*')
					{
						int addquote = (strchr(src, ' ') != NULL);

						if (addquote)
							*dst++ = '"';
						while (*src)
						{
							*dst++ = (*src == '/') ? '\\' : *src;
							src++;
						}
						if (addquote)
							*dst++ = '"';

						// if there's stuff after the asterisk, copy that
						replace++;
						while (*replace)
							*dst++ = *replace++;
					}

					// append a final space
					*dst++ = ' ';
					break;
				}

				// if we hit the end, we're also ok
				else if (compare[j] == 0 && j == srclen)
				{
					// copy the replacement up to the tilde
					replace = transtable[i+1];
					while (*replace && *replace != '~')
						*dst++ = *replace++;

					// if we hit a tilde, set the new output
					if (*replace == '~')
						outstring = replace + 1;

					// append a final space
					*dst++ = ' ';
					break;
				}

				// else keep looking
			}

			// warn if we didn't get a match
			if (!transtable[i])
				fprintf(stderr, "Unable to match parameter '%s'\n", src);
		}

		// otherwise, assume it's a filename and copy translating slashes
		// it can also be a Windows-specific option which is passed through unscathed
		else
		{
			int dotrans = (*src != '/');

			// if the output filename is implicitly first, append the out parameter
			if (output_is_first)
			{
				dst += sprintf(dst, "%s", outstring);
				output_is_first = 0;
			}

			// now copy the rest of the string
			while (*src)
			{
				*dst++ = (dotrans && *src == '/') ? '\\' : *src;
				src++;
			}
			*dst++ = ' ';
		}
	}

	// trim remaining spaces and NULL terminate
	while (dst > command_line && dst[-1] == ' ')
		dst--;
	*dst = 0;
}



//============================================================
//  main
//============================================================

int main(int argc, char *argv[])
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD exitcode;

	// build the new command line
	build_command_line(argc, argv);

//  printf("%s\n", command_line);

	// create the process information structures
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	// create and execute the process
	if (!CreateProcess(NULL, command_line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) ||
		pi.hProcess == INVALID_HANDLE_VALUE)
		return -101;

	// block until done and fetch the error code
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exitcode);

	// clean up the handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// return the child's error code
	return exitcode;
}
