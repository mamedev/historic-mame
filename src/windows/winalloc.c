//============================================================
//
//  winalloc.c - Win32 memory allocation routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"

// undefine any redefines we have in the prefix
#undef malloc
#undef calloc
#undef realloc



//============================================================
//  CONSTANTS
//============================================================

#define PAGE_SIZE		4096
#define MAX_ALLOCS		65536
#define COOKIE_VAL		0x11335577

// set this to 1 to align memory blocks to the start of a page;
// otherwise, they are aligned to the end, thus catching array
// overruns
#define ALIGN_START		0

// set this to 1 to record all mallocs and frees in the logfile
#define LOG_CALLS		0


//============================================================
//  TYPEDEFS
//============================================================

typedef struct
{
	UINT32			size;
	void *			base;
	void *			vbase;
	int				id;
	const char *	file;
	int				line;
} memory_entry;



//============================================================
//  LOCAL VARIABLES
//============================================================

static memory_entry memory_list[MAX_ALLOCS];
static int current_id;


//============================================================
//  INLINES
//============================================================

INLINE memory_entry *allocate_entry(void)
{
	int i;

	// find an empty entry
	for (i = 0; i < MAX_ALLOCS; i++)
	{
		if (memory_list[i].vbase == NULL)
		{
			memory_list[i].id = current_id++;
			return &memory_list[i];
		}
	}

	// if none, error out in a fatal way
	fprintf(stderr, "Out of allocation blocks!\n");
	exit(1);
}


INLINE memory_entry *find_entry(void *pointer)
{
	int i;

	if (pointer)
	{
		// find a matching entry
		for (i = 0; i < MAX_ALLOCS; i++)
			if (memory_list[i].base == pointer)
				return &memory_list[i];
	}
	return NULL;
}


INLINE void free_entry(memory_entry *entry)
{
	entry->size = 0;
	entry->base = NULL;
	entry->vbase = NULL;
	entry->file = NULL;
	entry->line = 0;
}



//============================================================
//  IMPLEMENTATION
//============================================================

void* malloc_file_line(size_t size, const char *file, int line)
{
	memory_entry *entry = allocate_entry();
	UINT8 *page_base, *block_base;
	size_t rounded_size;

	// round the size up to a page boundary
	rounded_size = ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

	// reserve that much memory, plus two guard pages
	page_base = VirtualAlloc(NULL, rounded_size + 2 * PAGE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
	if (page_base == NULL)
		return NULL;

	// now allow access to everything but the first and last pages
	page_base = VirtualAlloc(page_base + PAGE_SIZE, rounded_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (page_base == NULL)
		return NULL;

	// work backwards from the page base to get to the block base
#if ALIGN_START
	block_base = page_base;
#else
	block_base = page_base + rounded_size - size;
#endif

	// fill in the entry
	entry->size = size;
	entry->base = block_base;
	entry->vbase = page_base - PAGE_SIZE;
	entry->file = file;
	entry->line = line;
#if LOG_CALLS
	if (entry->file)
		logerror("malloc #%06d size = %d (%s:%d)\n",entry->id,entry->size,entry->file,entry->line);
	else
		logerror("malloc #%06d size = %d\n",entry->id,entry->size);
#endif
	return block_base;
}


void* CLIB_DECL malloc(size_t size)
{
	return malloc_file_line(size, NULL, 0);
}


void* calloc_file_line(size_t size, size_t count, const char *file, int line)
{
	// first allocate the memory
	void *memory = malloc_file_line(size * count, file, line);
	if (memory == NULL)
		return NULL;

	// then memset it
	memset(memory, 0, size * count);
	return memory;
}


void* CLIB_DECL calloc(size_t size, size_t count)
{
	return calloc_file_line(size, count, NULL, 0);
}


void * realloc_file_line(void *memory, size_t size, const char *file, int line)
{
	void *newmemory = NULL;

	// if size is non-zero, we need to reallocate memory
	if (size != 0)
	{
		// allocate space for the new amount
		newmemory = malloc_file_line(size, file, line);
		if (newmemory == NULL)
			return NULL;

		// if we have an old pointer, copy it
		if (memory != NULL)
		{
			memory_entry *entry = find_entry(memory);
			if (entry != NULL)
				memcpy(newmemory, memory, (size < entry->size) ? size : entry->size);
			else
				fprintf(stderr, "Error: realloc a non-existant block\n");
		}
	}

	// if we have an original pointer, free it
	if (memory != NULL)
		free(memory);

	return newmemory;
}


void * CLIB_DECL realloc(void *memory, size_t size)
{
	return realloc_file_line(memory, size, NULL, 0);
}


void CLIB_DECL free(void *memory)
{
	memory_entry *entry = find_entry(memory);

	// allow NULL frees
	if (memory == NULL)
		return;

	// error if no entry found
	if (entry == NULL)
	{
		fprintf(stderr, "Error: free a non-existant block\n");
		return;
	}

	// free the memory
	VirtualFree(entry->vbase, 0, MEM_RELEASE);
#if LOG_CALLS
	logerror("free #%06d size = %d\n",entry->id,entry->size);
#endif
	free_entry(entry);
}

size_t CLIB_DECL _msize(void *memory)
{
	memory_entry *entry = find_entry(memory);

	// error if no entry found
	if (entry == NULL)
	{
		fprintf(stderr, "Error: msize a non-existant block\n");
		return 0;
	}
	return entry->size;
}


void check_unfreed_mem(void)
{
	int i,total = 0;

	// check for leaked memory
	for (i = 0; i < MAX_ALLOCS; i++)
	{
		if (memory_list[i].base != NULL)
		{
			if (total == 0)
				printf("--- memory leak warning ---\n");
			total += memory_list[i].size;
			if (memory_list[i].file)
				printf("allocation #%06d, %d bytes (%s:%d)\n",memory_list[i].id,memory_list[i].size,memory_list[i].file,memory_list[i].line);
			else
				printf("allocation #%06d, %d bytes\n",memory_list[i].id,memory_list[i].size);
		}
	}

	if (total > 0)
		printf("a total of %d bytes were not free()'d\n",total);
}
