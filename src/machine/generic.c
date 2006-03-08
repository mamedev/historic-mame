/*********************************************************************

    generic.c

    Generic simple machine functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "config.h"
#include <stdarg.h>
#include <ctype.h>



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* These globals are only kept on a machine basis - LBO 042898 */
UINT32 dispensed_tickets;
UINT32 coin_count[COIN_COUNTERS];
UINT32 coinlockedout[COIN_COUNTERS];
static UINT32 lastcoin[COIN_COUNTERS];

/* generic NVRAM */
size_t generic_nvram_size;
UINT8 *generic_nvram;
UINT16 *generic_nvram16;
UINT32 *generic_nvram32;

/* memory card status */
static int memcard_inserted;

/* LED status */
static UINT32 leds_status;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void counters_load(int config_type, xml_data_node *parentnode);
static void counters_save(int config_type, xml_data_node *parentnode);



/***************************************************************************

    Initialization

***************************************************************************/

/*-------------------------------------------------
    generic_machine_init - initialize globals and
    register for save states
-------------------------------------------------*/

void generic_machine_init(void)
{
	int counternum;

	/* reset coin counters */
	for (counternum = 0; counternum < COIN_COUNTERS; counternum++)
	{
		lastcoin[counternum] = 0;
		coinlockedout[counternum] = 0;
	}

	/* reset NVRAM size and pointers */
	generic_nvram_size = 0;
	generic_nvram = NULL;
	generic_nvram16 = NULL;
	generic_nvram32 = NULL;

	/* reset memory card info */
	memcard_inserted = -1;

	/* register for configuration */
	config_register("counters", counters_load, counters_save);

	/* for memory cards, request save state and an exit callback */
	if (Machine->drv->memcard_handler != NULL)
	{
		state_save_register_global(memcard_inserted);
		add_exit_callback(memcard_eject);
	}
}



/***************************************************************************

    Coin counters

***************************************************************************/

/*-------------------------------------------------
    counters_load - load the state of the counters
    and tickets
-------------------------------------------------*/

static void counters_load(int config_type, xml_data_node *parentnode)
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


/*-------------------------------------------------
    counters_save - save the state of the counters
    and tickets
-------------------------------------------------*/

static void counters_save(int config_type, xml_data_node *parentnode)
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

    NVRAM management

***************************************************************************/

/*-------------------------------------------------
    nvram_select - select the right pointer based
    on which ones are non-NULL
-------------------------------------------------*/

INLINE void *nvram_select(void)
{
	if (generic_nvram)
		return generic_nvram;
	if (generic_nvram16)
		return generic_nvram16;
	if (generic_nvram32)
		return generic_nvram32;
	fatalerror("generic nvram handler called without nvram in the memory map");
	return 0;
}


/*-------------------------------------------------
    nvram_load - load a system's NVRAM
-------------------------------------------------*/

void nvram_load(void)
{
	if (Machine->drv->nvram_handler != NULL)
	{
		mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0);
		(*Machine->drv->nvram_handler)(nvram_file, 0);
		if (nvram_file != NULL)
			mame_fclose(nvram_file);
	}
}


/*-------------------------------------------------
    nvram_save - save a system's NVRAM
-------------------------------------------------*/

void nvram_save(void)
{
	if (Machine->drv->nvram_handler != NULL)
	{
		mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
		if (nvram_file != NULL)
		{
			(*Machine->drv->nvram_handler)(nvram_file, 1);
			mame_fclose(nvram_file);
		}
	}
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

    Memory card management

***************************************************************************/

/*-------------------------------------------------
    memcard_name - determine the name of a memcard
    file
-------------------------------------------------*/

INLINE void memcard_name(int index, char *buffer)
{
	sprintf(buffer, "memcard.%03d", index);
}


/*-------------------------------------------------
    memcard_create - create a new memory card with
    the given index
-------------------------------------------------*/

int memcard_create(int index, int overwrite)
{
	mame_file *file;
	char name[16];

	/* create a name */
	memcard_name(index, name);

	/* if we can't overwrite, fail if the file already exists */
	if (!overwrite)
	{
		file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MEMCARD, FALSE);
		if (file != NULL)
		{
			mame_fclose(file);
			return 1;
		}
	}

	/* create a new file */
	file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MEMCARD, TRUE);
	if (file == NULL)
		return 1;

	/* initialize and then save the card */
	if (Machine->drv->memcard_handler)
		(*Machine->drv->memcard_handler)(file, MEMCARD_CREATE);

	/* close the file */
	mame_fclose(file);
	return 0;
}


/*-------------------------------------------------
    memcard_insert - insert an existing memory card
    with the given index
-------------------------------------------------*/

int memcard_insert(int index)
{
	mame_file *file;
	char name[16];

	/* if a card is already inserted, eject it first */
	if (memcard_inserted != -1)
		memcard_eject();
	assert(memcard_inserted == -1);

	/* create a name */
	memcard_name(index, name);

	/* open the file; if we can't, it's an error */
	file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MEMCARD, FALSE);
	if (file == NULL)
		return 1;

	/* initialize and then load the card */
	if (Machine->drv->memcard_handler)
		(*Machine->drv->memcard_handler)(file, MEMCARD_INSERT);

	/* close the file */
	mame_fclose(file);
	memcard_inserted = index;
	return 0;
}


/*-------------------------------------------------
    memcard_eject - eject a memory card, saving
    its contents along the way
-------------------------------------------------*/

void memcard_eject(void)
{
	mame_file *file;
	char name[16];

	/* if no card is preset, just ignore */
	if (memcard_inserted == -1)
		return;

	/* create a name */
	memcard_name(memcard_inserted, name);

	/* open the file; if we can't, it's an error */
	file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MEMCARD, TRUE);
	if (file == NULL)
	{
		mame_fclose(file);
		return;
	}

	/* initialize and then load the card */
	if (Machine->drv->memcard_handler)
		(*Machine->drv->memcard_handler)(file, MEMCARD_EJECT);

	/* close the file */
	mame_fclose(file);
	memcard_inserted = -1;
}


/*-------------------------------------------------
    memcard_present - return the currently loaded
    card index, or -1 if none
-------------------------------------------------*/

int memcard_present(void)
{
	return memcard_inserted;
}



/***************************************************************************

    LED code

***************************************************************************/

/*-------------------------------------------------
    set_led_status - set the state of a given LED
-------------------------------------------------*/

void set_led_status(int num, int on)
{
	if (on)
		leds_status |=	(1 << num);
	else
		leds_status &= ~(1 << num);
}
