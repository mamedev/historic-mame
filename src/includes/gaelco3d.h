/*************************************************************************

	Driver for Gaelco 3D games

	driver by Aaron Giles

**************************************************************************/

/* should be 1, but can increased to 2 or 3 for speed */
#define GAELCO3D_RESOLUTION_DIVIDE	1


/*----------- defined in vidhrdw/gaelco3d.c -----------*/

extern offs_t gaelco3d_mask_offset;
extern offs_t gaelco3d_mask_size;

void gaelco3d_render(void);
WRITE32_HANDLER( gaelco3d_render_w );

WRITE16_HANDLER( gaelco3d_paletteram_w );
WRITE32_HANDLER( gaelco3d_paletteram_020_w );

VIDEO_START( gaelco3d );
VIDEO_UPDATE( gaelco3d );
