/****************************************************************************
 *																			*
 *	Function prototypes and constants used by the TMS34061 emulator			*
 *																			*
 *  Created by Zsolt Vasvari on 5/26/1998.	                                *
 *																			*
 ****************************************************************************/

/* Register addressing modes (DO NOT change the values!) */

#define TMS34061_REG_ADDRESS_NORMAL	2	/* As laid out by the User's Guide */

#define TMS34061_REG_ADDRESS_PACKED	1	/* A0 is actually connected to the */
										/* TMS34061 A1, so the high/lo     */
										/* bytes of the registers can be   */
										/* accessed by a single word operation */

/* Callback prototypes for accessing pixels */
typedef int  (*tms34061_getpixel_t)(int x, int y);
typedef void (*tms34061_setpixel_t)(int x, int y, int pixel);

/* Initializes the emulator */
int tms34061_start(int reg_addr_mode,  /* One of the addressing mode constants above */
				   int cpu,			   /* Which CPU is the TMS34061 causing interrupts on */
                   tms34061_getpixel_t getpixel,  /* Function to get a pixel in X/Y addressing */
                   tms34061_setpixel_t setpixel); /* Function to set a pixel in X/Y addressing */

/* Cleans up the emulation */
void tms34061_stop(void);

/* Write to a 34061 register */
void tms34061_w(int offset, int data);

/* Read a 34061 register */
int tms34061_r(int offset);

/* Write a pixel currently addressed by the XY registers */
void tms34061_xypixel_w(int offset, int data);

/* Read a pixel currently addressed by the XY registers */
int tms34061_xypixel_r(int offset);

/* Checks whether the display is inhibited */
int tms34061_display_blanked(void);

