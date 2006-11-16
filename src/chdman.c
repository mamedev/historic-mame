/***************************************************************************

    CHD compression frontend

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osd_tool.h"
#include "chdcd.h"
#include "md5.h"
#include "sha1.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>


/***************************************************************************
    CONSTANTS & DEFINES
***************************************************************************/

#define IDE_SECTOR_SIZE			512

#define ENABLE_CUSTOM_CHOMP		0



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static chd_error chdman_compress_file(chd_file *chd, const char *rawfile, UINT32 offset);
static chd_error chdman_compress_chd(chd_file *chd, chd_file *source, UINT32 totalhunks);

static chd_interface_file *chdman_open(const char *filename, const char *mode);
static void chdman_close(chd_interface_file *file);
static UINT32 chdman_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chdman_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chdman_length(chd_interface_file *file);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static chd_interface chdman_interface =
{
	chdman_open,
	chdman_close,
	chdman_read,
	chdman_write,
	chdman_length
};

static const char *error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"hunk out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file",
	"operation not supported",
	"can't find metadata",
	"invalid metadata size",
	"unsupported CHD version",
	"incomplete verify"
};

static clock_t lastprogress = 0;



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    put_bigendian_uint32 - write a UINT32 in big-endian order to memory
-------------------------------------------------*/

INLINE void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
	base[0] = value >> 24;
	base[1] = value >> 16;
	base[2] = value >> 8;
	base[3] = value;
}


/*-------------------------------------------------
    print_big_int - 64-bit int printing with commas
-------------------------------------------------*/

void print_big_int(UINT64 intvalue, char *output)
{
	int chunk;

	chunk = intvalue % 1000;
	intvalue /= 1000;
	if (intvalue != 0)
	{
		print_big_int(intvalue, output);
		strcat(output, ",");
		sprintf(&output[strlen(output)], "%03d", chunk);
	}
	else
		sprintf(&output[strlen(output)], "%d", chunk);
}


/*-------------------------------------------------
    big_int_string - return a string for a big int
-------------------------------------------------*/

char *big_int_string(UINT64 intvalue)
{
	static char buffer[256];
	buffer[0] = 0;
	print_big_int(intvalue, buffer);
	return buffer;
}


/*-------------------------------------------------
    progress - generic progress callback
-------------------------------------------------*/

static void progress(int forceit, const char *fmt, ...)
{
	clock_t curtime = clock();
	va_list arg;

	/* skip if it hasn't been long enough */
	if (!forceit && curtime - lastprogress < CLOCKS_PER_SEC / 2)
		return;
	lastprogress = curtime;

	/* standard vfprintf stuff here */
	va_start(arg, fmt);
	vprintf(fmt, arg);
	fflush(stdout);
	va_end(arg);
}


/*-------------------------------------------------
    error_string - return an error sting
-------------------------------------------------*/

static const char *error_string(int err)
{
	static char temp_buffer[100];

	if (err < sizeof(error_strings) / sizeof(error_strings[0]))
		return error_strings[err];

	sprintf(temp_buffer, "unknown error %d", err);
	return temp_buffer;
}


/*-------------------------------------------------
    usage - generic usage error display
-------------------------------------------------*/

static void usage(void)
{
	printf("usage: chdman -info input.chd\n");
	printf("   or: chdman -createraw inputhd.raw output.chd [inputoffs [hunksize]]\n");
	printf("   or: chdman -createhd inputhd.raw output.chd [inputoffs [cylinders heads sectors [sectorsize [hunksize]]]]\n");
	printf("   or: chdman -createblankhd output.chd cylinders heads sectors [sectorsize [hunksize]]\n");
	printf("   or: chdman -createcd input.toc output.chd\n");
	printf("   or: chdman -copydata input.chd output.chd\n");
	printf("   or: chdman -extract input.chd output.raw\n");
	printf("   or: chdman -extractcd input.chd output.toc output.bin\n");
	printf("   or: chdman -verify input.chd\n");
	printf("   or: chdman -verifyfix input.chd\n");
	printf("   or: chdman -update input.chd output.chd\n");
	printf("   or: chdman -chomp inout.chd output.chd maxhunk\n");
	printf("   or: chdman -merge parent.chd diff.chd output.chd\n");
	printf("   or: chdman -diff parent.chd compare.chd diff.chd\n");
	printf("   or: chdman -setchs inout.chd cylinders heads sectors\n");
	exit(1);
}


/*-------------------------------------------------
    fatalerror - error hook for assertions
-------------------------------------------------*/

void CLIB_DECL fatalerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);
	vfprintf(stderr, text, arg);
	va_end(arg);

	exit(1);
}


/*-------------------------------------------------
    guess_chs - given a file and an offset,
    compute a best guess CHS value set
-------------------------------------------------*/

static void guess_chs(const char *filename, int offset, int sectorsize, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps)
{
	UINT32 totalsecs, hds, secs;
	UINT64 filesize;

	/* if this is a direct physical drive read, handle it specially */
	if (osd_get_physical_drive_geometry(filename, cylinders, heads, sectors, bps))
		return;

	/* compute the filesize */
	filesize = osd_get_file_size(filename);
	if (filesize <= offset)
		fatalerror("Invalid file '%s'\n", filename);
	filesize -= offset;

	/* validate the size */
	if (filesize % sectorsize != 0)
		fatalerror("Can't guess CHS values because data size is not divisible by the sector size\n");
	totalsecs = filesize / sectorsize;

	/* now find a valid value */
	for (secs = 63; secs > 1; secs--)
		if (totalsecs % secs == 0)
		{
			size_t totalhds = totalsecs / secs;
			for (hds = 16; hds > 1; hds--)
				if (totalhds % hds == 0)
				{
					*cylinders = totalhds / hds;
					*heads = hds;
					*sectors = secs;
					*bps = sectorsize;
					return;
				}
		}

	/* ack, it didn't work! */
	fatalerror("Can't guess CHS values because no logical combination works!\n");
}


/*-------------------------------------------------
    do_createhd - create a new compressed hard
    disk image from a raw file
-------------------------------------------------*/

static void do_createhd(int argc, char *argv[])
{
	UINT32 cylinders, heads, sectors, sectorsize, hunksize, totalsectors, offset;
	UINT32 guess_cylinders, guess_heads, guess_sectors, guess_sectorsize;
	const char *inputfile, *outputfile;
	chd_file *chd = NULL;
	char metadata[256];
	chd_error err;

	/* require 4-5, or 8-10 args total */
	if (argc != 4 && argc != 5 && argc != 8 && argc != 9 && argc != 10)
		usage();

	/* extract the first few parameters */
	inputfile = argv[2];
	outputfile = argv[3];
	offset = (argc >= 5) ? atoi(argv[4]) : (osd_get_file_size(inputfile) % IDE_SECTOR_SIZE);

	/* if less than 8 parameters, we need to guess the CHS values */
	if (argc < 8)
		guess_chs(inputfile, offset, IDE_SECTOR_SIZE, &guess_cylinders, &guess_heads, &guess_sectors, &guess_sectorsize);

	/* parse the remaining parameters */
	cylinders = (argc >= 6) ? atoi(argv[5]) : guess_cylinders;
	heads = (argc >= 7) ? atoi(argv[6]) : guess_heads;
	sectors = (argc >= 8) ? atoi(argv[7]) : guess_sectors;
	sectorsize = (argc >= 9) ? atoi(argv[8]) : guess_sectorsize;
	hunksize = (argc >= 10) ? atoi(argv[9]) : (sectorsize > 4096) ? sectorsize : ((4096 / sectorsize) * sectorsize);
	totalsectors = cylinders * heads * sectors;

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	printf("Input offset: %d\n", offset);
	printf("Cylinders:    %d\n", cylinders);
	printf("Heads:        %d\n", heads);
	printf("Sectors:      %d\n", sectors);
	printf("Bytes/sector: %d\n", sectorsize);
	printf("Sectors/hunk: %d\n", hunksize / sectorsize);
	printf("Logical size: %s\n", big_int_string((UINT64)totalsectors * (UINT64)sectorsize));

	/* create the new hard drive */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* open the new hard drive */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* write the metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error adding hard disk metadata: %s\n", error_string(err));
		goto cleanup;
	}

	/* compress the hard drive */
	err = chdman_compress_file(chd, inputfile, offset);
	if (err != CHDERR_NONE)
	{
		printf("Error during compression: %s\n", error_string(err));
		goto cleanup;
	}

	/* success */
	chd_close(chd);
	return;

cleanup:
	if (chd != NULL)
		chd_close(chd);
	remove(outputfile);
}


/*-------------------------------------------------
    do_createraw - create a new compressed raw
    image from a raw file
-------------------------------------------------*/

static void do_createraw(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	UINT32 hunksize, offset;
	UINT64 logicalbytes;
	chd_file *chd = NULL;
	chd_error err;

	/* require 4, 5, or 6 args total */
	if (argc != 4 && argc != 5 && argc != 6)
		usage();

	/* extract the first few parameters */
	inputfile = argv[2];
	outputfile = argv[3];
	offset = (argc >= 5) ? atoi(argv[4]) : 0;
	hunksize = (argc >= 6) ? atoi(argv[5]) : 4096;
	logicalbytes = osd_get_file_size(inputfile) - offset;

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	printf("Input offset: %d\n", offset);
	printf("Bytes/hunk:   %d\n", hunksize);
	printf("Logical size: %s\n", big_int_string(logicalbytes));

	/* create the new CHD */
	err = chd_create(outputfile, logicalbytes, hunksize, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* open the new CHD */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* compress the CHD */
	err = chdman_compress_file(chd, inputfile, offset);
	if (err != CHDERR_NONE)
	{
		printf("Error during compression: %s\n", error_string(err));
		goto cleanup;
	}

	/* success */
	chd_close(chd);
	return;

cleanup:
	if (chd != NULL)
		chd_close(chd);
	remove(outputfile);
}


/*-------------------------------------------------
    do_createcd - create a new compressed CD
    image from a raw file
-------------------------------------------------*/

static void do_createcd(int argc, char *argv[])
{
	char *inputfile, *outputfile;
	chd_file *chd;
	chd_error err;
	UINT32 totalsectors = 0;
	UINT32 sectorsize = CD_FRAME_SIZE;
	UINT32 hunksize = ((CD_FRAME_SIZE * CD_FRAMES_PER_HUNK) / sectorsize) * sectorsize;
	static cdrom_toc toc;
	static cdrom_track_input_info track_info;
	int i;
	static UINT32 metadata[CD_METADATA_WORDS], *mwp;
	const chd_header *header;
	UINT32 totalhunks = 0;
	UINT8 *cache;
	double ratio = 1.0;

	/* allocate a cache */
	cache = malloc(hunksize);
	if (cache == NULL)
		return;

	/* require 4 args total */
	if (argc != 4)
		usage();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* setup the CDROM module and get the disc info */
	err = cdrom_parse_toc(inputfile, &toc, &track_info);
	if (err != CHDERR_NONE)
	{
		printf("Error reading input file: %s\n", error_string(err));
		return;
	}

	/* count up the total number of frames */
	totalsectors = 0;
	for (i = 0; i < toc.numtrks; i++)
	{
		totalsectors += toc.tracks[i].frames;
	}
	printf("\nCD-ROM %s has %d tracks and %d total frames\n", inputfile, toc.numtrks, totalsectors);

	/* pad each track to a hunk boundry.  cdrom.c will deal with this on the read side */
	for (i = 0; i < toc.numtrks; i++)
	{
		int hunks = toc.tracks[i].frames / CD_FRAMES_PER_HUNK;

		if ((toc.tracks[i].frames % CD_FRAMES_PER_HUNK) != 0)
		{
			hunks++;
			toc.tracks[i].extraframes = (hunks * CD_FRAMES_PER_HUNK) - toc.tracks[i].frames;

			// adjust the total sector count as well
			totalsectors += toc.tracks[i].extraframes;
		}
		else
		{
			toc.tracks[i].extraframes = 0;
		}

		/*
        printf("Track %02d: file %s offset %d type %d subtype %d datasize %d subsize %d frames %d extra %d\n", i,
            track_info.fname[i],
            track_info.offset[i],
            toc.tracks[i].trktype,
            toc.tracks[i].subtype,
            toc.tracks[i].datasize,
            toc.tracks[i].subsize,
            toc.tracks[i].frames,
            toc.tracks[i].extraframes);
        */
	}

	/* create the new CHD file */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		return;
	}

	/* open the new CHD file */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		remove(outputfile);
		return;
	}

	/* convert the metadata to a "portable" format */
	mwp = &metadata[0];
	put_bigendian_uint32((UINT8 *)mwp, toc.numtrks);
	mwp++;
	for (i = 0; i < CD_MAX_TRACKS; i++)
	{
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].trktype);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].subtype);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].datasize);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].subsize);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].frames);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].extraframes);
		mwp++;
	}

	/* write the metadata */
	err = chd_set_metadata(chd, CDROM_STANDARD_METADATA, 0, metadata, sizeof(metadata));
	if (err != CHDERR_NONE)
	{
		printf("Error adding CD-ROM metadata: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* begin state for writing */
	err = chd_compress_begin(chd);
	if (err != CHDERR_NONE)
	{
		printf("Error compressing: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	header = chd_get_header(chd);

	/* write each track */
	for (i = 0; i < toc.numtrks; i++)
	{
		int trkbytespersec = toc.tracks[i].datasize + toc.tracks[i].subsize;
		int hunks = (toc.tracks[i].frames + toc.tracks[i].extraframes) / CD_FRAMES_PER_HUNK;
		UINT64 sourcefileoffset = track_info.offset[i];
		chd_interface_file *srcfile;
		int curhunk;

		/* open the input file */
		srcfile = chdman_open(track_info.fname[i], "rb");
		if (srcfile == NULL)
		{
			printf("Unable to open file: %s\n", track_info.fname[i]);
			chd_close(chd);
			remove(outputfile);
			return;
		}

		printf("Compressing track %d / %d (file %s:%d, %d frames, %d hunks)\n", i+1, toc.numtrks, track_info.fname[i], track_info.offset[i], toc.tracks[i].frames, hunks);

		/* loop over hunks */
		for (curhunk = 0; curhunk < hunks; curhunk++, totalhunks++)
		{
			int i;

			progress(FALSE, "Compressing hunk %d/%d... (ratio=%d%%)  \r", totalhunks, header->totalhunks, (int)(ratio * 100));

			/* read the data.  first, zero the whole hunk */
			memset(cache, 0, hunksize);

			/* read each frame to a maximum framesize boundry, automatically padding them out */
			for (i = 0; i < CD_FRAMES_PER_HUNK; i++)
			{
				chdman_read(srcfile, sourcefileoffset, trkbytespersec, &cache[i*CD_FRAME_SIZE]);
				/*
                   NOTE: because we pad CD tracks to a hunk boundry, there is a possibility
                   that we will run off the end of the sourcefile and bytesread will be zero.
                   because we already zero out the hunk beforehand above, no special processing
                   need take place here.
                */

				sourcefileoffset += trkbytespersec;
			}

			err = chd_compress_hunk(chd, cache, &ratio);
			if (err != CHDERR_NONE)
			{
				printf("Error during compression: %s\n", error_string(err));
				chd_close(chd);
				remove(outputfile);
				return;
			}
		}

		/* close the file */
		chdman_close(srcfile);
	}

	/* cleanup */
	err = chd_compress_finish(chd);
	if (err != CHDERR_NONE)
	{
		printf("Error during compression finalization: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}


	/* success */
	chd_close(chd);
}

/*
    Create a new non-compressed hard disk image, with all hunks filled with 0s.

    Example:
        [program] -createblankhd out.hd 615 4 32 256 32768
*/
static void do_createblankhd(int argc, char *argv[])
{
	const char *outputfile;
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;
	chd_file *chd;
	char metadata[256];
	chd_error err;
	int hunknum;
	int totalhunks;
	UINT8 *cache;

	/* require 6, 7, or 8 args total */
	if (argc != 6 && argc != 7 && argc != 8)
		usage();

	/* extract the data */
	outputfile = argv[2];
	cylinders = atoi(argv[3]);
	heads = atoi(argv[4]);
	sectors = atoi(argv[5]);
	if (argc >= 7)
		sectorsize = atoi(argv[6]);
	else
		sectorsize = IDE_SECTOR_SIZE;
	if (argc >= 8)
		hunksize = atoi(argv[7]);
	else
		hunksize = (sectorsize > 4096) ? sectorsize : ((4096 / sectorsize) * sectorsize);
	totalsectors = cylinders * heads * sectors;

	/* print some info */
	printf("Output file:  %s\n", outputfile);
	printf("Cylinders:    %d\n", cylinders);
	printf("Heads:        %d\n", heads);
	printf("Sectors:      %d\n", sectors);
	printf("Bytes/sector: %d\n", sectorsize);
	printf("Sectors/hunk: %d\n", hunksize / sectorsize);
	printf("Logical size: %s\n", big_int_string((UINT64)totalsectors * (UINT64)sectorsize));

	/* create the new hard drive */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_NONE, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		return;
	}

	/* open the new hard drive */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		remove(outputfile);
		return;
	}

	/* write the metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error adding hard disk metadata: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* alloc and zero buffer*/
	cache = malloc(hunksize);
	if (! cache)
	{
		printf("Error allocating memory buffer\n");
		chd_close(chd);
		remove(outputfile);
		return;
	}
	memset(cache, 0, hunksize);

	/* Zero every hunk */
	totalhunks = (((UINT64)totalsectors * (UINT64)sectorsize) + hunksize - 1) / hunksize;
	for (hunknum = 0; hunknum < totalhunks; hunknum++)
	{
		/* progress */
		progress(FALSE, "Zeroing hunk %d/%d...  \r", hunknum, totalhunks);

		/* write out the data */
		err = chd_write(chd, hunknum, cache);
		if (err != CHDERR_NONE)
		{
			printf("Error writing CHD file: %s\n", error_string(err));
			chd_close(chd);
			remove(outputfile);
			return;
		}
	}
	progress(TRUE, "Creation complete!                    \n");

	/* free buffer */
	free(cache);

	/* success */
	chd_close(chd);
}

/*
    Compute the largest common divisor of two numbers.
*/
INLINE UINT32 lcd_u32(UINT32 a, UINT32 b)
{
	UINT32 c;

	/* We use the traditional Euclid algorithm. */
	while (b)
	{
		c = a % b;
		a = b;
		b = c;
	}

	return a;
}

/*-------------------------------------------------
    Copy all hunks of data from one CHD file to another.  The hunk sizes do not
    need to match.  If the source is shorter than the destination, the source
    data will be padded with 0s.

    Example
        [program] -copydata in.hd out.hd
-------------------------------------------------*/
static void do_copydata(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	chd_file *in_chd, *out_chd;
	UINT32 in_hunksize, out_hunksize;
	UINT32 in_totalhunks, out_totalhunks;
	UINT32 in_hunknum, out_hunknum;
	UINT8 *cache;
	UINT32 cache_data_len;
	chd_error err;

	/* require 4 args total */
	if (argc != 4)
		usage();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);

	/* open the src hard drive */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &in_chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening src CHD file: %s\n", error_string(err));
		return;
	}
	in_hunksize = chd_get_header(in_chd)->hunkbytes;
	in_totalhunks = chd_get_header(in_chd)->totalhunks;

	/* open the dest hard drive */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &out_chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening dest CHD file: %s\n", error_string(err));
		return;
	}
	out_hunksize = chd_get_header(out_chd)->hunkbytes;
	out_totalhunks = chd_get_header(out_chd)->totalhunks;

	/* alloc buffer */
	cache = malloc(in_hunksize + out_hunksize - lcd_u32(in_hunksize, out_hunksize));
	if (! cache)
	{
		printf("Error allocating memory buffer\n");
		chd_close(out_chd);
		chd_close(in_chd);
		return;
	}

	/* copy data */
	cache_data_len = 0;
	in_hunknum = 0;
	for (out_hunknum = 0; out_hunknum < out_totalhunks;)
	{
		/* progress */
		if (out_hunknum)
			progress(FALSE, "Copying hunk %d/%d...  \r", out_hunknum, out_totalhunks);

		/* read in the data */
		while (cache_data_len < out_hunksize)
		{
			if (in_hunknum < in_totalhunks)
			{	/* read data if available */
				err = chd_read(in_chd, in_hunknum, cache+cache_data_len);
				if (err != CHDERR_NONE)
				{
					printf("Error reading CHD file: %s\n", error_string(err));
					chd_close(out_chd);
					chd_close(in_chd);
					return;
				}
				in_hunknum++;
				cache_data_len += in_hunksize;
			}
			else
			{	/* if beyond EOF, just zero the buffer */
				memset(cache+cache_data_len, 0, out_hunksize-cache_data_len);
				cache_data_len = out_hunksize;
			}
		}

		/* write out the data */
		while (cache_data_len >= out_hunksize)
		{
			err = chd_write(out_chd, out_hunknum, cache);
			if (err != CHDERR_NONE)
			{
				printf("Error writing CHD file: %s\n", error_string(err));
				chd_close(out_chd);
				chd_close(in_chd);
				return;
			}
			out_hunknum++;
			cache_data_len -= out_hunksize;
			if (cache_data_len)
				memmove(cache, cache+out_hunksize, cache_data_len);
		}
	}
	progress(TRUE, "Copy complete!                    \n");

	/* free buffer */
	free(cache);

	/* success */
	chd_close(out_chd);
	chd_close(in_chd);
}

/*-------------------------------------------------
    do_extract - extract a raw file from a
    CHD image
-------------------------------------------------*/

static void do_extract(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	chd_interface_file *outfile = NULL;
	chd_file *infile = NULL;
	chd_header header;
	void *hunk = NULL;
	chd_error err;
	int hunknum;

	/* require 4 args total */
	if (argc != 4)
		usage();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);

	/* get the header */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &infile);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err));
		goto cleanup;
	}
	header = *chd_get_header(infile);

	/* allocate memory to hold a hunk */
	hunk = malloc(header.hunkbytes);
	if (!hunk)
	{
		printf("Out of memory allocating hunk buffer!\n");
		goto cleanup;
	}

	/* create the output file */
	outfile = (*chdman_interface.open)(outputfile, "wb");
	if (!outfile)
	{
		printf("Error opening output file '%s'\n", outputfile);
		goto cleanup;
	}

	/* loop over hunks, reading and writing */
	for (hunknum = 0; hunknum < header.totalhunks; hunknum++)
	{
		UINT32 byteswritten;

		/* progress */
		progress(FALSE, "Extracting hunk %d/%d...  \r", hunknum, header.totalhunks);

		/* read the hunk into a buffer */
		err = chd_read(infile, hunknum, hunk);
		if (err != CHDERR_NONE)
		{
			printf("Error reading hunk %d from CHD file: %s\n", hunknum, error_string(err));
			goto cleanup;
		}

		if ( (((UINT64)hunknum+1) * (UINT64)header.hunkbytes) < header.logicalbytes )
		{
			/* write the hunk to the file */
			byteswritten = (*chdman_interface.write)(outfile, (UINT64)hunknum * (UINT64)header.hunkbytes, header.hunkbytes, hunk);
			if (byteswritten != header.hunkbytes)
			{
				printf("Error writing hunk %d to output file: %s\n", hunknum, error_string(CHDERR_WRITE_ERROR));
				goto cleanup;
			}
		}
		else
		{
			/* write partial hunk to ensure the correct size raw image is written */
			byteswritten = (*chdman_interface.write)(outfile, (UINT64)hunknum * (UINT64)header.hunkbytes,  header.logicalbytes - ((UINT64)hunknum * (UINT64)header.hunkbytes), hunk);
			if (byteswritten != (header.logicalbytes - ((UINT64)hunknum * (UINT64)header.hunkbytes)) )
			{
				printf("Error writing hunk %d to output file: %s\n", hunknum, error_string(CHDERR_WRITE_ERROR));
				goto cleanup;
			}
		}
	}
	progress(TRUE, "Extraction complete!                    \n");

	/* close everything down */
	(*chdman_interface.close)(outfile);
	free(hunk);
	chd_close(infile);
	return;

cleanup:
	/* clean up our mess */
	if (outfile)
		(*chdman_interface.close)(outfile);
	if (hunk)
		free(hunk);
	if (infile)
		chd_close(infile);
}


/*-------------------------------------------------
    do_extractcd - extract a CDRDAO .toc/.bin
    file from a CHD-CD image
-------------------------------------------------*/

static void do_extractcd(int argc, char *argv[])
{
	const char *inputfile, *outputfile, *outputfile2;
	chd_file *infile = NULL;
	void *hunk = NULL;
	cdrom_file *cdrom = NULL;
	cdrom_toc *toc = NULL;
	FILE *outfile = NULL, *outfile2 = NULL;
	int track, m, s, f, frame;
	long trkoffs, trklen;
	static const char *typenames[8] = { "MODE1", "MODE1_RAW", "MODE2", "MODE2_FORM1", "MODE2_FORM2", "MODE2_FORM_MIX", "MODE2_RAW", "AUDIO" };
	static const char *subnames[2] = { "RW", "RW_RAW" };
	UINT8 sector[CD_MAX_SECTOR_DATA + CD_MAX_SUBCODE_DATA];
	chd_error err;

	/* require 5 args total */
	if (argc != 5)
		usage();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];
	outputfile2 = argv[4];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output files:  %s (toc) and %s (bin)\n", outputfile, outputfile2);

	/* get the header */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &infile);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err));
		goto cleanup;
	}

	/* open the CD */
	cdrom = cdrom_open(infile);
	if (!cdrom)
	{
		printf("Error opening CHD-CD '%s'\n", inputfile);
		goto cleanup;
	}

	/* get the TOC data */
	toc = cdrom_get_toc(cdrom);

	/* create the output files */
	outfile = fopen(outputfile, "w");
	if (!outfile)
	{
		printf("Error opening output file '%s'\n", outputfile);
		goto cleanup;
	}

	outfile2 = fopen(outputfile2, "wb");
	if (!outfile2)
	{
		printf("Error opening output file '%s'\n", outputfile2);
		goto cleanup;
	}

	/* process away */
	fprintf(outfile, "CD_ROM\n\n\n");

	trkoffs = 0;
	for (track = 0; track < toc->numtrks; track++)
	{
		printf("Extracting track %d\n", track+1);

		fprintf(outfile, "// Track %d\n", track+1);

		/* write out the track type */
		if (toc->tracks[track].subtype != CD_SUB_NONE)
		{
			fprintf(outfile, "TRACK %s %s\n", typenames[toc->tracks[track].trktype], subnames[toc->tracks[track].subtype]);
		}
		else
		{
			fprintf(outfile, "TRACK %s\n", typenames[toc->tracks[track].trktype]);
		}

		/* write out the attributes */
		fprintf(outfile, "NO COPY\n");
		if (toc->tracks[track].trktype == CD_TRACK_AUDIO)
		{
			fprintf(outfile, "NO PRE_EMPHASIS\n");
			fprintf(outfile, "TWO_CHANNEL_AUDIO\n");

			/* the first audio track on a mixed-track disc always has a 2 second pad */
			if (track == 1)
			{
				if (toc->tracks[track].subtype != CD_SUB_NONE)
				{
					fprintf(outfile, "ZERO AUDIO %s 00:02:00\n", subnames[toc->tracks[track].subtype]);
				}
				else
				{
					fprintf(outfile, "ZERO AUDIO 00:02:00\n");
				}
			}
		}

		/* handle the datafile */
		trklen = toc->tracks[track].frames;

		/* convert to minutes/seconds/frames */
		f = trklen;
		s = f / 75;
		f %= 75;
		m = s / 60;
		s %= 60;

		if (track > 0)
		{
			fprintf(outfile, "DATAFILE \"%s\" #%ld %02d:%02d:%02d // length in bytes: %ld\n", outputfile2, trkoffs, m, s, f, trklen*(toc->tracks[track].datasize+toc->tracks[track].subsize));
		}
		else
		{
			fprintf(outfile, "DATAFILE \"%s\" %02d:%02d:%02d // length in bytes: %ld\n", outputfile2, m, s, f, trklen*(toc->tracks[track].datasize+toc->tracks[track].subsize));
		}

		if ((toc->tracks[track].trktype == CD_TRACK_AUDIO) && (track == 1))
		{
			fprintf(outfile, "START 00:02:00\n");
		}

		// now handle the actual writeout
		for (frame = 0; frame < trklen; frame++)
		{
			cdrom_read_data(cdrom, cdrom_get_chd_start_of_track(cdrom, track)+frame, 1, sector, toc->tracks[track].trktype);
			fwrite(sector, toc->tracks[track].datasize, 1, outfile2);
			trkoffs += toc->tracks[track].datasize;
			cdrom_read_subcode(cdrom, cdrom_get_chd_start_of_track(cdrom, track)+frame, sector);
			fwrite(sector, toc->tracks[track].subsize, 1, outfile2);
			trkoffs += toc->tracks[track].subsize;
		}

		fprintf(outfile, "\n\n");
	}

	printf("Completed!\n");

	/* close everything down */
	fclose(outfile);
	fclose(outfile2);
	free(hunk);
	chd_close(infile);
	return;

cleanup:
	/* clean up our mess */
	if (outfile)
		fclose(outfile);
	if (outfile2)
		fclose(outfile2);
	if (cdrom)
		cdrom_close(cdrom);
	if (infile)
		chd_close(infile);
}


/*-------------------------------------------------
    do_verify - validate the MD5/SHA1 on a drive
    image
-------------------------------------------------*/

static void do_verify(int argc, char *argv[], int fix)
{
	UINT8 actualmd5[CHD_MD5_BYTES], actualsha1[CHD_SHA1_BYTES];
	chd_header header;
	const char *inputfile;
	chd_file *chd;
	int fixed = 0, i;
	chd_error err;

	/* require 3 args total */
	if (argc != 3)
		usage();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* open the new hard drive */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file: %s\n", error_string(err));
		return;
	}
	header = *chd_get_header(chd);

	/* verify the CHD data */
	err = chd_verify_begin(chd);
	if (err == CHDERR_NONE)
	{
		UINT32 hunknum;
		for (hunknum = 0; hunknum < header.totalhunks; hunknum++)
		{
			/* progress */
			progress(FALSE, "Verifying hunk %d/%d... \r", hunknum, header.totalhunks);

			/* verify the data */
			err = chd_verify_hunk(chd);
			if (err != CHDERR_NONE)
				break;
		}

		/* finish it */
		if (err == CHDERR_NONE)
			err = chd_verify_finish(chd, actualmd5, actualsha1);
	}

	if (err != CHDERR_NONE)
	{
		if (err == CHDERR_CANT_VERIFY)
			printf("Can't verify this type of image (probably writeable)\n");
		else
			printf("\nError during verify: %s\n", error_string(err));
		chd_close(chd);
		return;
	}

	/* verify the MD5 */
	if (!memcmp(header.md5, actualmd5, sizeof(header.md5)))
		printf("MD5 verification successful!\n");
	else
	{
		printf("Error: MD5 in header = ");
		for (i = 0; i < CHD_MD5_BYTES; i++)
			printf("%02x", header.md5[i]);
		printf("\n");
		printf("          actual MD5 = ");
		for (i = 0; i < CHD_MD5_BYTES; i++)
			printf("%02x", actualmd5[i]);
		printf("\n");

		/* fix it */
		if (fix)
		{
			memcpy(header.md5, actualmd5, sizeof(header.md5));
			fixed = 1;
		}
	}

	/* verify the SHA1 */
	if (header.version >= 3)
	{
		if (!memcmp(header.sha1, actualsha1, sizeof(header.sha1)))
			printf("SHA1 verification successful!\n");
		else
		{
			printf("Error: SHA1 in header = ");
			for (i = 0; i < CHD_SHA1_BYTES; i++)
				printf("%02x", header.sha1[i]);
			printf("\n");
			printf("          actual SHA1 = ");
			for (i = 0; i < CHD_SHA1_BYTES; i++)
				printf("%02x", actualsha1[i]);
			printf("\n");

			/* fix it */
			if (fix)
			{
				memcpy(header.sha1, actualsha1, sizeof(header.sha1));
				fixed = 1;
			}
		}
	}

	/* close the drive */
	chd_close(chd);

	/* update the header */
	if (fixed)
	{
		err = chd_set_header(inputfile, &header);
		if (err != CHDERR_NONE)
			printf("Error writing new header: %s\n", error_string(err));
		else
			printf("Updated header successfully\n");
	}
}


/*-------------------------------------------------
    do_info - dump the header information from
    a drive image
-------------------------------------------------*/

static void do_info(int argc, char *argv[])
{
	static const char *compression_type[] =
	{
		"none",
		"zlib",
		"zlib+"
	};
	chd_header header;
	const char *inputfile;
	chd_file *chd;
	chd_error err;

	/* require 3 args total */
	if (argc != 3)
		usage();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* get the header */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err));
		return;
	}
	header = *chd_get_header(chd);

	/* print the info */
	printf("Header Size:  %d bytes\n", header.length);
	printf("File Version: %d\n", header.version);
	printf("Flags:        %s, %s\n",
			(header.flags & CHDFLAGS_HAS_PARENT) ? "HAS_PARENT" : "NO_PARENT",
			(header.flags & CHDFLAGS_IS_WRITEABLE) ? "WRITEABLE" : "READ_ONLY");
	if (header.compression < 3)
		printf("Compression:  %s\n", compression_type[header.compression]);
	else
		printf("Compression:  Unknown type %d\n", header.compression);
	printf("Hunk Size:    %d bytes\n", header.hunkbytes);
	printf("Total Hunks:  %d\n", header.totalhunks);
	printf("Logical size: %s bytes\n", big_int_string(header.logicalbytes));
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE))
		printf("MD5:          %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.md5[0], header.md5[1], header.md5[2], header.md5[3],
				header.md5[4], header.md5[5], header.md5[6], header.md5[7],
				header.md5[8], header.md5[9], header.md5[10], header.md5[11],
				header.md5[12], header.md5[13], header.md5[14], header.md5[15]);
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE) && header.version >= 3)
		printf("SHA1:         %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.sha1[0], header.sha1[1], header.sha1[2], header.sha1[3],
				header.sha1[4], header.sha1[5], header.sha1[6], header.sha1[7],
				header.sha1[8], header.sha1[9], header.sha1[10], header.sha1[11],
				header.sha1[12], header.sha1[13], header.sha1[14], header.sha1[15],
				header.sha1[16], header.sha1[17], header.sha1[18], header.sha1[19]);
	if (header.flags & CHDFLAGS_HAS_PARENT)
	{
		printf("Parent MD5:   %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.parentmd5[0], header.parentmd5[1], header.parentmd5[2], header.parentmd5[3],
				header.parentmd5[4], header.parentmd5[5], header.parentmd5[6], header.parentmd5[7],
				header.parentmd5[8], header.parentmd5[9], header.parentmd5[10], header.parentmd5[11],
				header.parentmd5[12], header.parentmd5[13], header.parentmd5[14], header.parentmd5[15]);
		if (header.version >= 3)
			printf("Parent SHA1:  %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
					header.parentsha1[0], header.parentsha1[1], header.parentsha1[2], header.parentsha1[3],
					header.parentsha1[4], header.parentsha1[5], header.parentsha1[6], header.parentsha1[7],
					header.parentsha1[8], header.parentsha1[9], header.parentsha1[10], header.parentsha1[11],
					header.parentsha1[12], header.parentsha1[13], header.parentsha1[14], header.parentsha1[15],
					header.parentsha1[16], header.parentsha1[17], header.parentsha1[18], header.parentsha1[19]);
	}

	/* print out metadata */
	{
		UINT8 metadata[256];
		int i, j;

		for (i = 0; ; i++)
		{
			UINT32 metatag, metasize;

			err = chd_get_metadata(chd, CHDMETATAG_WILDCARD, i, metadata, sizeof(metadata), &metasize, &metatag);
			if (err != CHDERR_NONE)
				break;

			if (isprint((metatag >> 24) & 0xff) && isprint((metatag >> 16) & 0xff) && isprint((metatag >> 8) & 0xff) && isprint(metatag & 0xff))
				printf("Metadata:     Tag='%c%c%c%c'    Length=%d\n", (metatag >> 24) & 0xff, (metatag >> 16) & 0xff, (metatag >> 8) & 0xff, metatag & 0xff, metasize);
			else
				printf("Metadata:     Tag=%08x  Length=%d\n", metatag, metasize);
			printf("              ");

			if (metasize > 60)
				metasize = 60;
			for (j = 0; j < metasize; j++)
				printf("%c", isprint(metadata[j]) ? metadata[j] : '.');
			printf("\n");
		}
	}

	chd_close(chd);
}


/*-------------------------------------------------
    handle_custom_chomp - custom chomp a file
-------------------------------------------------*/

#if ENABLE_CUSTOM_CHOMP
static int handle_custom_chomp(const char *name, chd_file *chd, UINT32 *maxhunk)
{
	const chd_header *header = chd_get_header(chd);
	int sectors_per_hunk = (header->hunkbytes / IDE_SECTOR_SIZE);
	UINT8 *temp = malloc(header->hunkbytes);
	if (!temp)
		return CHDERR_OUT_OF_MEMORY;

	/* check for midway */
	if (!strcmp(name, "midway"))
	{
		UINT32 maxsector = 0;
		UINT32 numparts;
		chd_error err;
		int i;

		err = chd_read(chd, 0, temp);
		if (err != CHDERR_NONE)
			goto cleanup;
		if (temp[0] != 0x54 || temp[1] != 0x52 || temp[2] != 0x41 || temp[3] != 0x50)
			goto cleanup;
		numparts = temp[4] | (temp[5] << 8) | (temp[6] << 16) | (temp[7] << 24);
		printf("%d partitions\n", numparts);
		for (i = 0; i < numparts; i++)
		{
			UINT32 pstart = temp[i*12 + 8] | (temp[i*12 + 9] << 8) | (temp[i*12 + 10] << 16) | (temp[i*12 + 11] << 24);
			UINT32 psize  = temp[i*12 + 12] | (temp[i*12 + 13] << 8) | (temp[i*12 + 14] << 16) | (temp[i*12 + 15] << 24);
			UINT32 pflags = temp[i*12 + 16] | (temp[i*12 + 17] << 8) | (temp[i*12 + 18] << 16) | (temp[i*12 + 19] << 24);
			printf("  %2d. %7d - %7d (%X)\n", i, pstart, pstart + psize - 1, pflags);
			if (i != 0 && pstart + psize > maxsector)
				maxsector = pstart + psize;
		}
		*maxhunk = (maxsector + sectors_per_hunk - 1) / sectors_per_hunk;
		printf("Maximum hunk: %d\n", *maxhunk);
		if (*maxhunk >= header->totalhunks)
		{
			printf("Warning: chomp will have no effect\n");
			*maxhunk = header->totalhunks;
		}
	}

	/* check for atari */
	if (!strcmp(name, "atari"))
	{
		UINT32 sectors[4];
		UINT8 *data;
		int i, maxdiff;
		chd_error err;

		err = chd_read(chd, 0x200 / header->hunkbytes, temp);
		if (err != CHDERR_NONE)
			goto cleanup;
		data = &temp[0x200 % header->hunkbytes];

		if (data[0] != 0x0d || data[1] != 0xf0 || data[2] != 0xed || data[3] != 0xfe)
			goto cleanup;
		for (i = 0; i < 4; i++)
			sectors[i] = data[i*4+0x40] | (data[i*4+0x41] << 8) | (data[i*4+0x42] << 16) | (data[i*4+0x43] << 24);
		maxdiff = sectors[2] - sectors[1];
		if (sectors[3] - sectors[2] > maxdiff)
			maxdiff = sectors[3] - sectors[2];
		if (sectors[0] != 8)
			goto cleanup;
		*maxhunk = (sectors[3] + maxdiff + sectors_per_hunk - 1) / sectors_per_hunk;
		printf("Maximum hunk: %d\n", *maxhunk);
		if (*maxhunk >= header->totalhunks)
		{
			printf("Warning: chomp will have no effect\n");
			*maxhunk = header->totalhunks;
		}
	}

	free(temp);
	return CHDERR_NONE;

cleanup:
	printf("Error: unable to identify file or compute chomping size.\n");
	free(temp);
	return CHDERR_INVALID_DATA;
}
#endif


/*-------------------------------------------------
    do_merge_update_chomp - merge a parent and its
    child together (also works for update & chomp)
-------------------------------------------------*/

#define OPERATION_UPDATE		0
#define OPERATION_MERGE			1
#define OPERATION_CHOMP			2

static void do_merge_update_chomp(int argc, char *argv[], int operation)
{
	const char *parentfile, *inputfile, *outputfile;
	chd_file *parentchd = NULL;
	chd_file *inputchd = NULL;
	chd_file *outputchd = NULL;
	const chd_header *inputheader;
	UINT32 maxhunk = ~0;
	chd_error err;

	/* require 4-5 args total */
	if (operation == OPERATION_UPDATE && argc != 4)
		usage();
	if ((operation == OPERATION_MERGE || operation == OPERATION_CHOMP) && argc != 5)
		usage();

	/* extract the data */
	if (operation == OPERATION_MERGE)
	{
		parentfile = argv[2];
		inputfile = argv[3];
		outputfile = argv[4];
	}
	else
	{
		parentfile = NULL;
		inputfile = argv[2];
		outputfile = argv[3];
		if (operation == OPERATION_CHOMP)
			maxhunk = atoi(argv[4]);
	}

	/* print some info */
	if (parentfile)
	{
		printf("Parent file:  %s\n", parentfile);
		printf("Diff file:    %s\n", inputfile);
	}
	else
		printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	if (operation == OPERATION_CHOMP)
		printf("Maximum hunk: %d\n", maxhunk);

	/* open the parent CHD */
	if (parentfile)
	{
		err = chd_open(parentfile, CHD_OPEN_READ, NULL, &parentchd);
		if (err != CHDERR_NONE)
		{
			printf("Error opening CHD file '%s': %s\n", parentfile, error_string(err));
			goto cleanup;
		}
	}

	/* open the diff CHD */
	err = chd_open(inputfile, CHD_OPEN_READ, parentchd, &inputchd);
	if (err !=  CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err));
		goto cleanup;
	}
	inputheader = chd_get_header(inputchd);

#if ENABLE_CUSTOM_CHOMP
	/* if we're chomping with a auto parameter, now is the time to figure it out */
	if (operation == OPERATION_CHOMP && maxhunk == 0)
		if (handle_custom_chomp(argv[4], inputchd, &maxhunk) != CHDERR_NONE)
			return;
#endif

	/* create the new merged CHD */
	err = chd_create(outputfile, inputheader->logicalbytes, inputheader->hunkbytes, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* open the new CHD */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, NULL, &outputchd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* clone the metadata from the input file (which should have inherited from the parent) */
	err = chd_clone_metadata(inputchd, outputchd);
	if (err != CHDERR_NONE)
	{
		printf("Error cloning metadata: %s\n", error_string(err));
		goto cleanup;
	}

	/* do the compression; our interface will route reads for us */
	err = chdman_compress_chd(outputchd, inputchd, (operation == OPERATION_CHOMP) ? (maxhunk + 1) : 0);
	if (err != CHDERR_NONE)
		printf("Error during compression: %s\n", error_string(err));

cleanup:
	/* close everything down */
	if (outputchd)
		chd_close(outputchd);
	if (inputchd)
		chd_close(inputchd);
	if (parentchd)
		chd_close(parentchd);
	if (err != CHDERR_NONE)
		remove(outputfile);
}


/*-------------------------------------------------
    do_diff - generate a difference between two
    CHD files
-------------------------------------------------*/

static void do_diff(int argc, char *argv[])
{
	const char *parentfile = NULL, *inputfile = NULL, *outputfile = NULL;
	chd_file *parentchd = NULL;
	chd_file *inputchd = NULL;
	chd_file *outputchd = NULL;
	chd_error err;

	/* require 5 args total */
	if (argc != 5)
		usage();

	/* extract the data */
	if (argc == 5)
	{
		parentfile = argv[2];
		inputfile = argv[3];
		outputfile = argv[4];
	}

	/* print some info */
	printf("Parent file:  %s\n", parentfile);
	printf("Input file:   %s\n", inputfile);
	printf("Diff file:    %s\n", outputfile);

	/* open the soon-to-be-parent CHD */
	err = chd_open(parentfile, CHD_OPEN_READ, NULL, &parentchd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", parentfile, error_string(err));
		goto cleanup;
	}

	/* open the input CHD */
	err = chd_open(inputfile, CHD_OPEN_READ, NULL, &inputchd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err));
		goto cleanup;
	}

	/* create the new CHD as a diff against the parent */
	err = chd_create(outputfile, 0, 0, CHDCOMPRESSION_ZLIB_PLUS, parentchd);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* open the new CHD */
	err = chd_open(outputfile, CHD_OPEN_READWRITE, parentchd, &outputchd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening new CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* do the compression; our interface will route reads for us */
	err = chdman_compress_chd(outputchd, inputchd, 0);
	if (err != CHDERR_NONE)
		printf("Error during compression: %s\n", error_string(err));

cleanup:
	/* close everything down */
	if (outputchd)
		chd_close(outputchd);
	if (inputchd)
		chd_close(inputchd);
	if (parentchd)
		chd_close(parentchd);
	if (err != CHDERR_NONE)
		remove(outputfile);
}


/*-------------------------------------------------
    do_setchs - change the CHS values on a hard
    disk image
-------------------------------------------------*/

static void do_setchs(int argc, char *argv[])
{
	int oldcyls, oldhds, oldsecs, oldsecsize;
	int cyls, hds, secs;
	chd_error err;
	const char *inoutfile;
	chd_header header;
	chd_file *chd = NULL;
	char metadata[256];
	UINT64 old_logicalbytes;
	UINT8 was_readonly = 0;

	/* require 6 args total */
	if (argc != 6)
		usage();

	/* extract the data */
	inoutfile = argv[2];
	cyls = atoi(argv[3]);
	hds = atoi(argv[4]);
	secs = atoi(argv[5]);

	/* print some info */
	printf("Input file:   %s\n", inoutfile);
	printf("Cylinders:    %d\n", cyls);
	printf("Heads:        %d\n", hds);
	printf("Sectors:      %d\n", secs);

	/* open the file read-only and get the header */
	err = chd_open(inoutfile, CHD_OPEN_READ, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s' read-only: %s\n", inoutfile, error_string(err));
		return;
	}
	header = *chd_get_header(chd);
	chd_close(chd);

	/* if the drive is not writeable, note that, and make it so */
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE))
	{
		was_readonly = 1;
		header.flags |= CHDFLAGS_IS_WRITEABLE;

		/* write the new header */
		err = chd_set_header(inoutfile, &header);
		if (err != CHDERR_NONE)
		{
			printf("Error making CHD file writeable: %s\n", error_string(err));
			return;
		}
	}

	/* open the file read/write */
	err = chd_open(inoutfile, CHD_OPEN_READWRITE, NULL, &chd);
	if (err != CHDERR_NONE)
	{
		printf("Error opening CHD file '%s' read/write: %s\n", inoutfile, error_string(err));
		return;
	}

	/* get the hard disk metadata */
	err = chd_get_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, sizeof(metadata), NULL, NULL);
	if (err != CHDERR_NONE || sscanf(metadata, HARD_DISK_METADATA_FORMAT, &oldcyls, &oldhds, &oldsecs, &oldsecsize) != 4)
	{
		printf("CHD file '%s' is not a hard disk!\n", inoutfile);
		goto cleanup;
	}

	/* write our own */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cyls, hds, secs, oldsecsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error writing new metadata to CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* get the header and compute the new logical size */
	header = *chd_get_header(chd);
	old_logicalbytes = header.logicalbytes;
	header.logicalbytes = (UINT64)cyls * (UINT64)hds * (UINT64)secs * (UINT64)oldsecsize;

	/* close the file */
	chd_close(chd);

	/* restore the read-only state */
	if (was_readonly)
		header.flags &= ~CHDFLAGS_IS_WRITEABLE;

	/* set the new logical size */
	if (header.logicalbytes != old_logicalbytes || was_readonly)
	{
		err = chd_set_header(inoutfile, &header);
		if (err != CHDERR_NONE)
			printf("Error writing new header to CHD file: %s\n", error_string(err));
	}

	/* print a warning if the size is different */
	if (header.logicalbytes < old_logicalbytes)
		printf("WARNING: new size is smaller; run chdman -update to reclaim empty space\n");
	else if (header.logicalbytes > old_logicalbytes)
		printf("WARNING: new size is larger; run chdman -update to account for new empty space\n");
	return;

cleanup:
	if (chd)
		chd_close(chd);
	if (was_readonly)
	{
		header.flags &= ~CHDFLAGS_IS_WRITEABLE;
		chd_set_header(inoutfile, &header);
	}
}


/*-------------------------------------------------
    chdman_compress_file - compress a regular
    file via the compression interfaces
-------------------------------------------------*/

static chd_error chdman_compress_file(chd_file *chd, const char *rawfile, UINT32 offset)
{
	chd_interface_file *sourcefile;
	const chd_header *header;
	UINT64 sourceoffset = 0;
	UINT8 *cache = NULL;
	double ratio = 1.0;
	chd_error err;
	int hunknum;

	/* open the raw file */
	sourcefile = chdman_open(rawfile, "rb");
	if (sourcefile == NULL)
	{
		err = CHDERR_FILE_NOT_FOUND;
		goto cleanup;
	}

	/* get the header */
	header = chd_get_header(chd);
	cache = malloc(header->hunkbytes);
	if (cache == NULL)
	{
		err = CHDERR_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* begin compressing */
	err = chd_compress_begin(chd);
	if (err != CHDERR_NONE)
		goto cleanup;

	/* loop over source hunks until we run out */
	for (hunknum = 0; hunknum < header->totalhunks; hunknum++)
	{
		UINT32 bytesread;

		/* progress */
		progress(FALSE, "Compressing hunk %d/%d... (ratio=%d%%)  \r", hunknum, header->totalhunks, (int)(100.0 * ratio));

		/* read the data */
		bytesread = chdman_read(sourcefile, sourceoffset + offset, header->hunkbytes, cache);
		if (bytesread < header->hunkbytes)
			memset(&cache[bytesread], 0, header->hunkbytes - bytesread);

		/* append the data */
		err = chd_compress_hunk(chd, cache, &ratio);
		if (err != CHDERR_NONE)
			goto cleanup;

		/* prepare for the next hunk */
		sourceoffset += header->hunkbytes;
	}

	/* finish compression */
	err = chd_compress_finish(chd);
	if (err != CHDERR_NONE)
		goto cleanup;

	/* final progress update */
	progress(TRUE, "Compression complete ... final ratio = %d%%            \n", (int)(100.0 * ratio));

cleanup:
	if (sourcefile != NULL)
		chdman_close(sourcefile);
	if (cache != NULL)
		free(cache);
	return err;
}


/*-------------------------------------------------
    chdman_compress_chd - (re)compress a CHD file
    via the compression interfaces
-------------------------------------------------*/

static chd_error chdman_compress_chd(chd_file *chd, chd_file *source, UINT32 totalhunks)
{
	const chd_header *source_header;
	const chd_header *header;
	UINT8 *source_cache = NULL;
	UINT64 source_offset = 0;
	UINT32 source_bytes = 0;
	UINT8 *cache = NULL;
	double ratio = 1.0;
	chd_error err, verifyerr;
	int hunknum;

	/* get the header */
	header = chd_get_header(chd);
	cache = malloc(header->hunkbytes);
	if (cache == NULL)
	{
		err = CHDERR_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* get the source CHD header */
	source_header = chd_get_header(source);
	source_cache = malloc(source_header->hunkbytes);
	if (source_cache == NULL)
	{
		err = CHDERR_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* begin compressing */
	err = chd_compress_begin(chd);
	if (err != CHDERR_NONE)
		goto cleanup;

	/* also begin verifying the source driver */
	verifyerr = chd_verify_begin(source);

	/* a zero count means the natural number */
	if (totalhunks == 0)
		totalhunks = source_header->totalhunks;

	/* loop over source hunks until we run out */
	for (hunknum = 0; hunknum < totalhunks; hunknum++)
	{
		UINT32 bytesremaining = header->hunkbytes;
		UINT8 *dest = cache;

		/* progress */
		progress(FALSE, "Compressing hunk %d/%d... (ratio=%d%%)  \r", hunknum, totalhunks, (int)(100.0 * ratio));

		/* read the data */
		while (bytesremaining > 0)
		{
			/* if we have data in the buffer, copy it */
			if (source_bytes > 0)
			{
				UINT32 bytestocopy = MIN(bytesremaining, source_bytes);
				memcpy(dest, &source_cache[source_header->hunkbytes - source_bytes], bytestocopy);
				dest += bytestocopy;
				source_bytes -= bytestocopy;
				bytesremaining -= bytestocopy;
			}

			/* otherwise, read in another hunk of the source */
			else
			{
				/* verify the next hunk */
				if (verifyerr == CHDERR_NONE)
					err = chd_verify_hunk(source);

				/* then read it (should be the same) */
				err = chd_read(source, source_offset / source_header->hunkbytes, source_cache);
				if (err != CHDERR_NONE)
					memset(source_cache, 0, source_header->hunkbytes);
				source_bytes = source_header->hunkbytes;
				source_offset += source_bytes;
			}
		}

		/* append the data */
		err = chd_compress_hunk(chd, cache, &ratio);
		if (err != CHDERR_NONE)
			goto cleanup;
	}

	/* if we read all the source data, verify the checksums */
	if (verifyerr == CHDERR_NONE && source_offset >= source_header->logicalbytes)
	{
		static const UINT8 empty_checksum[CHD_SHA1_BYTES] = { 0 };
		UINT8 md5[CHD_MD5_BYTES];
		UINT8 sha1[CHD_SHA1_BYTES];
		int i;

		/* get the final values */
		err = chd_verify_finish(source, md5, sha1);

		/* check the MD5 */
		if (memcmp(source_header->md5, empty_checksum, CHD_MD5_BYTES) != 0)
		{
			if (memcmp(source_header->md5, md5, CHD_MD5_BYTES) != 0)
			{
				progress(TRUE, "WARNING: expected input MD5 = ");
				for (i = 0; i < CHD_MD5_BYTES; i++)
					progress(TRUE, "%02x", source_header->md5[i]);
				progress(TRUE, "\n");

				progress(TRUE, "                 actual MD5 = ");
				for (i = 0; i < CHD_MD5_BYTES; i++)
					progress(TRUE, "%02x", md5[i]);
				progress(TRUE, "\n");
			}
			else
				progress(TRUE, "Input MD5 verified                            \n");
		}

		/* check the SHA1 */
		if (memcmp(source_header->sha1, empty_checksum, CHD_SHA1_BYTES) != 0)
		{
			if (memcmp(source_header->sha1, sha1, CHD_SHA1_BYTES) != 0)
			{
				progress(TRUE, "WARNING: expected input SHA1 = ");
				for (i = 0; i < CHD_SHA1_BYTES; i++)
					progress(TRUE, "%02x", source_header->sha1[i]);
				progress(TRUE, "\n");

				progress(TRUE, "                 actual SHA1 = ");
				for (i = 0; i < CHD_SHA1_BYTES; i++)
					progress(TRUE, "%02x", sha1[i]);
				progress(TRUE, "\n");
			}
			else
				progress(TRUE, "Input SHA1 verified                            \n");
		}
	}

	/* finish compression */
	err = chd_compress_finish(chd);
	if (err != CHDERR_NONE)
		goto cleanup;

	/* final progress update */
	progress(TRUE, "Compression complete ... final ratio = %d%%            \n", (int)(100.0 * ratio));

cleanup:
	if (source_cache != NULL)
		free(source_cache);
	if (cache != NULL)
		free(cache);
	return err;
}


/*-------------------------------------------------
    chdman_open - open file
-------------------------------------------------*/

static chd_interface_file *chdman_open(const char *filename, const char *mode)
{
	return (chd_interface_file *)osd_tool_fopen(filename, mode);
}


/*-------------------------------------------------
    chdman_close - close file
-------------------------------------------------*/

static void chdman_close(chd_interface_file *file)
{
	osd_tool_fclose((osd_tool_file *)file);
}


/*-------------------------------------------------
    chdman_read - read from an offset
-------------------------------------------------*/

static UINT32 chdman_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	return osd_tool_fread((osd_tool_file *)file, offset, count, buffer);
}


/*-------------------------------------------------
    chdman_write - write to an offset
-------------------------------------------------*/

static UINT32 chdman_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	return osd_tool_fwrite((osd_tool_file *)file, offset, count, buffer);
}


/*-------------------------------------------------
    chdman_length - return the current EOF
-------------------------------------------------*/

static UINT64 chdman_length(chd_interface_file *file)
{
	return osd_tool_flength((osd_tool_file *)file);
}


/*-------------------------------------------------
    main - entry point
-------------------------------------------------*/

int main(int argc, char **argv)
{
	extern char build_version[];
	printf("chdman - MAME Compressed Hunks of Data (CHD) manager %s\n", build_version);

	/* require at least 1 argument */
	if (argc < 2)
		usage();

	/* set the interface for everyone */
	chd_set_interface(&chdman_interface);

	/* handle the appropriate command */
	if (!strcmp(argv[1], "-createhd"))
		do_createhd(argc, argv);
	else if (!strcmp(argv[1], "-createraw"))
		do_createraw(argc, argv);
	else if (!strcmp(argv[1], "-createblankhd"))
		do_createblankhd(argc, argv);
	else if (!strcmp(argv[1], "-copydata"))
		do_copydata(argc, argv);
	else if (!strcmp(argv[1], "-createcd"))
		do_createcd(argc, argv);
	else if (!strcmp(argv[1], "-extract"))
		do_extract(argc, argv);
	else if (!strcmp(argv[1], "-extractcd"))
		do_extractcd(argc, argv);
	else if (!strcmp(argv[1], "-verify"))
		do_verify(argc, argv, 0);
	else if (!strcmp(argv[1], "-verifyfix"))
		do_verify(argc, argv, 1);
	else if (!strcmp(argv[1], "-update"))
		do_merge_update_chomp(argc, argv, OPERATION_UPDATE);
	else if (!strcmp(argv[1], "-chomp"))
		do_merge_update_chomp(argc, argv, OPERATION_CHOMP);
	else if (!strcmp(argv[1], "-info"))
		do_info(argc, argv);
	else if (!strcmp(argv[1], "-merge"))
		do_merge_update_chomp(argc, argv, OPERATION_MERGE);
	else if (!strcmp(argv[1], "-diff"))
		do_diff(argc, argv);
	else if (!strcmp(argv[1], "-setchs"))
		do_setchs(argc, argv);
	else
		usage();

	return 0;
}
