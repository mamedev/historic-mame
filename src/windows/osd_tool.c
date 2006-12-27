/***************************************************************************

    osd_tool.c

    OS-dependent code interface for tools

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "osd_tool.h"

/* Older versions of Platform SDK don't define these */
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xffffffff
#endif

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER 0xffffffff
#endif

struct _osd_tool_dir
{
	HANDLE find;
	int is_first;
	osd_tool_dirent ent;
	WIN32_FIND_DATA data;
};



/*-------------------------------------------------
    is_physical_drive - clue to Win32 code that
    we're reading a physical drive directly
-------------------------------------------------*/

int osd_is_physical_drive(const char *file)
{
	return !_strnicmp(file, "\\\\.\\physicaldrive", 17);
}



/*-------------------------------------------------
    osd_get_physical_drive_geometry - retrieves
    geometry for physical drives
-------------------------------------------------*/

int osd_get_physical_drive_geometry(const char *filename, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps)
{
	int result = FALSE;
	DISK_GEOMETRY dg;
	DWORD bytesRead;
	HANDLE file;

	file = CreateFile(filename, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		if (DeviceIoControl(file, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg), &bytesRead, NULL))
		{
		  	*cylinders = (UINT32)dg.Cylinders.QuadPart;
		  	*heads = dg.TracksPerCylinder;
		  	*sectors = dg.SectorsPerTrack;
		  	*bps = dg.BytesPerSector;

		  	/* normalize */
		  	while (*heads > 16 && !(*heads & 1))
		  	{
		  		*heads /= 2;
		  		*cylinders *= 2;
		  	}
		  	result = TRUE;
		}
		CloseHandle(file);
	}
	return result;
}



/*-------------------------------------------------
    osd_get_file_size - returns the 64-bit file size
    for a file
-------------------------------------------------*/

UINT64 osd_get_file_size(const char *file)
{
	DWORD highSize = 0, lowSize;
	HANDLE handle;
	UINT64 filesize;

	/* attempt to open the file */
	handle = CreateFile(file, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	/* get the file size */
	lowSize = GetFileSize(handle, &highSize);
	filesize = lowSize | ((UINT64)highSize << 32);
	if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
		filesize = 0;

	/* close the file and return */
	CloseHandle(handle);
	return filesize;
}



static osd_tool_enttype get_attributes_enttype(DWORD attributes)
{
	if (attributes == 0xFFFFFFFF)
		return ENTTYPE_NONE;
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}



osd_tool_enttype osd_get_file_type(const char *filename)
{
	DWORD attributes;
	attributes = GetFileAttributes(filename);
	return get_attributes_enttype(attributes);
}



osd_tool_file *osd_tool_fopen(const char *filename, const char *mode)
{
	DWORD disposition, access, share = 0;
	HANDLE handle;

	/* convert the mode into disposition and access */
	if (!strcmp(mode, "rb"))
		disposition = OPEN_EXISTING, access = GENERIC_READ, share = FILE_SHARE_READ;
	else if (!strcmp(mode, "rb+"))
		disposition = OPEN_EXISTING, access = GENERIC_READ | GENERIC_WRITE;
	else if (!strcmp(mode, "wb"))
		disposition = CREATE_ALWAYS, access = GENERIC_WRITE;
	else if (!strcmp(mode, "wb+"))
		disposition = CREATE_ALWAYS, access = GENERIC_READ | GENERIC_WRITE;
	else
		return NULL;

	/* if this is a physical drive, we have to share */
	if (osd_is_physical_drive(filename))
	{
		disposition = OPEN_EXISTING;
		share = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}

	/* attempt to open the file */
	handle = CreateFile(filename, access, share, NULL, disposition, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return NULL;
	return (osd_tool_file *)handle;
}



void osd_tool_fclose(osd_tool_file *file)
{
	CloseHandle((HANDLE)file);
}



UINT32 osd_tool_fread(osd_tool_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	LONG upperPos = offset >> 32;
	DWORD result;

	/* attempt to seek to the new location */
	result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
	if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return 0;

	/* do the read */
	if (ReadFile((HANDLE)file, buffer, count, &result, NULL))
		return result;
	else
		return 0;
}



UINT32 osd_tool_fwrite(osd_tool_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	LONG upperPos = offset >> 32;
	DWORD result;

	/* attempt to seek to the new location */
	result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
	if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return 0;

	/* do the read */
	if (WriteFile((HANDLE)file, buffer, count, &result, NULL))
		return result;
	else
		return 0;
}



UINT64 osd_tool_flength(osd_tool_file *file)
{
	DWORD highSize = 0, lowSize;
	UINT64 filesize;

	/* get the file size */
	lowSize = GetFileSize((HANDLE)file, &highSize);
	filesize = lowSize | ((UINT64)highSize << 32);
	if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
		filesize = 0;
	return filesize;
}



osd_tool_dir *osd_tool_opendir(const char *dirname)
{
	osd_tool_dir *dir = NULL;
	char *dirfilter = NULL;

	dir = malloc(sizeof(*dir));
	if (!dir)
		goto done;
	memset(dir, 0, sizeof(*dir));
	dir->find = INVALID_HANDLE_VALUE;
	dir->is_first = TRUE;

	dirfilter = malloc(strlen(dirname) + 5);
	if (!dirfilter)
		goto done;
	sprintf(dirfilter, "%s\\*.*", dirname);

	dir->find = FindFirstFile(dirfilter, &dir->data);

done:
	if (dirfilter)
		free(dirfilter);

	if (dir && (dir->find == INVALID_HANDLE_VALUE))
	{
		free(dir);
		dir = NULL;
	}
	return dir;
}



const osd_tool_dirent *osd_tool_readdir(osd_tool_dir *dir)
{
	if (!dir->is_first)
	{
		if (!FindNextFile(dir->find, &dir->data))
			return NULL;
	}
	else
	{
		dir->is_first = FALSE;
	}
	dir->ent.name = dir->data.cFileName;
	dir->ent.type = get_attributes_enttype(dir->data.dwFileAttributes);
	dir->ent.size = dir->data.nFileSizeLow | ((UINT64) dir->data.nFileSizeHigh << 32);
	return &dir->ent;
}



void osd_tool_closedir(osd_tool_dir *dir)
{
	if (dir->find != INVALID_HANDLE_VALUE)
		CloseHandle(dir->find);
	free(dir);
}
