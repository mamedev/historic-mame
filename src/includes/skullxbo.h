/*************************************************************************

	Atari Skull & Crossbones hardware

*************************************************************************/

/*----------- defined in vidhrdw/skullxbo.c -----------*/

WRITE16_HANDLER( skullxbo_playfieldlatch_w );
WRITE16_HANDLER( skullxbo_hscroll_w );
WRITE16_HANDLER( skullxbo_vscroll_w );
WRITE16_HANDLER( skullxbo_mobmsb_w );

VIDEO_START( skullxbo );
VIDEO_UPDATE( skullxbo );

void skullxbo_scanline_update(int param);
