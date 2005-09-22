/*************************************************************************

    Sega System C/C2 Driver

**************************************************************************/

/******************************************************************************
    Constants
******************************************************************************/

#define MASTER_CLOCK		53693100

/*----------- defined in vidhrdw/segac2.c -----------*/

extern UINT8		segac2_vdp_regs[];
extern UINT16		segac2_bg_pal_lookup[];
extern UINT16		segac2_sp_pal_lookup[];
extern UINT16		scanbase;

VIDEO_START( puckpkmn );
VIDEO_START( megatech );
VIDEO_START( megaplay );

VIDEO_START( segac2 );

VIDEO_UPDATE( segac2 );
VIDEO_UPDATE( megatech );
VIDEO_UPDATE( megaplay );

void	segac2_enable_display(int enable);

READ16_HANDLER ( segac2_vdp_r );
WRITE16_HANDLER( segac2_vdp_w );
