/*********************************************************************

    common.c

    Generic functions, mostly resource and memory related.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "png.h"
#include "artwork.h"
#include "config.h"
#include "state.h"
#include <stdarg.h>
#include <ctype.h>


//#define LOG_LOAD



/***************************************************************************

    Constants

***************************************************************************/

// VERY IMPORTANT: osd_alloc_bitmap must allocate also a "safety area" 16 pixels wide all
// around the bitmap. This is required because, for performance reasons, some graphic
// routines don't clip at boundaries of the bitmap.
#define BITMAP_SAFETY			16



/***************************************************************************

    Type definitions

***************************************************************************/

struct _region_info
{
	UINT8 *		base;
	size_t		length;
	UINT32		type;
	UINT32		flags;
};
typedef struct _region_info region_info;




/***************************************************************************

    Global variables

***************************************************************************/

/* These globals are only kept on a machine basis - LBO 042898 */
unsigned int dispensed_tickets;
unsigned int coin_count[COIN_COUNTERS];
unsigned int coinlockedout[COIN_COUNTERS];
static unsigned int lastcoin[COIN_COUNTERS];

/* malloc tracking */
static void** malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* array of memory regions */
static region_info mem_region[MAX_MEMORY_REGIONS];

/* generic NVRAM */
size_t generic_nvram_size;
UINT8 *generic_nvram;
UINT16 *generic_nvram16;
UINT32 *generic_nvram32;

/* movie file */
static mame_file *movie_file = NULL;
static int movie_frame = 0;



/***************************************************************************

    Functions

***************************************************************************/

void showdisclaimer(void)   /* MAURY_BEGIN: dichiarazione */
{
	printf("MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		 "several arcade machines. But hardware is useless without software, so an image\n"
		 "of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
		 "commercial software, are copyrighted material and it is therefore illegal to\n"
		 "use them if you don't own the original arcade machine. Needless to say, ROMs\n"
		 "are not distributed together with MAME. Distribution of MAME together with ROM\n"
		 "images is a violation of copyright law and should be promptly reported to the\n"
		 "authors so that appropriate legal action can be taken.\n\n");
}                           /* MAURY_END: dichiarazione */



/***************************************************************************

    Memory region code

***************************************************************************/

/*-------------------------------------------------
    memory_region - returns pointer to a memory
    region
-------------------------------------------------*/

UINT8 *memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return mem_region[num].base;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (mem_region[i].type == num)
				return mem_region[i].base;
		}
	}

	return 0;
}


/*-------------------------------------------------
    memory_region_length - returns length of a
    memory region
-------------------------------------------------*/

size_t memory_region_length(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return mem_region[num].length;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (mem_region[i].type == num)
				return mem_region[i].length;
		}
	}

	return 0;
}


/*-------------------------------------------------
    new_memory_region - allocates memory for a
    region
-------------------------------------------------*/

int new_memory_region(int num, size_t length, UINT32 flags)
{
    int i;

    if (num < MAX_MEMORY_REGIONS)
    {
        mem_region[num].length = length;
        mem_region[num].base = malloc(length);
        return (mem_region[num].base == NULL) ? 1 : 0;
    }
    else
    {
        for (i = 0;i < MAX_MEMORY_REGIONS;i++)
        {
            if (mem_region[i].base == NULL)
            {
                mem_region[i].length = length;
                mem_region[i].type = num;
                mem_region[i].flags = flags;
                mem_region[i].base = malloc(length);
                return (mem_region[i].base == NULL) ? 1 : 0;
            }
        }
    }
	return 1;
}


/*-------------------------------------------------
    free_memory_region - releases memory for a
    region
-------------------------------------------------*/

void free_memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
	{
		free(mem_region[num].base);
		memset(&mem_region[num], 0, sizeof(mem_region[num]));
	}
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (mem_region[i].type == num)
			{
				free(mem_region[i].base);
				memset(&mem_region[i], 0, sizeof(mem_region[i]));
				return;
			}
		}
	}
}


/*-------------------------------------------------
    free_disposable_memory_regions - releases all
    disposable memory regions
-------------------------------------------------*/

void free_disposable_memory_regions(void)
{
	int region;

	for (region = 0; region < MAX_MEMORY_REGIONS; region++)
		if (mem_region[region].flags & ROMREGION_DISPOSE)
		{
			int i;

			/* invalidate contents to avoid subtle bugs */
			for (i = 0; i < mem_region[region].length; i++)
				mem_region[region].base[i] = rand();
			free(mem_region[region].base);
			mem_region[region].base = 0;
		}
}



/***************************************************************************

    Coin counter code

***************************************************************************/

void counters_load(int config_type, xml_data_node *parentnode)
{
	xml_data_node *coinnode, *ticketnode;

	/* on init, reset the counters */
	if (config_type == CONFIG_TYPE_INIT)
	{
		memset(coin_count, 0, sizeof(coin_count));
		dispensed_tickets = 0;
	}

	/* only care about game-specific data */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* might not have any data */
	if (!parentnode)
		return;

	/* iterate over coins nodes */
	for (coinnode = xml_get_sibling(parentnode->child, "coins"); coinnode; coinnode = xml_get_sibling(coinnode->next, "coins"))
	{
		int index = xml_get_attribute_int(coinnode, "index", -1);
		if (index >= 0 && index < COIN_COUNTERS)
			coin_count[index] = xml_get_attribute_int(coinnode, "number", 0);
	}

	/* get the single tickets node */
	ticketnode = xml_get_sibling(parentnode->child, "tickets");
	if (ticketnode)
		dispensed_tickets = xml_get_attribute_int(ticketnode, "number", 0);
}


void counters_save(int config_type, xml_data_node *parentnode)
{
	int i;

	/* only care about game-specific data */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* iterate over coin counters */
	for (i = 0; i < COIN_COUNTERS; i++)
		if (coin_count[i] != 0)
		{
			xml_data_node *coinnode = xml_add_child(parentnode, "coins", NULL);
			if (coinnode)
			{
				xml_set_attribute_int(coinnode, "index", i);
				xml_set_attribute_int(coinnode, "number", coin_count[i]);
			}
		}

	/* output tickets */
	if (dispensed_tickets != 0)
	{
		xml_data_node *tickets = xml_add_child(parentnode, "tickets", NULL);
		if (tickets)
			xml_set_attribute_int(tickets, "number", dispensed_tickets);
	}
}


void coin_counter_reset(void)
{
	int counternum;
	for (counternum = 0; counternum < COIN_COUNTERS; counternum++)
	{
		lastcoin[counternum] = 0;
		coinlockedout[counternum] = 0;
	}
}


/*-------------------------------------------------
    coin_counter_w - sets input for coin counter
-------------------------------------------------*/

void coin_counter_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;
	/* Count it only if the data has changed from 0 to non-zero */
	if (on && (lastcoin[num] == 0))
	{
		coin_count[num]++;
	}
	lastcoin[num] = on;
}


/*-------------------------------------------------
    coin_lockout_w - locks out one coin input
-------------------------------------------------*/

void coin_lockout_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;

	coinlockedout[num] = on;
}


/*-------------------------------------------------
    coin_lockout_global_w - locks out all the coin
    inputs
-------------------------------------------------*/

void coin_lockout_global_w(int on)
{
	int i;

	for (i = 0; i < COIN_COUNTERS; i++)
	{
		coin_lockout_w(i,on);
	}
}



/***************************************************************************

    Generic NVRAM code

***************************************************************************/

void *nvram_select(void)
{
	if (generic_nvram)
		return generic_nvram;
	if (generic_nvram16)
		return generic_nvram16;
	if (generic_nvram32)
		return generic_nvram32;
	osd_die("generic nvram handler called without nvram in the memory map\n");
	return 0;
}


/*-------------------------------------------------
    nvram_handler_generic_0fill - generic NVRAM
    with a 0 fill
-------------------------------------------------*/

void nvram_handler_generic_0fill(mame_file *file, int read_or_write)
{
	if (read_or_write)
		mame_fwrite(file, nvram_select(), generic_nvram_size);
	else if (file)
		mame_fread(file, nvram_select(), generic_nvram_size);
	else
		memset(nvram_select(), 0, generic_nvram_size);
}


/*-------------------------------------------------
    nvram_handler_generic_1fill - generic NVRAM
    with a 1 fill
-------------------------------------------------*/

void nvram_handler_generic_1fill(mame_file *file, int read_or_write)
{
	if (read_or_write)
		mame_fwrite(file, nvram_select(), generic_nvram_size);
	else if (file)
		mame_fread(file, nvram_select(), generic_nvram_size);
	else
		memset(nvram_select(), 0xff, generic_nvram_size);
}


/*-------------------------------------------------
    nvram_handler_generic_randfill - generic NVRAM
    with a random fill
-------------------------------------------------*/

void nvram_handler_generic_randfill(mame_file *file, int read_or_write)
{
	int i;

	if (read_or_write)
		mame_fwrite(file, nvram_select(), generic_nvram_size);
	else if (file)
		mame_fread(file, nvram_select(), generic_nvram_size);
	else
	{
		UINT8 *nvram = nvram_select();
		for (i = 0; i < generic_nvram_size; i++)
			nvram[i] = rand();
	}
}



/***************************************************************************

    Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
    bitmap_alloc_core
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(mame_bitmap)) : malloc(sizeof(mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		UINT8 *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(UINT8 *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (UINT8 *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		set_pixel_functions(bitmap);
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
    bitmap_alloc - allocate a bitmap at the
    current screen depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->color_depth,0);
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,0);
}


/*-------------------------------------------------
    bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



/***************************************************************************

    Resource tracking code

***************************************************************************/

/*-------------------------------------------------
    auto_malloc_add - add pointer to malloc list
-------------------------------------------------*/

INLINE void auto_malloc_add(void *result)
{
	/* make sure we have tracking space */
	if (malloc_list_index == malloc_list_size)
	{
		void **list;

		/* if this is the first time, allocate 256 entries, otherwise double the slots */
		if (malloc_list_size == 0)
			malloc_list_size = 256;
		else
			malloc_list_size *= 2;

		/* realloc the list */
		list = realloc(malloc_list, malloc_list_size * sizeof(list[0]));
		if (list == NULL)
			osd_die("Unable to extend malloc tracking array to %d slots", malloc_list_size);
		malloc_list = list;
	}
	malloc_list[malloc_list_index++] = result;
}


/*-------------------------------------------------
    auto_start - prepare frames
-------------------------------------------------*/

void auto_start(void)
{
	auto_malloc_add(NULL);
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *auto_malloc(size_t size)
{
	void *result;

	/* malloc-ing zero bytes is inherently unportable; hence this check */
	if (size == 0)
		osd_die("Attempted to auto_malloc zero bytes");

	result = malloc(size);

	/* fail horribly if it doesn't work */
	if (!result)
		osd_die("Out of memory attempting to allocate %d bytes\n", (int)size);

	auto_malloc_add(result);

	return result;
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *auto_strdup(const char *str)
{
	return strcpy(auto_malloc(strlen(str) + 1), str);
}


/*-------------------------------------------------
    auto_free - release auto_malloc'd memory
-------------------------------------------------*/

void auto_free(void)
{
	/* start at the end and free everything till you reach the sentinel */
	while (malloc_list_index > 0 && malloc_list[--malloc_list_index] != NULL)
		free(malloc_list[malloc_list_index]);

	/* if we free everything, free the list */
	if (malloc_list_index == 0)
	{
		free(malloc_list);
		malloc_list = NULL;
		malloc_list_size = 0;
	}
}


/*-------------------------------------------------
    bitmap_alloc - allocate a bitmap at the
    current screen depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->color_depth,1);
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,1);
}


/*-------------------------------------------------
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	auto_start();

	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
    end_resource_tracking - stop tracking
    resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	/* call everyone who tracks resources to let them know */
	auto_free();
	timer_free();
	state_save_free();

	/* decrement the tag counter */
	resource_tracking_tag--;
}



/***************************************************************************

    Screen snapshot and movie recording code

***************************************************************************/

/*-------------------------------------------------
    save_frame_with - save a frame with a
    given handler for screenshots and movies
-------------------------------------------------*/

static void save_frame_with(mame_file *fp, mame_bitmap *bitmap, int (*write_handler)(mame_file *, mame_bitmap *))
{
	rectangle bounds;
	mame_bitmap *osdcopy;
	UINT32 saved_rgb_components[3];

	/* allow the artwork system to override certain parameters */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		bounds.min_x = 0;
		bounds.max_x = bitmap->width - 1;
		bounds.min_y = 0;
		bounds.max_y = bitmap->height - 1;
	}
	else
	{
		bounds = Machine->visible_area;
	}
	memcpy(saved_rgb_components, direct_rgb_components, sizeof(direct_rgb_components));
	artwork_override_screenshot_params(&bitmap, &bounds, direct_rgb_components);

	/* allow the OSD system to muck with the screenshot */
	osdcopy = osd_override_snapshot(bitmap, &bounds);
	if (osdcopy)
		bitmap = osdcopy;

	/* now do the actual work */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		write_handler(fp, bitmap);
	}
	else
	{
		mame_bitmap *copy;
		int sizex, sizey, scalex, scaley;

		sizex = bounds.max_x - bounds.min_x + 1;
		sizey = bounds.max_y - bounds.min_y + 1;

		scalex = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_2_1) ? 2 : 1;
		scaley = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) ? 2 : 1;

		if(Machine->gamedrv->flags & ORIENTATION_SWAP_XY)
		{
			int temp;

			temp = scalex;
			scalex = scaley;
			scaley = temp;
		}

		copy = bitmap_alloc_depth(sizex * scalex,sizey * scaley,bitmap->depth);
		if (copy)
		{
			int x,y,sx,sy;

			sx = bounds.min_x;
			sy = bounds.min_y;

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			write_handler(fp, copy);
			bitmap_free(copy);
		}
	}
	memcpy(direct_rgb_components, saved_rgb_components, sizeof(saved_rgb_components));

	/* if the OSD system allocated a bitmap; free it */
	if (osdcopy)
		bitmap_free(osdcopy);
}


 /*-------------------------------------------------
    save_screen_snapshot_as - save a snapshot to
    the given file handle
-------------------------------------------------*/

void save_screen_snapshot_as(mame_file *fp, mame_bitmap *bitmap)
{
	save_frame_with(fp, bitmap, png_write_bitmap);
}


/*-------------------------------------------------
    open the next non-existing file of type
    filetype according to our numbering scheme
-------------------------------------------------*/

static mame_file *mame_fopen_next(int filetype)
{
	char name[FILENAME_MAX];
	int seq;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.xxx" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (mame_faccess(name, filetype))
	{
		seq = 0;
		do
		{
			/* otherwise use "nameNNNN.xxx" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name, seq++);
		} while (mame_faccess(name, filetype));
	}

    return (mame_fopen(Machine->gamedrv->name, name, filetype, 1));
}


/*-------------------------------------------------
    save_screen_snapshot - save a snapshot.
-------------------------------------------------*/

void save_screen_snapshot(mame_bitmap *bitmap)
{
	mame_file *fp;

	if ((fp = mame_fopen_next(FILETYPE_SCREENSHOT)) != NULL)
	{
		save_screen_snapshot_as(fp, bitmap);
		mame_fclose(fp);
	}
}


/*-------------------------------------------------
    record_movie - start, stop and update the
    recording of a MNG movie
-------------------------------------------------*/

void record_movie_toggle(void)
{
	if (movie_file == NULL)
	{
		movie_frame = 0;
		movie_file = mame_fopen_next(FILETYPE_MOVIE);
		if (movie_file)
			ui_popup("REC START");
	}
	else
	{
		mng_capture_stop(movie_file);
		mame_fclose(movie_file);
		movie_file = NULL;
		ui_popup("REC STOP (%d frames)", movie_frame);
	}
}


void record_movie_stop(void)
{
	if (movie_file)
		record_movie_toggle();
}


void record_movie_frame(mame_bitmap *bitmap)
{
	if (movie_file != NULL && bitmap != NULL)
	{
		if (movie_frame++ == 0)
			save_frame_with(movie_file, bitmap, mng_capture_start);
		save_frame_with(movie_file, bitmap, mng_capture_frame);
	}
}
