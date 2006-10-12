/***************************************************************************

    fileio.c

    File access functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "chd.h"
#include "hash.h"
#include "unzip.h"
#include "options.h"


/***************************************************************************
    VALIDATION
***************************************************************************/

#if !defined(CRLF) || (CRLF < 1) || (CRLF > 3)
#error CRLF undefined: must be 1 (CR), 2 (LF) or 3 (CR/LF)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PLAIN_FILE				0
#define RAM_FILE				1
#define ZIPPED_FILE				2

#define FILE_BUFFER_SIZE		512

#define OPEN_FLAG_HAS_CRC		0x10000

#ifdef MAME_DEBUG
#define DEBUG_COOKIE			0xbaadf00d
#endif



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* typedef struct _mame_file mame_file -- declared in fileio.h */
struct _mame_file
{
#ifdef DEBUG_COOKIE
	UINT32		debug_cookie;
#endif
	osd_file *	file;
	zip_file *	zipfile;
	UINT8 *		data;
	UINT64		offset;
	UINT64		length;
	UINT8		eof;
	UINT8		type;
	char		hash[HASH_BUF_SIZE];
	int			back_char; /* Buffered char for unget. EOF for empty. */
	UINT64		bufferbase;
	UINT32		bufferbytes;
	UINT8		buffer[FILE_BUFFER_SIZE];
};


typedef struct _path_iterator path_iterator;
struct _path_iterator
{
	const char *base;
	const char *cur;
	int			index;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* core functions */
static void fileio_exit(running_machine *machine);

/* file open/close */
static mame_file_error fopen_internal(const char *searchpath, const char *filename, UINT32, UINT32 flags, mame_file **file);
static mame_file_error fopen_attempt_plain(char *fullname, UINT32 openflags, mame_file *file);
static mame_file_error fopen_attempt_zipped(char *fullname, const char *filename, UINT32 crc, UINT32 openflags, mame_file *file);

/* CHD callbacks */
static chd_interface_file *chd_open_cb(const char *filename, const char *mode);
static void chd_close_cb(chd_interface_file *file);
static UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chd_length_cb(chd_interface_file *file);

/* path iteration */
static int path_iterator_init(path_iterator *iterator, const char *searchpath);
static int path_iterator_get_next(path_iterator *iterator, char *buffer, int buflen);

/* misc helpers */
static UINT32 safe_buffer_copy(const void *source, UINT32 sourceoffs, UINT32 sourcelen, void *dest, UINT32 destoffs, UINT32 destlen);
static mame_file_error convert_plain_to_ram(mame_file *file);
static mame_file_error convert_zipped_to_ram(mame_file *file);



/***************************************************************************
    HARD DISK INTERFACE
***************************************************************************/

static chd_interface mame_chd_interface =
{
	chd_open_cb,
	chd_close_cb,
	chd_read_cb,
	chd_write_cb,
	chd_length_cb
};



/***************************************************************************
    CORE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    fileio_init - initialize the internal file
    I/O system; note that the OS layer is free to
    call mame_fopen before fileio_init
-------------------------------------------------*/

void fileio_init(running_machine *machine)
{
	chd_set_interface(&mame_chd_interface);
	add_exit_callback(machine, fileio_exit);
}


/*-------------------------------------------------
    fileio_exit - clean up behind ourselves
-------------------------------------------------*/

static void fileio_exit(running_machine *machine)
{
	zip_file_cache_clear();
}



/***************************************************************************
    FILE OPEN/CLOSE
***************************************************************************/

/*-------------------------------------------------
    mame_fopen - open a file for access and
    return an error code
-------------------------------------------------*/

mame_file_error mame_fopen(const char *searchpath, const char *filename, UINT32 openflags, mame_file **file)
{
	return fopen_internal(searchpath, filename, 0, openflags, file);
}


/*-------------------------------------------------
    mame_fopen_crc - open a file by name or CRC
    and return an error code
-------------------------------------------------*/

mame_file_error mame_fopen_crc(const char *searchpath, const char *filename, UINT32 crc, UINT32 openflags, mame_file **file)
{
	return fopen_internal(searchpath, filename, crc, openflags | OPEN_FLAG_HAS_CRC, file);
}


/*-------------------------------------------------
    fopen_internal - open a file
-------------------------------------------------*/

static mame_file_error fopen_internal(const char *searchpath, const char *filename, UINT32 crc, UINT32 openflags, mame_file **file)
{
	mame_file_error filerr = FILERR_NOT_FOUND;
	path_iterator iterator;
	int maxlen, pathlen;
	char *fullname;

	/* can either have a hash or open for write, but not both */
	assert(!(openflags & OPEN_FLAG_HAS_CRC) || !(openflags & OPEN_FLAG_WRITE));

	/* allocate the file itself */
	*file = malloc(sizeof(**file));
	if (*file == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* reset the file handle */
	memset(*file, 0, sizeof(**file));
	(*file)->back_char = EOF;
#ifdef DEBUG_COOKIE
	(*file)->debug_cookie = DEBUG_COOKIE;
#endif

	/* determine the maximum length of a composed filename, plus some extra space for .zip extensions */
	maxlen = strlen(filename) + 5 + path_iterator_init(&iterator, searchpath) + 1;

	/* allocate a temporary buffer to hold it */
	fullname = malloc(maxlen);
	if (fullname == NULL)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto error;
	}

	/* loop over paths */
	while ((pathlen = path_iterator_get_next(&iterator, fullname, maxlen)) != -1)
	{
		char *dest;

		/* compute the full pathname */
		dest = &fullname[pathlen];
		if (pathlen > 0)
			*dest++ = PATH_SEPARATOR[0];
		strncpy(dest, filename, &fullname[maxlen] - dest);

		/* attempt to open the file directly */
		filerr = fopen_attempt_plain(fullname, openflags, *file);
		if (filerr == FILERR_NONE)
			break;

		/* if we're opening for read-only we have other options */
		if ((openflags & (OPEN_FLAG_READ | OPEN_FLAG_WRITE)) == OPEN_FLAG_READ)
		{
			filerr = fopen_attempt_zipped(fullname, dest, crc, openflags, *file);
			if (filerr == FILERR_NONE)
				break;
		}
	}
	free(fullname);

	/* handle errors and return */
error:
	if (filerr != FILERR_NONE)
	{
		mame_fclose(*file);
		*file = NULL;
	}
	return filerr;
}


/*-------------------------------------------------
    fopen_attempt_plain - attempt to open a
    regular file, optionally creating a directory
    if we get an error
-------------------------------------------------*/

static mame_file_error fopen_attempt_plain(char *fullname, UINT32 openflags, mame_file *file)
{
	mame_file_error filerr;

	/* attempt to open with the input flags; stop if we succeed */
	filerr = osd_open(fullname, openflags, &file->file, &file->length);
	if (filerr == FILERR_NONE)
	{
		file->type = PLAIN_FILE;
		return FILERR_NONE;
	}

	/* return the error */
	return filerr;
}


/*-------------------------------------------------
    fopen_attempt_zipped - attempt to open a
    ZIPped file
-------------------------------------------------*/

static mame_file_error fopen_attempt_zipped(char *fullname, const char *filename, UINT32 crc, UINT32 openflags, mame_file *file)
{
	char *dirsep = fullname + strlen(fullname);
	zip_error ziperr;
	zip_file *zip;
	char saved[5];

	assert(filename >= fullname && filename < fullname + strlen(fullname));

	/* loop over directory parts up to the start of filename */
	while (1)
	{
		const zip_file_header *header;
		int filenamelen;

		/* find the previous path separator */
		for (dirsep--; dirsep >= filename && *dirsep != PATH_SEPARATOR[0]; dirsep--) ;

		/* if none, we're done */
		if (*dirsep != PATH_SEPARATOR[0])
			return FILERR_NOT_FOUND;

		/* truncate here and replace with .zip */
		dirsep[0] = '.';
		saved[1] = dirsep[1]; dirsep[1] = 'z';
		saved[2] = dirsep[2]; dirsep[2] = 'i';
		saved[3] = dirsep[3]; dirsep[3] = 'p';
		saved[4] = dirsep[4]; dirsep[4] = 0;

		/* attempt to open the ZIP file */
		ziperr = zip_file_open(fullname, &zip);

		/* fix the original buffer */
		dirsep[0] = PATH_SEPARATOR[0];
		dirsep[1] = saved[1];
		dirsep[2] = saved[2];
		dirsep[3] = saved[3];
		dirsep[4] = saved[4];

		/* if we didn't open this file, continue scanning */
		if (ziperr != ZIPERR_NONE)
			continue;

		/* precompute the filename length */
		filenamelen = strlen(dirsep + 1);

		/* see if we can find the file */
		for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
		{
			const char *zipfile = header->filename + header->filename_length - filenamelen;

			/* filename match first */
			if (zipfile >= header->filename && mame_stricmp(zipfile, dirsep + 1) == 0 &&
				(zipfile == header->filename || zipfile[-1] == PATH_SEPARATOR[0]))
				break;

			/* CRC match second */
			if ((openflags & OPEN_FLAG_HAS_CRC) && header->crc == crc)
				break;
		}

		/* if we got it, read the data */
		if (header != NULL)
		{
			UINT8 crcs[4];

			file->zipfile = zip;
			file->type = ZIPPED_FILE;
			file->length = header->uncompressed_length;

			/* build a hash with just the CRC */
			hash_data_clear(file->hash);
			crcs[0] = header->crc >> 24;
			crcs[1] = header->crc >> 16;
			crcs[2] = header->crc >> 8;
			crcs[3] = header->crc >> 0;
			hash_data_insert_binary_checksum(file->hash, HASH_CRC, crcs);
			return FILERR_NONE;
		}

		/* close up the ZIP file and try the next level */
		zip_file_close(zip);
	}
}


/*-------------------------------------------------
    mame_fclose - closes a file
-------------------------------------------------*/

void mame_fclose(mame_file *file)
{
#ifdef DEBUG_COOKIE
	assert(file->debug_cookie == DEBUG_COOKIE);
	file->debug_cookie = 0;
#endif

	/* close files and free memory */
	if (file->zipfile != NULL)
		zip_file_close(file->zipfile);
	if (file->file != NULL)
		osd_close(file->file);
	if (file->data != NULL)
		free(file->data);
	free(file);
}



/***************************************************************************
    FILE POSITIONING
***************************************************************************/

/*-------------------------------------------------
    mame_fseek - seek within a file
-------------------------------------------------*/

int mame_fseek(mame_file *file, INT64 offset, int whence)
{
	int err = 0;

	/* flush any buffered char */
	file->back_char = EOF;

	/* switch off the relative location */
	switch (whence)
	{
		case SEEK_SET:
			file->offset = offset;
			break;

		case SEEK_CUR:
			file->offset += offset;
			break;

		case SEEK_END:
			file->offset = file->length + offset;
			break;
	}
	return err;
}


/*-------------------------------------------------
    mame_ftell - return the current file position
-------------------------------------------------*/

UINT64 mame_ftell(mame_file *file)
{
	/* return the current offset */
	return file->offset;
}


/*-------------------------------------------------
    mame_feof - return 1 if we're at the end
    of file
-------------------------------------------------*/

int mame_feof(mame_file *file)
{
	/* check for buffered chars */
	if (file->back_char != EOF)
		return 0;

	/* if the offset == length, we're at EOF */
	return (file->offset >= file->length);
}


/*-------------------------------------------------
    mame_fsize - returns the size of a file
-------------------------------------------------*/

UINT64 mame_fsize(mame_file *file)
{
	return file->length;
}



/***************************************************************************
    FILE READ
***************************************************************************/

/*-------------------------------------------------
    mame_fread - read from a file
-------------------------------------------------*/

UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length)
{
	mame_file_error filerr;
	UINT32 bytes_read = 0;

	/* flush any buffered char */
	file->back_char = EOF;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:

			/* if we're within the buffer, consume that first */
			if (file->offset >= file->bufferbase && file->offset < file->bufferbase + file->bufferbytes)
				bytes_read += safe_buffer_copy(file->buffer, file->offset - file->bufferbase, file->bufferbytes, buffer, bytes_read, length);

			/* if we've got a small amount left, read it into the buffer first */
			if (bytes_read < length)
			{
				if (length - bytes_read < sizeof(file->buffer) / 2)
				{
					/* read as much as makes sense into the buffer */
					file->bufferbase = file->offset + bytes_read;
					file->bufferbytes = 0;
					filerr = osd_read(file->file, file->buffer, file->bufferbase, sizeof(file->buffer), &file->bufferbytes);

					/* do a bounded copy from the buffer to the destination */
					bytes_read += safe_buffer_copy(file->buffer, 0, file->bufferbytes, buffer, bytes_read, length);
				}

				/* read the remainder directly from the file */
				else
				{
					UINT32 new_bytes_read;
					filerr = osd_read(file->file, (UINT8 *)buffer + bytes_read, file->offset + bytes_read, length - bytes_read, &new_bytes_read);
					bytes_read += new_bytes_read;
				}
			}
			break;

		case ZIPPED_FILE:

			/* convert to a RAM-based file and fall through.... */
			filerr = convert_zipped_to_ram(file);
			if (filerr != FILERR_NONE)
				return 0;
			assert(file->type == RAM_FILE);

		case RAM_FILE:

			/* only process if we have data */
			if (file->data != NULL)
				bytes_read += safe_buffer_copy(file->data, (UINT32)file->offset, file->length, buffer, bytes_read, length);
			break;
	}

	/* return the number of bytes read */
	file->offset += bytes_read;
	return bytes_read;
}


/*-------------------------------------------------
    mame_fgetc - read a character from a file
-------------------------------------------------*/

int mame_fgetc(mame_file *file)
{
	UINT8 buffer;

	/* handle ungetc */
	if (file->back_char != EOF)
	{
		buffer = file->back_char;
		file->back_char = EOF;
		return buffer;
	}

	/* otherwise, fetch the next byte */
	if (mame_fread(file, &buffer, 1) == 1)
		return buffer;
	return EOF;
}


/*-------------------------------------------------
    mame_ungetc - put back a character read from
    a file
-------------------------------------------------*/

int mame_ungetc(int c, mame_file *file)
{
	file->back_char = c;
	return c;
}


/*-------------------------------------------------
    mame_fgets - read a line from a text file
-------------------------------------------------*/

char *mame_fgets(char *s, int n, mame_file *file)
{
	char *cur = s;

	/* loop while we have characters */
	while (n > 0)
	{
		int c = mame_fgetc(file);
		if (c == EOF)
			break;

		/* if there's a CR, look for an LF afterwards */
		if (c == 0x0d)
		{
			int c2 = mame_fgetc(file);
			if (c2 != 0x0a)
				mame_ungetc(c2, file);
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* if there's an LF, reinterp as a CR for consistency */
		else if (c == 0x0a)
		{
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* otherwise, pop the character in and continue */
		*cur++ = c;
		n--;
	}

	/* if we put nothing in, return NULL */
	if (cur == s)
		return NULL;

	/* otherwise, terminate */
	if (n > 0)
		*cur++ = 0;
	return s;
}



/***************************************************************************
    FILE WRITE
***************************************************************************/

/*-------------------------------------------------
    mame_fwrite - write to a file
-------------------------------------------------*/

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	UINT32 bytes_written = 0;
	mame_file_error filerr;

	/* flush any buffered char */
	file->back_char = EOF;

	/* invalidate any buffered data */
	file->bufferbytes = 0;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:

			/* write directly to the file */
			filerr = osd_write(file->file, buffer, file->offset, length, &bytes_written);
			break;

		case ZIPPED_FILE:
		case RAM_FILE:
			fatalerror("Cannot write to ZIPped or RAM-based files");
			break;
	}

	/* return the number of bytes read */
	file->offset += bytes_written;
	file->length = MAX(file->length, file->offset);
	return bytes_written;
}


/*-------------------------------------------------
    mame_fputs - write a line to a text file
-------------------------------------------------*/

int mame_fputs(mame_file *f, const char *s)
{
	char convbuf[1024];
	char *pconvbuf;

	for (pconvbuf = convbuf; *s; s++)
	{
		if (*s == '\n')
		{
			if (CRLF == 1)		/* CR only */
				*pconvbuf++ = 13;
			else if (CRLF == 2)	/* LF only */
				*pconvbuf++ = 10;
			else if (CRLF == 3)	/* CR+LF */
			{
				*pconvbuf++ = 13;
				*pconvbuf++ = 10;
			}
		}
		else
			*pconvbuf++ = *s;
	}
	*pconvbuf++ = 0;

	return mame_fwrite(f, convbuf, strlen(convbuf));
}


/*-------------------------------------------------
    mame_vfprintf - vfprintf to a text file
-------------------------------------------------*/

static int mame_vfprintf(mame_file *f, const char *fmt, va_list va)
{
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, va);
	return mame_fputs(f, buf);
}


/*-------------------------------------------------
    mame_fprintf - vfprintf to a text file
-------------------------------------------------*/

int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...)
{
	int rc;
	va_list va;
	va_start(va, fmt);
	rc = mame_vfprintf(f, fmt, va);
	va_end(va);
	return rc;
}



/***************************************************************************
    MISCELLANEOUS
***************************************************************************/

/*-------------------------------------------------
    mame_fhash - returns the hash for a file
-------------------------------------------------*/

const char *mame_fhash(mame_file *file, UINT32 functions)
{
	mame_file_error filerr = FILERR_NONE;
	UINT32 wehave;

	/* if we already have the functions we need, just return */
	wehave = hash_data_used_functions(file->hash);
	if ((wehave & functions) == functions)
		return file->hash;

	/* if the file is plain or ZIPped, convert to RAM-based */
	if (file->type == PLAIN_FILE)
		filerr = convert_plain_to_ram(file);
	else if (file->type == ZIPPED_FILE)
		filerr = convert_zipped_to_ram(file);
	if (filerr != FILERR_NONE)
		return file->hash;

	assert(file->type == RAM_FILE);
	assert(file->data != NULL);

	/* compute the hash */
	hash_compute(file->hash, file->data, file->length, wehave | functions);
	return file->hash;
}



/***************************************************************************
    CHD CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    chd_open_cb - interface for opening
    a hard disk image
-------------------------------------------------*/

chd_interface_file *chd_open_cb(const char *filename, const char *mode)
{
	mame_file_error filerr;
	mame_file *file;

	/* look for read-only drives first in the ROM path */
	if (mode[0] == 'r' && !strchr(mode, '+'))
	{
		const game_driver *drv;
		char *path;

		/* allocate a temporary buffer to hold the path */
		path = malloc_or_die(strlen(filename) + 1 + MAX_DRIVER_NAME_CHARS + 1);

		/* attempt reading up the chain through the parents */
		for (drv = Machine->gamedrv; drv != NULL; drv = driver_get_clone(drv))
		{
			sprintf(path, "%s" PATH_SEPARATOR "%s", drv->name, filename);
			filerr = mame_fopen(SEARCHPATH_IMAGE, path, OPEN_FLAG_READ, &file);
			if (filerr == FILERR_NONE)
			{
				free(path);
				return (chd_interface_file *)file;
			}
		}
		free(path);
		return NULL;
	}

	/* look for read/write drives in the diff area */
	filerr = mame_fopen(SEARCHPATH_IMAGE_DIFF, filename, OPEN_FLAG_READ | OPEN_FLAG_WRITE, &file);
	if (filerr != FILERR_NONE)
		filerr = mame_fopen(SEARCHPATH_IMAGE_DIFF, filename, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &file);
	return (filerr == FILERR_NONE) ? (chd_interface_file *)file : NULL;
}


/*-------------------------------------------------
    chd_close_cb - interface for closing
    a hard disk image
-------------------------------------------------*/

void chd_close_cb(chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}


/*-------------------------------------------------
    chd_read_cb - interface for reading
    from a hard disk image
-------------------------------------------------*/

UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_write_cb - interface for writing
    to a hard disk image
-------------------------------------------------*/

UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_length_cb - interface for getting
    the length a hard disk image
-------------------------------------------------*/

UINT64 chd_length_cb(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}



/***************************************************************************
    PATH ITERATION
***************************************************************************/

/*-------------------------------------------------
    path_iterator_init - prepare to iterate over
    a given path type
-------------------------------------------------*/

static int path_iterator_init(path_iterator *iterator, const char *searchpath)
{
	const char *cur, *base;
	int maxlen = 0;

	/* reset the structure */
	memset(iterator, 0, sizeof(*iterator));
	iterator->base = (searchpath != NULL) ? options_get_string(searchpath) : "";
	iterator->cur = iterator->base;

	/* determine the maximum path embedded here */
	base = iterator->base;
	for (cur = iterator->base; *cur != 0; cur++)
		if (*cur == ';')
		{
			maxlen = MAX(maxlen, cur - base);
			base = cur + 1;
		}

	/* account for the last path */
	maxlen = MAX(maxlen, cur - base);
	return maxlen + 1;
}


/*-------------------------------------------------
    path_iterator_get_next - get the next entry
    in a multipath sequence
-------------------------------------------------*/

static int path_iterator_get_next(path_iterator *iterator, char *buffer, int buflen)
{
	char *dest = buffer;
	const char *cur;

	/* if none left, return -1 to indicate we are done */
	if (iterator->index != 0 && *iterator->cur == 0)
		return -1;

	/* copy up to the next semicolon */
	cur = iterator->cur;
	while (*cur != 0 && *cur != ';' && buflen > 1)
	{
		*dest++ = *cur++;
		buflen--;
	}
	*dest = 0;

	/* if we ran out of buffer space, skip forward to the semicolon */
	while (*cur != 0 && *cur != ';')
		cur++;
	iterator->cur = (*cur == ';') ? (cur + 1) : cur;

	/* bump the index and return the length */
	iterator->index++;
	return dest - buffer;
}



/***************************************************************************
    MISC HELPERS
***************************************************************************/

/*-------------------------------------------------
    safe_buffer_copy - copy safely from one
    bounded buffer to another
-------------------------------------------------*/

static UINT32 safe_buffer_copy(const void *source, UINT32 sourceoffs, UINT32 sourcelen, void *dest, UINT32 destoffs, UINT32 destlen)
{
	UINT32 sourceavail = sourcelen - sourceoffs;
	UINT32 destavail = destlen - destoffs;
	UINT32 bytes_to_copy = MIN(sourceavail, destavail);
	if (bytes_to_copy > 0)
		memcpy((UINT8 *)dest + destoffs, (const UINT8 *)source + sourceoffs, bytes_to_copy);
	return bytes_to_copy;
}


/*-------------------------------------------------
    convert_plain_to_ram - convert a plain file
    to a RAM-based file
-------------------------------------------------*/

static mame_file_error convert_plain_to_ram(mame_file *file)
{
	mame_file_error filerr;
	UINT32 read_length;

	assert(file->type == PLAIN_FILE);
	assert(file->data == NULL);
	assert(file->file != NULL);

	/* allocate some memory */
	file->data = malloc(file->length);
	if (file->data == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* read the file */
	filerr = osd_read(file->file, file->data, 0, file->length, &read_length);
	if (filerr == FILERR_NONE && read_length != file->length)
		filerr = FILERR_FAILURE;
	if (filerr != FILERR_NONE)
	{
		free(file->data);
		file->data = NULL;
		return filerr;
	}

	/* convert to RAM file */
	file->type = RAM_FILE;
	osd_close(file->file);
	file->file = NULL;
	return FILERR_NONE;
}


/*-------------------------------------------------
    convert_zipped_to_ram - convert a ZIPped file
    to a RAM-based file
-------------------------------------------------*/

static mame_file_error convert_zipped_to_ram(mame_file *file)
{
	zip_error ziperr;

	assert(file->type == ZIPPED_FILE);
	assert(file->data == NULL);
	assert(file->zipfile != NULL);

	/* allocate some memory */
	file->data = malloc(file->length);
	if (file->data == NULL)
		return FILERR_OUT_OF_MEMORY;

	/* read the data into our buffer and return */
	ziperr = zip_file_decompress(file->zipfile, file->data, file->length);
	if (ziperr != ZIPERR_NONE)
	{
		free(file->data);
		file->data = NULL;
		return FILERR_FAILURE;
	}

	/* convert to RAM file */
	file->type = RAM_FILE;
	zip_file_close(file->zipfile);
	file->zipfile = NULL;
	return FILERR_NONE;
}
