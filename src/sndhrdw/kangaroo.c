/* CHANGELOG

   97/06/19  - minor cleanup -V-
   97/04/xx  - renamed the arabian.c to kangaroo.c, only function names
               were changed to protect the innocent ;-) -V-
*/
#include "driver.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw\generic.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1,	/* 1 update per video frame (low quality) */
	1250000000,     /* 1.250000000 MHZ?????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int kangaroo_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
