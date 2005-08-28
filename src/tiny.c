/******************************************************************************

  tiny.c

  driver.c substitute file for "tiny" MAME builds.

******************************************************************************/

#include "driver.h"

/* The "root" driver, defined so we can have &driver_##NAME in macros. */
game_driver driver_0 =
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

extern game_driver TINY_NAME;

const game_driver *drivers[] =
{
	TINY_POINTER,
	0	/* end of array */
};
