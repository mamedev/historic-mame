/***************************************************************************

	fileio.h

	Core file I/O interface functions and definitions.

***************************************************************************/

#ifndef FILEIO_H
#define FILEIO_H

#include "osd_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif


/* file types */
enum
{
	FILETYPE_RAW = 0,
	FILETYPE_ROM,
	FILETYPE_ROM_NOCRC,
	FILETYPE_IMAGE,
	FILETYPE_IMAGE_DIFF,
	FILETYPE_SAMPLE,
	FILETYPE_ARTWORK,
	FILETYPE_NVRAM,
	FILETYPE_HIGHSCORE,
	FILETYPE_HIGHSCORE_DB,
	FILETYPE_CONFIG,
	FILETYPE_INPUTLOG,
	FILETYPE_STATE,
	FILETYPE_MEMCARD,
	FILETYPE_SCREENSHOT,
	FILETYPE_HISTORY,
	FILETYPE_CHEAT,
	FILETYPE_LANGUAGE,
	FILETYPE_CTRLR,
	FILETYPE_INI,
	FILETYPE_end /* dummy last entry */
};


/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

typedef struct _mame_file mame_file;

int mame_faccess(const char *filename, int filetype);
mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite);
UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length);
UINT32 mame_fread_swap(mame_file *file, void *buffer, UINT32 length);
UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, UINT32 length);
#ifdef LSB_FIRST
#define mame_fread_msbfirst mame_fread_swap
#define mame_fwrite_msbfirst mame_fwrite_swap
#define mame_fread_lsbfirst mame_fread
#define mame_fwrite_lsbfirst mame_fwrite
#else
#define mame_fread_msbfirst mame_fread
#define mame_fwrite_msbfirst mame_fwrite
#define mame_fread_lsbfirst mame_fread_swap
#define mame_fwrite_lsbfirst mame_fwrite_swap
#endif
int mame_fseek(mame_file *file, INT64 offset, int whence);
void mame_fclose(mame_file *file);
int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum);
UINT64 mame_fsize(mame_file *file);
UINT32 mame_fcrc(mame_file *file);
int mame_fgetc(mame_file *file);
int mame_ungetc(int c, mame_file *file);
char *mame_fgets(char *s, int n, mame_file *file);
int mame_feof(mame_file *file);
UINT64 mame_ftell(mame_file *file);


#ifdef __cplusplus
}
#endif

#endif