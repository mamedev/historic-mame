/***************************************************************************

	PSX SPU

	preliminary version by smf.

***************************************************************************/

#if !defined( PSX_SPU_H )

extern int PSX_sh_start( const struct MachineSound *msound );
extern void PSX_sh_stop( void );
extern void PSX_sh_reset( void );
WRITE32_HANDLER( psx_spu_w );
READ32_HANDLER( psx_spu_r );
WRITE32_HANDLER( psx_spu_delay_w );
READ32_HANDLER( psx_spu_delay_r );

typedef void ( *spu_handler )( UINT32, INT32 );

struct PSXSPUinterface
{
	int mixing_level;
	data32_t **p_psxram;
	void (*irq_set)(UINT32);
	void (*spu_install_read_handler)(int,spu_handler);
	void (*spu_install_write_handler)(int,spu_handler);
};

#define PSX_SPU_H ( 1 )
#endif
