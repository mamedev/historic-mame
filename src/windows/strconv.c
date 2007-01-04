//============================================================
//
//  strconv.h - Win32 string conversion
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "restrack.h"
#include "strconv.h"



CHAR *astring_from_utf8(const char *s)
{
	int char_count;
	WCHAR *wide_string;
	CHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	wide_string = (WCHAR *) alloca(char_count * sizeof(*wide_string));
	MultiByteToWideChar(CP_UTF8, 0, s, -1, wide_string, char_count);

	// convert UTF-16 to "ANSI code page" string
	char_count = WideCharToMultiByte(CP_ACP, 0, wide_string, -1, NULL, 0, NULL, NULL);
	result = (CHAR *) malloc_or_die(char_count * sizeof(*result));
	WideCharToMultiByte(CP_ACP, 0, wide_string, -1, result, char_count, NULL, NULL);

	return result;
}



char *utf8_from_astring(const CHAR *s)
{
	int char_count;
	WCHAR *wide_string;
	CHAR *result;

	// convert "ANSI code page" string to UTF-16
	char_count = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
	wide_string = (WCHAR *) alloca(char_count * sizeof(*wide_string));
	MultiByteToWideChar(CP_ACP, 0, s, -1, wide_string, char_count);

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, NULL, 0, NULL, NULL);
	result = (char *) malloc_or_die(char_count * sizeof(*result));
	WideCharToMultiByte(CP_UTF8, 0, wide_string, -1, result, char_count, NULL, NULL);

	return result;
}



WCHAR *wstring_from_utf8(const char *s)
{
	int char_count;
	WCHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
	result = (WCHAR *) malloc_or_die(char_count * sizeof(*result));
	MultiByteToWideChar(CP_UTF8, 0, s, -1, result, char_count);

	return result;
}



char *utf8_from_wstring(const WCHAR *s)
{
	int char_count;
	char *result;

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
	result = (char *) malloc_or_die(char_count * sizeof(*result));
	WideCharToMultiByte(CP_UTF8, 0, s, -1, result, char_count, NULL, NULL);

	return result;
}
