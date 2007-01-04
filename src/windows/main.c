//============================================================
//
//  winmain.c - Win32 main program
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// MAME headers
#include "driver.h"
#include "strconv.h"

extern int utf8_main(int argc, char **argv);



//============================================================
//  main
//============================================================

#ifdef __GNUC__
int main(int argc, char **a_argv)
#else // !__GNUC__
int _tmain(int argc, TCHAR **argv)
#endif // __GNUC__
{
	int i, rc;
	char **utf8_argv;

#ifdef __GNUC__
	TCHAR **argv;
#ifdef UNICODE
	// MinGW doesn't support wmain() directly, so we have to jump through some hoops
	extern void __wgetmainargs(int *argc, wchar_t ***wargv, wchar_t ***wenviron, int expand_wildcards, int *startupinfo);
	WCHAR **wenviron;
	int startupinfo;
	__wgetmainargs(&argc, &argv, &wenviron, 0, &startupinfo);
#else // !UNICODE
	argv = a_argv;
#endif // UNICDE
#endif // __GNUC__

	/* convert arguments to UTF-8 */
	utf8_argv = (char **) malloc_or_die(argc * sizeof(*argv));
	for (i = 0; i < argc; i++)
		utf8_argv[i] = utf8_from_tstring(argv[i]);

	/* run utf8_main */
	rc = utf8_main(argc, utf8_argv);

	/* free arguments */
	for (i = 0; i < argc; i++)
		free(utf8_argv[i]);
	free(utf8_argv);

	return rc;
}
