#ifndef PALETTE_H
#define PALETTE_H

#define MAX_PENS 256	/* unless 16-bit mode is used, can't handle more */
						/* than 256 colors on screen */

int palette_start(void);
void palette_stop(void);
void palette_init(void);
void palette_change_color(int color,unsigned char red, unsigned char green, unsigned char blue);

#endif
