#include "driver.h"
#include "unzip.h"
#include <sys/stat.h>
#include <allegro.h>
#include <unistd.h>

#define MAXPATHC 20 /* at most 20 path entries */
#define MAXPATHL 256 /* at most 255 character path length */

char buf1[MAXPATHL];
char buf2[MAXPATHL];

char *rompathv[MAXPATHC];
char *samplepathv[MAXPATHC];
int rompathc;
int samplepathc;
char *cfgdir, *hidir, *inpdir;


char *alternate_name; /* for "-romdir" */

typedef enum
{
	kPlainFile,
	kZippedFile
} eFileType;

typedef struct
{
	FILE			*file;
	unsigned char	*data;
	int				offset;
	int				length;
	eFileType		type;
} FakeFileHandle;


extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);


/* helper function which decomposes a path list into a vector of paths */
int path2vector (char *path, char *buf, char **pathv)
{
	int i;
	char *token;

	/* copy the path variable, cause strtok will modify it */
	strncpy (buf, path, MAXPATHL-1);
	i = 0;
	token = strtok (buf, ";");
	while ((i < MAXPATHC) && token)
	{
		pathv[i] = token;
		i++;
		token = strtok (NULL, ";");
	}
	return i;
}

void decompose_rom_sample_path (char *rompath, char *samplepath)
{
	rompathc    = path2vector (rompath,    buf1, rompathv);
	samplepathc = path2vector (samplepath, buf2, samplepathv);
}

/*
 * file handling routines
 *
 * gamename holds the driver name, filename is only used for ROMs and samples.
 * if 'write' is not 0, the file is opened for write. Otherwise it is opened
 * for read.
 */

/*
 * check if roms/samples for a game exist at all
 * return index+1 of the path vector component on success, otherwise 0
 */
int osd_faccess(const char *newfilename, int filetype)
{
	char name[256];
	char **pathv;
	int  pathc;
	static int indx;
	static const char *filename;
	char *dirname;

	/* if filename == NULL, continue the search */
	if (newfilename != NULL)
	{
		indx = 0;
		filename = newfilename;
	}
	else
		indx++;

	if (filetype == OSD_FILETYPE_ROM)
	{
		pathv = rompathv;
		pathc = rompathc;
	}
	else if (filetype == OSD_FILETYPE_SAMPLE)
	{
		pathv = samplepathv;
		pathc = samplepathc;
	}
	else
		return 0;

	for (; indx < pathc; indx++)
	{
		dirname = pathv[indx];

		/* does such a directory (or file) exist? */
		sprintf(name,"%s/%s",dirname,filename);
		if (access(name, F_OK) == 0)
			return indx+1;

		/* try again with a .zip extension */
		sprintf(name,"%s/%s.zip", dirname, filename);
		if (access(name, F_OK) == 0)
			return indx+1;

		/* try again with a .zif extension */
		sprintf(name,"%s/%s.zif", dirname, filename);
		if (access(name, F_OK) == 0)
			return indx+1;
	}

	/* no match */
	return 0;
}

void *osd_fopen(const char *game,const char *filename,int filetype,int _write)
{
	char name[100];
	char *dirname;
	char *gamename;

	int  indx;
	struct stat stat_buffer;
	FakeFileHandle *f;

	f = (FakeFileHandle *)malloc(sizeof(FakeFileHandle));
	if (f == 0) return f;
	f->type = kPlainFile;
	f->file = 0;

	gamename = (char *)game;

	/* Support "-romdir" yuck. */
	if (alternate_name)
		gamename = alternate_name;


	switch (filetype)
	{
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_SAMPLE:

			indx = osd_faccess (gamename, filetype);

			while (indx && !f->file)
			{
				if (filetype == OSD_FILETYPE_ROM)
					dirname = rompathv[indx-1];
				else /* filetype == OSD_FILETYPE_SAMPLE */
					dirname = samplepathv[indx-1];

				sprintf(name,"%s/%s/%s",dirname,gamename,filename);
				f->file = fopen(name,_write ? "wb" : "rb");
				if (f->file == 0)
				{
					/* try with a .zip extension */
					sprintf(name,"%s/%s.zip", dirname, gamename);
					f->file = fopen(name, _write ? "wb" : "rb");
					stat(name, &stat_buffer);
					if ((stat_buffer.st_mode & S_IFDIR))
					{
						/* it's a directory */
						fclose(f->file);
						f->file = 0;
					}
					if (f->file)
					{
						if (errorlog)
							fprintf(errorlog,
									"using zip file for %s\n", filename);
						fclose(f->file);
						f->type = kZippedFile;
						if (load_zipped_file(name, filename, &f->data, &f->length))
						{
							f->data = 0;
							f->file = 0;
						}
						else
							f->offset = 0;
					}
				}
				if (f->file == 0)
				{
					/* try with a .zip directory (if ZipMagic is installed) */
					sprintf(name,"%s/%s.zip/%s",dirname,gamename,filename);
					f->file = fopen(name,_write ? "wb" : "rb");
				}
				if (f->file == 0)
				{
					/* try with a .zif directory (if ZipFolders is installed) */
					sprintf(name,"%s/%s.zif/%s",dirname,gamename,filename);
					f->file = fopen(name,_write ? "wb" : "rb");
				}

				/* check next path entry */
				if (f->file == 0)
					indx = osd_faccess (NULL, filetype);
			}
			break;
		case OSD_FILETYPE_HIGHSCORE:
			if (mame_highscore_enabled())
			{
				sprintf(name,"%s/%s.hi",hidir,gamename);
				f->file = fopen(name,_write ? "wb" : "rb");
				if (f->file == 0)
				{
					/* try with a .zip directory (if ZipMagic is installed) */
					sprintf(name,"%s.zip/%s.hi",hidir,gamename);
					f->file = fopen(name,_write ? "wb" : "rb");
				}
				if (f->file == 0)
				{
					/* try with a .zif directory (if ZipFolders is installed) */
					sprintf(name,"%s.zif/%s.hi",hidir,gamename);
					f->file = fopen(name,_write ? "wb" : "rb");
				}
			}
			break;
		case OSD_FILETYPE_CONFIG:
			sprintf(name,"%s/%s.cfg",cfgdir,gamename);
			f->file = fopen(name,_write ? "wb" : "rb");
			if (f->file == 0)
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s.zip/%s.cfg",cfgdir,gamename);
				f->file = fopen(name,_write ? "wb" : "rb");
			}
			if (f->file == 0)
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s.zif/%s.cfg",cfgdir,gamename);
				f->file = fopen(name,_write ? "wb" : "rb");
			}
			break;
		case OSD_FILETYPE_INPUTLOG:
			sprintf(name,"%s/%s.inp", inpdir, gamename);
			f->file = fopen(name,_write ? "wb" : "rb");
			break;
		default:
			break;
	}

	if (f->file == 0)
	{
		free(f);
		return 0;
	}

	return f;
}



int osd_fread(void *file,void *buffer,int length)
{
	FakeFileHandle *f = (FakeFileHandle *)file;

	switch (f->type)
	{
		case kPlainFile:
			return fread(buffer,1,length,f->file);
			break;
		case kZippedFile:
			/* reading from the uncompressed image of a zipped file */
			if (f->data)
			{
				if (length + f->offset > f->length)
					length = f->length - f->offset;
				memcpy(buffer, f->offset + f->data, length);
				f->offset += length;
				return length;
			}
			break;
	}

	return 0;
}



int osd_fwrite(void *file,const void *buffer,int length)
{
	return fwrite(buffer,1,length,((FakeFileHandle *)file)->file);
}



int osd_fseek(void *file,int offset,int whence)
{
	FakeFileHandle *f = (FakeFileHandle *)file;
	int err = 0;

	switch (f->type)
	{
		case kPlainFile:
			return fseek(((FakeFileHandle *)file)->file,offset,whence);
			break;
		case kZippedFile:
			/* seeking within the uncompressed image of a zipped file */
			switch (whence)
			{
				case SEEK_SET:
					f->offset = offset;
					break;
				case SEEK_CUR:
					f->offset += offset;
					break;
				case SEEK_END:
					f->offset = f->length + offset;
					break;
			}
			break;
	}

	return err;
}



void osd_fclose(void *file)
{
	FakeFileHandle *f = (FakeFileHandle *) file;

	switch(f->type)
	{
		case kPlainFile:
			fclose(f->file);
			break;
		case kZippedFile:
			if (f->data)
				free(f->data);
			break;
	}
	free(f);
}

/* JB 980803 */
int osd_fchecksum (const char *game, const char *filename, int *sum)
{
	char name[100];
	char *dirname;
	char *gamename;

	int  indx;
	struct stat stat_buffer;
	FILE *f = NULL;
	int found = 0;
	int err = 0;

	gamename = (char *)game;

	/* Support "-romdir" yuck. */
	if (alternate_name)
		gamename = alternate_name;

	indx = osd_faccess (gamename, OSD_FILETYPE_ROM);

	while (indx && !f && !found)
	{
		dirname = rompathv[indx-1];

		sprintf(name,"%s/%s/%s",dirname,gamename,filename);
		f = fopen(name,"rb");
		if (f == 0)
		{
			/* try with a .zip extension */
			sprintf(name,"%s/%s.zip", dirname, gamename);
			f = fopen(name, "rb");
			stat(name, &stat_buffer);
			if ((stat_buffer.st_mode & S_IFDIR))
			{
				/* it's a directory */
				fclose(f);
				f = 0;
			}
			if (f)
			{
				fclose(f);
				if (checksum_zipped_file (name, filename, sum))
					f = 0;
				else
					found = 1;
			}
		}
		if (f == 0)
		{
			/* try with a .zip directory (if ZipMagic is installed) */
			sprintf(name,"%s/%s.zip/%s",dirname,gamename,filename);
			f = fopen(name,"rb");
		}
		if (f == 0)
		{
			/* try with a .zif directory (if ZipFolders is installed) */
			sprintf(name,"%s/%s.zif/%s",dirname,gamename,filename);
			f = fopen(name,"rb");
		}

		/* check next path entry */
		if (f == 0)
			indx = osd_faccess (NULL, OSD_FILETYPE_ROM);
		else
		{
			if (!found)
			{
				int	length, rd;
				unsigned char *data = NULL;

				found = 1;

				/* determine length of file */
				err = fseek (f, 0L, SEEK_END);
				if (err==0)
				{
					length = ftell (f);
					if (length == -1L)
						err = -1;
				}
				if (err==0)
				{
					/* allocate space for entire file */
					data = malloc (length);
					if (data == NULL)
						err = -1;
				}
				if (err==0)
				{
					/* read entire file into memory */
					fseek (f, 0L, SEEK_SET);
					rd = fread (data, sizeof (unsigned char), length, f);
					if (rd != length)
						err = -1;
				}
				if (err==0)
					*sum = crc32 (0L, data, length);

				if (data)
					free (data);

				fclose (f);
			}
		}
	}

	if (err || !found)
		return -1;
	else
		return 0;
}

