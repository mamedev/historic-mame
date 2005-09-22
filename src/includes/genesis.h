/* Todo, reorganise, cleanup etc.*/

#include "sound/okim6295.h"
#include "sound/sn76496.h"
#include "sound/2612intf.h"
#include "sound/upd7759.h"

/* In src/drivers/segac2.c */
extern void vdp_reload_counter(int scanline);

/* In src/drivers/megaplay.c */
extern UINT16 *ic36_ram;
extern unsigned char bios_6204;
extern unsigned char bios_ctrl[6];

/* In src/drivers/megatech.c */
extern unsigned int bios_ctrl_inputs;
extern READ8_HANDLER (megatech_bios_port_be_bf_r);
extern WRITE8_HANDLER (megatech_bios_port_be_bf_w);
extern INTERRUPT_GEN (megatech_irq);

/* In src/drivers/segac2.c */
extern WRITE16_HANDLER( sn76489_w );
extern INTERRUPT_GEN( vblank_interrupt );
extern DRIVER_INIT( segac2 );
extern MACHINE_INIT( segac2 );
extern struct YM3438interface segac2_ym3438_intf;

/* In src/drivers/genesis.c */
extern unsigned char *genesis_z80_ram;
extern UINT16 *genesis_68k_ram;
extern MACHINE_INIT( genesis );
extern READ16_HANDLER ( megaplay_genesis_io_r );
extern WRITE16_HANDLER ( genesis_io_w );
extern UINT16 *genesis_io_ram;
extern READ16_HANDLER(genesis_ctrl_r);
extern READ16_HANDLER ( megaplay_68k_to_z80_r );
extern READ8_HANDLER ( genesis_z80_r );
extern READ8_HANDLER ( genesis_z80_bank_r );
extern WRITE8_HANDLER ( genesis_z80_w );
extern WRITE16_HANDLER(genesis_ctrl_w);
extern WRITE16_HANDLER ( megaplay_68k_to_z80_w );
extern READ16_HANDLER ( genesis_io_r );
extern READ16_HANDLER ( genesis_68k_to_z80_r );

/* Generic Input Ports */
#define GENESIS_PORTS \
	PORT_START \
 \
	PORT_START	/* Player 1 Controls - part 1 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 1 Controls - part 2 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 2 Controls - part 1 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) \
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) \
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  ) PORT_PLAYER(2) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \
 \
	PORT_START	/* Player 2 Controls - part 2 */ \
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) \
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \


