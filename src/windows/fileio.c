//============================================================
//
//  fileio.c - Win32 file access functions
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <tchar.h>

// MAME headers
#include "osdepend.h"


static DWORD create_path_recursive(TCHAR *path);



//============================================================
//  TYPE DEFINITIONS
//============================================================

struct _osd_file
{
	HANDLE		handle;
	TCHAR 		filename[1];
};


//============================================================
//  error_to_mame_file_error
//============================================================

static mame_file_error error_to_mame_file_error(DWORD error)
{
	mame_file_error filerr;

	// convert a Windows error to a mame_file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}


//============================================================
//  osd_open
//============================================================

mame_file_error osd_open(const char *path, UINT32 openflags, osd_file **file, UINT64 *filesize)
{
	DWORD disposition, access, sharemode;
	const char *src;
	DWORD upper;
	TCHAR *dst;

	// allocate a file object, plus space for the converted filename
	*file = malloc_or_die(sizeof(**file) + sizeof(TCHAR) * strlen(path));

	// convert the path into something Windows compatible
	dst = (*file)->filename;
	for (src = path; *src != 0; src++)
		*dst++ = *src;//(*src == '/') ? '\\' : *src;
	*dst++ = 0;

	// select the file open modes
	if (openflags & OPEN_FLAG_WRITE)
	{
		disposition = (openflags & OPEN_FLAG_CREATE) ? CREATE_ALWAYS : OPEN_EXISTING;
		access = (openflags & OPEN_FLAG_READ) ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE;
		sharemode = 0;
	}
	else if (openflags & OPEN_FLAG_READ)
	{
		disposition = OPEN_EXISTING;
		access = GENERIC_READ;
		sharemode = FILE_SHARE_READ;
	}
	else
		fatalerror("Invalid openflags in osd_open!");

	// attempt to open the file
	(*file)->handle = CreateFile((*file)->filename, access, sharemode, NULL, disposition, 0, NULL);
	if ((*file)->handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();

		// create the path if necessary
		if (error == ERROR_PATH_NOT_FOUND && (openflags & OPEN_FLAG_CREATE))
		{
			TCHAR *pathsep = _tcsrchr((*file)->filename, '\\');
			if (pathsep != NULL)
			{
				// create the path up to the file
				*pathsep = 0;
				error = create_path_recursive((*file)->filename);
				*pathsep = '\\';

				// attempt to reopen the file
				if (error == NO_ERROR)
					(*file)->handle = CreateFile((*file)->filename, access, sharemode, NULL, disposition, 0, NULL);
			}
		}

		// if we still failed, clean up and free
		if ((*file)->handle == INVALID_HANDLE_VALUE)
		{
			free(*file);
			*file = NULL;
			return error_to_mame_file_error(error);
		}
	}

	// get the file size
	*filesize = GetFileSize((*file)->handle, &upper);
	*filesize |= (UINT64)upper << 32;
	return FILERR_NONE;
}


//============================================================
//  osd_read
//============================================================

mame_file_error osd_read(osd_file *file, void *buffer, UINT64 offset, UINT32 length, UINT32 *actual)
{
	LONG upper = offset >> 32;
	DWORD result;

	// attempt to set the file pointer
	result = SetFilePointer(file->handle, (UINT32)offset, &upper, FILE_BEGIN);
	if (result == INVALID_SET_FILE_POINTER)
	{
		DWORD error = GetLastError();
		if (error != NO_ERROR)
			return error_to_mame_file_error(error);
	}

	// then perform the read
	if (!ReadFile(file->handle, buffer, length, &result, NULL))
		return error_to_mame_file_error(GetLastError());
	if (actual != NULL)
		*actual = result;
	return FILERR_NONE;
}


//============================================================
//  osd_write
//============================================================

mame_file_error osd_write(osd_file *file, const void *buffer, UINT64 offset, UINT32 length, UINT32 *actual)
{
	LONG upper = offset >> 32;
	DWORD result;

	// attempt to set the file pointer
	result = SetFilePointer(file->handle, (UINT32)offset, &upper, FILE_BEGIN);
	if (result == INVALID_SET_FILE_POINTER)
	{
		DWORD error = GetLastError();
		if (error != NO_ERROR)
			return error_to_mame_file_error(error);
	}

	// then perform the read
	if (!WriteFile(file->handle, buffer, length, &result, NULL))
		return error_to_mame_file_error(GetLastError());
	if (actual != NULL)
		*actual = result;
	return FILERR_NONE;
}


//============================================================
//  osd_close
//============================================================

mame_file_error osd_close(osd_file *file)
{
	// close the file handle and free the file structure
	CloseHandle(file->handle);
	free(file);
	return FILERR_NONE;
}


//============================================================
//  create_path_recursive
//============================================================

static DWORD create_path_recursive(TCHAR *path)
{
	TCHAR *sep = _tcsrchr(path, '\\');
	mame_file_error filerr;

	// if there's still a separator, and it's not the root, nuke it and recurse
	if (sep != NULL && sep > path && sep[0] != ':' && sep[-1] != '\\')
	{
		*sep = 0;
		filerr = create_path_recursive(path);
		*sep = '\\';
	}

	// if the path already exists, we're done
	if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
		return NO_ERROR;

	// create the path
	if (CreateDirectory(path, NULL) == 0)
		return GetLastError();
	return NO_ERROR;
}
