/******************************************************************************

  tiny.c

  driver.c substitute file for "tiny" MAME builds.

******************************************************************************/

#include "driver.h"

/* The "root" driver, defined so we can have &driver_##NAME in macros. */
struct GameDriver driver_0 =
{
	__FILE__,
	0,
	"",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NOT_A_DRIVER
};

extern struct GameDriver TINY_NAME;

const struct GameDriver *drivers[] =
{
	TINY_POINTER,
	0	/* end of array */
};
