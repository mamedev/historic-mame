/***************************************************************************

	Sega 16-bit common hardware

***************************************************************************/

/* multiply chip */
READ16_HANDLER( segaic16_multiply_0_r );
READ16_HANDLER( segaic16_multiply_1_r );
WRITE16_HANDLER( segaic16_multiply_0_w );
WRITE16_HANDLER( segaic16_multiply_1_w );

/* divide chip */
READ16_HANDLER( segaic16_divide_0_r );
READ16_HANDLER( segaic16_divide_1_r );
WRITE16_HANDLER( segaic16_divide_0_w );
WRITE16_HANDLER( segaic16_divide_1_w );

/* compare/timer chip */
void compare_timer_init(void (*sound_write_callback)(data8_t data), void (*timer_ack_callback)(void));
int compare_timer_clock(void);
READ16_HANDLER( segaic16_compare_timer_0_r );
READ16_HANDLER( segaic16_compare_timer_1_r );
WRITE16_HANDLER( segaic16_compare_timer_0_w );
WRITE16_HANDLER( segaic16_compare_timer_1_w );

