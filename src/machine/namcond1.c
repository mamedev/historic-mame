/***************************************************************************

  Namco ND-1

  machine.c

  Functions to emulate general aspects of the machine
  (RAM, ROM, interrupts, I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "namcond1.h"

/* Perform basic machine initialisation */

UINT8 namcond1_h8_irq5_enabled;
int namcond1_gfxbank;

MACHINE_INIT( namcond1 )
{
#ifdef MAME_DEBUG
    /*unsigned char   *ROM = memory_region(REGION_CPU1);*/
    /*unsigned int debug_trigger_addr;*/
    /*int             i;*/

#if 0
    // debug trigger patch
    // insert a "move.b $B0000000,D2" into the code
    debug_trigger_addr = 0x152d4; // after ygv_init
    ROM[debug_trigger_addr++] = 0x39;
    ROM[debug_trigger_addr++] = 0x14;
    ROM[debug_trigger_addr++] = 0xB0;
    ROM[debug_trigger_addr++] = 0x00;
    ROM[debug_trigger_addr++] = 0x00;
    ROM[debug_trigger_addr++] = 0x00;
#endif
#endif

    // initialise MCU states
    namcond1_h8_irq5_enabled = 0;

    // halt the MCU
    cpunum_set_input_line(1,INPUT_LINE_HALT,ASSERT_LINE);
}

// instance of the shared ram pointer
data16_t *namcond1_shared_ram;

READ16_HANDLER( namcond1_shared_ram_r )
{
	return namcond1_shared_ram[offset];
}

// $c3ff00-$c3ffff
READ16_HANDLER( namcond1_cuskey_r )
{
    switch( offset )
    {
        // this address returns a jump vector inside ISR2
        // - if zero then the ISR returns without jumping
        case (0x2e>>1):
            return( 0x0000 );
        case (0x30>>1):
            return( 0x0000 );

        default :
            logerror( "offset $%X accessed from $%X\n",
                      offset<<1, activecpu_get_pc() );
            return( 0 );
    }
}

WRITE16_HANDLER( namcond1_shared_ram_w )
{

    switch( offset )
    {
        default :
            COMBINE_DATA( namcond1_shared_ram + offset );
            break;
    }
}

WRITE16_HANDLER( namcond1_cuskey_w )
{
    switch( offset )
    {
        case (0x0a>>1):
            // this is a kludge until we emulate the h8
	    if ((namcond1_h8_irq5_enabled == 0) && (data != 0x0000))
	    {
		cpunum_resume(1, SUSPEND_REASON_HALT);
		cpunum_reset(1,NULL,NULL); /* immediate */
	    }
            namcond1_h8_irq5_enabled = ( data != 0x0000 );
            break;

		case (0x0c>>1):
			namcond1_gfxbank = (data & 0x0002) >>1; // i think
			// should mark tilemaps dirty but i think they already are
			break;

        default :
            break;
    }
}

#define NAMCOND1_EEPROM_SIZE    0x200
data16_t *namcond1_eeprom;

/* not used at this point */
NVRAM_HANDLER( namcond1 )
{
  if( file == 0 )
    return;

  if( read_or_write == 0 ) {
    /* read eeprom contents */
    mame_fread( file, namcond1_eeprom, NAMCOND1_EEPROM_SIZE );
  }
  else {
    /* write eeprom contents */
    mame_fwrite( file, namcond1_eeprom, NAMCOND1_EEPROM_SIZE );
  }
}

