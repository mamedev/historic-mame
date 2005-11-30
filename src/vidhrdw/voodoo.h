/*************************************************************************

    3dfx Voodoo Graphics SST-1 emulator

    driver by Aaron Giles

**************************************************************************/

#define MAX_VOODOO			2

/* enumeration specifying which model of Voodoo we are emulating */
enum
{
	VOODOO_1,
	VOODOO_2,
	VOODOO_BANSHEE,
	VOODOO_3
};

int voodoo_start(int which, int type, int fbmem_in_mb, int tmem0_in_mb, int tmem1_in_mb);
void voodoo_update(int which, mame_bitmap *bitmap, const rectangle *cliprect);
void voodoo_reset(int which);
int voodoo_get_type(int which);
int voodoo_is_stalled(int which);
void voodoo_set_init_enable(int which, UINT32 newval);
void voodoo_set_vblank_callback(int which, void (*vblank)(int));
void voodoo_set_unstall_callback(int which, void (*unstall)(void));

READ32_HANDLER( voodoo_0_r );
WRITE32_HANDLER( voodoo_0_w );

READ32_HANDLER( voodoo_1_r );
WRITE32_HANDLER( voodoo_1_w );
