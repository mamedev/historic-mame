/* CHANGELOG

   97/04/xx  renamed the arabian.c to kangaroo.c, only function names
             were changed to protect the innocent ;-) -V-
*/
#include "driver.h"
#include "Z80.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw\generic.h"



static struct AY8910interface interface =
{
	1,	/* 1 chip */
	1832727040,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ },
	{ },
	{ },
	{ }
};



int kangaroo_sh_start(void)
{
	return AY8910_sh_start(&interface);
}
