/* C140.h */

#ifndef _NAMCO_C140_
#define _NAMCO_C140_

READ8_HANDLER( C140_r );
WRITE8_HANDLER( C140_w );

enum
{
	C140_TYPE_SYSTEM2,
	C140_TYPE_SYSTEM21_A,
	C140_TYPE_SYSTEM21_B
};

struct C140interface {
    int banking_type;
    int region;
};

#endif
