#ifndef NAMCOIO_H
#define NAMCOIO_H

enum
{
	NAMCOIO_56XX = 1,
	NAMCOIO_58XX,
	NAMCOIO_PACNPAL
};

#define MAX_NAMCOIO 4


struct namcoio_interface
{
	read8_handler in[4];	/* read handlers for ports A-D */
	write8_handler out[2];	/* write handlers for ports A-B */
};


READ8_HANDLER( namcoio_r );
WRITE8_HANDLER( namcoio_w );
void namcoio_init(int chipnum, int type, const struct namcoio_interface *intf);
void namcoio_set_reset_line(int chipnum, int state);
void namcoio_set_irq_line(int chipnum, int state);


#endif
