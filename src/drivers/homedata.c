/***************************************************************************

Homedata Games:

1987 Mahjong Hourouki Part1 -Seisyun Hen-
1987 Mahjong Hourouki Gaiden
1988 Mahjong Clinic
1988 Mahjong Joshi Pro-wres -Give up 5 byou mae-
1988 Mahjong Hourouki Okite
1988 Reikai Doushi / Chinese Exorcist
1989 Mahjong Kojin Kyouju (Private Teacher)
1989 Battle Cry (not released in Japan)
1989 Mahjong Vitamin C
1989 Mahjong Rokumeikan
1989 Mahjong Yougo no Kiso Tairyoku
1990 Mahjong Lemon Angel
1990 Mahjong Kinjirareta Asobi -Ike Ike Kyoushi no Yokubou-

----------------------------------------------------------------------------
Reikai Doushi (Chinese Exorcist)
aka Last Apostle Puppet Show (US)
(c)1988 HOME DATA

CPU   : 68B09E
SOUND : YM2203 Y8950(?)
OSC.  : 16.000MHz 9.000MHz

- One of sound chip seems Y8950 but its surface was erased.

----------------------------------------------------------------------------

Mahjong Kojinkyouju (Private Teacher)
(c)1989 HOME DATA

CPU	:6809
Sound	:SN76489 uPC324C
OSC	:16.000MHz 9.000MHz

Notes:

These games use only tilemaps for graphics.  These tilemaps are organized into
two pages.  It isn't known how the displayed/working page toggles.  In the
Reikai Doushi emulation, it is hacked to toggle every frame and seems to behave
correctly.

A blitter unpacks graphics data into videoram.  Its function is mostly
understood, but there may be glitches with certain opcodes.

Blitting appears to be just one function of a more powerful coprocessor.
The coprocessor manages input ports and sound/music.

When commands are written to the coprocessor, the main cpu polls the coprocessor
busy bit until is is 1 (indicating that the coprocessor has received the command
and is processing it), then waits until the coprocessor busy bit is 0 (indicating
that the coprocessor is finished handling/queuing the command).

It probably wouldn't be too hard to pick out the sample-related commands and
play the appropriate samples from sound ROMs, but unless music data is also
externally stored there, we will need music samples or a dump of the internal
coprocessor code to complete emulation for these games.

Reikai Doushi seems to be fully playable, except for the missing sound.

Private Teacher has a few issues:
- palette banking is sometimes wrong
- input ports/dips are largely untested
- the paging mechanism isn't working, so it's been modified to always display
  and draw to page#0.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"

static data8_t	dipswitch;
static data8_t	coprocessor_command;	/* contains most-recent write to 0x8002 */
static int		control_bitcount;		/* used to manage port 0x7803 bits */
static data8_t	*homedata_io_ram;		/* pointer to RAM associated with 0x780x */
static data8_t	*homedata_vreg;			/* pointer to RAM associated with 0x7ffx */
static int		homedata_cocktail;
static struct	tilemap *tilemap[2][4];
static data8_t	pteacher_blitter_bank;
static data8_t	reikaids_gfx_bank[2];
static int		blitter_param_count;
static data8_t	blitter_param[4];		/* buffers last 4 writes to 0x8006 */
static int		homedata_visible_page;

WRITE_HANDLER( reikaids_videoram_w );
VIDEO_START( reikaids );
PALETTE_INIT( reikaids );
PALETTE_INIT( pteacher );
VIDEO_UPDATE( reikaids );

/***************************************************************************
		Reikai Doushi (aka Last Apostle Puppet Show)
***************************************************************************/

PALETTE_INIT( reikaids )
{
	int i;

	/* initialize 555 RGB palette */
	for (i = 0; i < 0x8000; i++)
	{
		int r,g,b;
		int color = color_prom[i*2] * 256 + color_prom[i*2+1];
		/* xxxx------------ green
		 * ----xxxx-------- red
		 * --------xxxx---- blue
		 * ------------x--- green
		 * -------------x-- red
		 * --------------x- blue
		 * ---------------x unused
		 */
		g = ((color >> 11) & 0x1e) | ((color >> 3) & 1);
		r = ((color >>  7) & 0x1e) | ((color >> 2) & 1);
		b = ((color >>  3) & 0x1e) | ((color >> 1) & 1);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(i,r,g,b);
	}
}

WRITE_HANDLER( reikaids_gfx_bank_w )
{
	/* reikaids_gfx_bank[0]:
	 *		-xxx.x---	layer#1
	 *		----.-xxx	layer#3
	 *
	 * reikaids_gfx_bank[1]:
	 *		xxxx.x---	layer#0
	 *		----.-xxx	layer#2
	 */
	static int which;

	logerror( "[setbank %02x]\n",data );

	if( reikaids_gfx_bank[which] != data )
	{
		reikaids_gfx_bank[which] = data;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}
	which = 1-which;
}

WRITE_HANDLER( reikaids_videoram_w )
{
	if( videoram[offset] != data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( tilemap[offset/0x2000][offset&3],(offset/4)&0x3ff );
	}
}

static void reikaids_get_tile_info0( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = 0 + tile_index*4 + homedata_visible_page*0x2000;
	attr = videoram[addr];
	code = videoram[addr + 0x1000] + 256*(attr&3);
	color = (attr&0x7c)>>2;
	code += (reikaids_gfx_bank[1]>>3)*0x400;

	flags = homedata_cocktail;
	if( attr&0x80 )
	{
		flags ^= TILE_FLIPX;
	}
	SET_TILE_INFO( 3, code, color, flags );
	tile_info.priority = (color!=0)?1:0;
}

static void reikaids_get_tile_info1( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = 1 + tile_index*4 + homedata_visible_page*0x2000;
	attr = videoram[addr];
	code = videoram[addr + 0x1000] + 256*(attr&3);
	color = (attr&0x7c)>>2;
	code += ((reikaids_gfx_bank[0]&0x78)>>3)*0x400;

	flags = homedata_cocktail;
	if( attr&0x80 )
	{
		flags ^= TILE_FLIPX;
	}
	SET_TILE_INFO( 2, code, color, flags );
	tile_info.priority = (color!=0)?1:0;
}

static void reikaids_get_tile_info2( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = 2 + tile_index*4 + homedata_visible_page*0x2000;
	attr = videoram[addr];
	code = videoram[addr + 0x1000] + 256*(attr&3);
	color = (attr&0x7c)>>2;
	code += (reikaids_gfx_bank[1]&0x7)*0x400;

	flags = homedata_cocktail;
	if( attr&0x80 )
	{
		flags ^= TILE_FLIPX;
	}
	SET_TILE_INFO( 1, code, color, flags );
}

static void reikaids_get_tile_info3( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = 3 + tile_index*4 + homedata_visible_page*0x2000;
	attr = videoram[addr];
	code = videoram[addr + 0x1000] + 256*(attr&3);
	color = (attr&0x7c)>>2;
	code += (reikaids_gfx_bank[0]&0x7)*0x400;

	flags = homedata_cocktail;
	if( attr&0x80 )
	{
		flags ^= TILE_FLIPX;
	}
	SET_TILE_INFO( 0, code, color, flags );
	tile_info.priority = (color!=0)?1:0;
}

static void reikaids_handleblit( void )
{
	int i,col0,row0,row,col,offs;
	UINT16 SourceParam,DestParam;
	int select,layer,code;
	data8_t *pBlitData;
	int flipx;
	int SourceAddr, BaseAddr;
	int DestAddr;

	int opcode,data,NumTiles;

	pBlitData = memory_region( REGION_USER1 );

	DestParam =
		blitter_param[(blitter_param_count-4)&3]*256+
		blitter_param[(blitter_param_count-3)&3];

	SourceParam =
		blitter_param[(blitter_param_count-2)&3]*256+
		blitter_param[(blitter_param_count-1)&3];

	/*	x---.----.----.---- flipx
	 * 	-x--.----.----.---- select: attr/tile
	 *	--xx.xxxx.----.---- row		(signed)
	 *	----.----.xxxx.xx-- col		(signed)
	 *	----.----.----.--xx layer	(0..3)
	 */
	layer	= (DestParam&0x0003);
	col0	= (DestParam&0x007c)>>2;
	row0	= (DestParam&0x1f00)>>8;
	select	= (DestParam&0x4000)>>14;
	flipx	= DestParam>>15;

	code	= homedata_vreg[0xe];
	/* 0x03: BlitData bank select
	 * 0x08: target page
	 * 0x20: ?
	 */

	logerror( "[blit %02x %04x]\n",code,SourceParam);

	if( DestParam&0x0080 )
	{
		col0 -= 32;
	}
	if( DestParam&0x2000 )
	{
		row0 -= 32;
	}

	SourceAddr = SourceParam + (code&3)*0x10000;
	BaseAddr = select*0x1000+layer;

	if( homedata_visible_page == 0 )
	{
		BaseAddr += 0x2000;
	}

	offs = 0;
	for(;;)
	{
		opcode = pBlitData[SourceAddr++];
		/* 00 xxxxxx	Raw Run
		 * 01 xxxxxx	RLE incrementing
		 * 11 xxxxxx	RLE constant data
		 * 10 xxxxxx	skip?
		 */
		if( opcode == 0x00 )
		{
			/* end-of-graphic */
			return;
		}

		data  = pBlitData[SourceAddr++];

		if( (opcode&0xc0)==0x80 )
		{
			NumTiles = 0x80 - (opcode&0x7f);
			offs += NumTiles;
		}
		else
		{
			NumTiles = 0x40 - (opcode&0x3f);
			for( i=0; i<NumTiles; i++ )
			{
				if( i!=0 )
				{
					switch( opcode&0xc0 )
					{
					case 0x00: // Raw Run
						data  = pBlitData[SourceAddr++];
						break;

					case 0x40: // RLE incrementing
						data++;
						break;
					}
				} /* i!=0 */

				row = row0 + (offs/64);
				col = col0 + (offs%64);
				{
					int dat = data;
					if( flipx )
					{
						if( select==0 ) dat |= 0x80;
						col = 31-col;
					}
					if( row>=0 && row<32 && col>=0 && col<32 )
					{
						DestAddr = BaseAddr + (row*32+(col%32))*4;
						reikaids_videoram_w( DestAddr, dat );
					}
				}
				offs++;
			} /* for( i=0; i<NumTiles; i++ ) */
		}
	}
}


VIDEO_START( reikaids )
{
	int i;
	for( i=0; i<2; i++ )
	{
		tilemap[i][0] = tilemap_create( reikaids_get_tile_info0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32 );
		tilemap[i][1] = tilemap_create( reikaids_get_tile_info1, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32 );
		tilemap[i][2] = tilemap_create( reikaids_get_tile_info2, tilemap_scan_rows, TILEMAP_OPAQUE,	  8, 8, 32, 32 );
		tilemap[i][3] = tilemap_create( reikaids_get_tile_info3, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32 );

		if (tilemap[0] && tilemap[1] && tilemap[2] && tilemap[3])
		{
			tilemap_set_transparent_pen(tilemap[i][0],0xff);
			tilemap_set_transparent_pen(tilemap[i][1],0xff);
			tilemap_set_transparent_pen(tilemap[i][3],0xff);
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

VIDEO_UPDATE( reikaids )
{
	int flags;

	flags = (homedata_vreg[1]&0x80)?(TILE_FLIPX|TILE_FLIPY):0;
	if( flags!=homedata_cocktail )
	{
		homedata_cocktail = flags;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}

	/* background, player score/health, shadows */
	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][2], 0, 0);

	/* insert coin, country warning, white backdrop for title screen, boss#3 */
	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][0], 1, 0);

	/* player sprite, continue screen */
	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][3], 1, 0);

	/* enemy sprite, high scores */
	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][1], 1, 0);
}

/***************************************************************************
	Mahjong Kojinkyouju (Private Teacher)
***************************************************************************/

PALETTE_INIT( pteacher )
{
	int i;

	/* initialize 555 RGB palette */
	for (i = 0; i < 0x8000; i++)
	{
		int r,g,b;
		int color = color_prom[i*2] * 256 + color_prom[i*2+1];
		/* xxxxx----------- green
		 * -----xxxxx------ red
		 * ----------xxxxx- blue
		 * ---------------x unused
		 */
		g = ((color >> 11) & 0x1f);
		r = ((color >>  6) & 0x1f);
		b = ((color >>  1) & 0x1f);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(i,r,g,b);
	}
}

WRITE_HANDLER( pteacher_gfx_bank_w )
{
	logerror( "gfxbank:=%02x\n", data );
	if( reikaids_gfx_bank[1] != data )
	{
		reikaids_gfx_bank[1] = data;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}
}

WRITE_HANDLER( pteacher_videoram_w )
{
	if( videoram[offset] != data )
	{
		videoram[offset] = data;
		if( offset<0x2000 )
		{
			tilemap_mark_tile_dirty( tilemap[offset/0x2000][offset/0x1000], (offset&0xfff)/2 );
		}
	}
}

static WRITE_HANDLER( pteacher_blitter_bank_w )
{
	pteacher_blitter_bank = data;
}

static void pteacher_handleblit( void )
{
	int i;
	int SourceParam,DestParam;
	int select,row0,col0,layer,code;
	data8_t *pBlitData;
	int SourceAddr;
	int DestAddr, BaseAddr;
//	int flipx;
	int offs,row,col;
	int opcode,data,NumTiles;

	pBlitData = memory_region( REGION_USER1 );

	DestParam =
		blitter_param[(blitter_param_count-4)&3]*256+
		blitter_param[(blitter_param_count-3)&3];

	SourceParam =
		blitter_param[(blitter_param_count-2)&3]*256+
		blitter_param[(blitter_param_count-1)&3];

	/*  x---.----.----.---- flip?
	 *  -x--.----.----.---- layer	(0..1)
	 *	--xx.xxxx.----.---- row
	 *	----.----.xxxx.xxx- col
	 *	----.----.----.---x select: attr/tile
	 */
	layer	= (DestParam&0x4000)>>14;
	col0	= (DestParam&0x007e)>>1;
	row0	= (DestParam&0x1f00)>>8;
	select	= (DestParam&0x0001);

	if( DestParam&0x0080 ) col0 -= 64;
	if( DestParam&0x2000 ) row0 -= 32;

	BaseAddr = select+layer*0x1000;

	code = pteacher_blitter_bank;

	logerror( "[blit %02x %04x]->%d\n",code,SourceParam,homedata_visible_page);

	SourceAddr = SourceParam + (code>>4)*0x8000;

	if( homedata_visible_page == 0 )
	{
		BaseAddr += 0x2000;
	}

	offs = 0;
	for(;;)
	{
		opcode = pBlitData[SourceAddr++];
		/* 00 xxxxxx	Raw Run
		 * 01 xxxxxx	RLE incrementing
		 * 10 xxxxxx	skip
		 * 11 xxxxxx	RLE constant data
		 */
		if( opcode == 0x00 )
		{
 			/* end-of-graphic */
 			return;
		}
		data  = pBlitData[SourceAddr++];
		NumTiles = 0x40-(opcode&0x3f);

		if( (opcode&0xc0) == 0x80 )
		{
			offs += NumTiles;
		}
		else
		{
			for( i=0; i<NumTiles; i++ )
			{
				if( i!=0 )
				{
					switch( opcode&0xc0 )
					{
					case 0x00: // Raw Run
						data  = pBlitData[SourceAddr++];
						break;

					case 0x40: // RLE incrementing
						data++;
						break;
					}
				} /* i!=0 */

				col = (offs%64)+col0;
				row = (offs/64)+row0;
				if( col>=0 && col<64 && row>=0 && row<32 )
				{
					DestAddr = BaseAddr + (row*64+col)*2;
					pteacher_videoram_w( DestAddr, data );
				}

				offs++;
			} /* for( i=0; i<NumTiles; i++ ) */
		} /* opaque run */
	} /* for(;;) */
}

static void pteacher_get_tile_info0( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = tile_index*2;
	attr = videoram[addr];
	code = videoram[addr + 1] + 256*(attr&7);
	code += (reikaids_gfx_bank[1]&0xf)*0x800;
	color = attr>>3;
//	if( reikaids_gfx_bank[1]&0x04 ) color += 0x20;

	flags = homedata_cocktail;
	SET_TILE_INFO( 0, code, color, flags );
}

static void pteacher_get_tile_info1( int tile_index )
{
	int code,addr,color,attr,flags;

	addr = tile_index*2+0x1000;
	attr = videoram[addr];
	code = videoram[addr + 1] + 256*(attr&7);
	code += ((reikaids_gfx_bank[1]&0xf0)>>4)*0x800;
	color = attr>>3;
//	if( reikaids_gfx_bank[1]&0x40 ) color += 0x20;

	flags = homedata_cocktail;
	SET_TILE_INFO( 1, code, color, flags );
	tile_info.priority = (color==0)?0:1;
}

VIDEO_START( pteacher )
{
	int i;
	for( i=0; i<2; i++ )
	{
		tilemap[i][0] = tilemap_create( pteacher_get_tile_info0, tilemap_scan_rows, TILEMAP_OPAQUE, 	 8, 8, 64,32 );
		tilemap[i][1] = tilemap_create( pteacher_get_tile_info1, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64,32 );
		if( tilemap[0] && tilemap[1] )
		{
			tilemap_set_transparent_pen(tilemap[i][1],0xff);
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

VIDEO_UPDATE( pteacher )
{
	int flags;

	homedata_visible_page = 0;
//	logerror( "refresh(%d)\n",homedata_visible_page );

	flags = (homedata_vreg[1]&0x80)?(TILE_FLIPX|TILE_FLIPY):0;
	if( flags!=homedata_cocktail )
	{
		homedata_cocktail = flags;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}

	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][0], 0, 0);
	tilemap_draw(bitmap, cliprect, tilemap[homedata_visible_page][1], 1, 0);

	homedata_visible_page = 1;
}

/********************************************************************************/

static WRITE_HANDLER( pteacher_coprocessor_command_w )
{
	coprocessor_command = data;

	if( data == 0x56 )
	{
		control_bitcount = 0;
	}
}

static READ_HANDLER( pteacher_vreg_r )
{
	int data;

	data = homedata_vreg[offset];

	switch( offset )
	{
	case 2:
		if( control_bitcount/3 > 2 )
		{
			control_bitcount = 0;
		}

		switch( control_bitcount%3 )
		{
		case 0:
			data = 0x80;
			break;

		case 1:
			data = readinputport(control_bitcount/3);
			break;

		case 2:
			data = 0x00;
			break;
		}
		control_bitcount++;
		break;
	}
	return data;
}

static WRITE_HANDLER( pteacher_7ff0_w )
{
	homedata_vreg[offset] = data;

	if( offset==0xf )
	{
		pteacher_handleblit();
	}
}

READ_HANDLER( pteacher_io_r )
{
	int pc;

	switch( offset )
	{
	case 0:
		/* normal RAM */
		return homedata_io_ram[0];

	case 1:
		/* 0x40: vblank
		 * 0x80: visible page
		 */
		pc = activecpu_get_pc();
		switch( pc )
		{
		case 0xed13: /* visible page */
			return homedata_visible_page*0x80;

		case 0xf90d: /* vblank */
			logerror( "iloop:%d\n", cpu_getiloops() );
			if( cpu_getiloops()==15 )
			{
				homedata_visible_page = 1;//-homedata_visible_page;
				return 0x00;
			}
			return 0x40;

		default:
			logerror( "unknown read from 7801; pc=0x%04x\n", pc );
			break;
		}
		break;

	default:
		break;
	}
	return 0;
}

static WRITE_HANDLER( reikaids_coprocessor_command_w )
{
	static int which;

	if( data == 0xea ) which = 0;

	coprocessor_command = data;
	control_bitcount = 0x00;

	if( data == 0x80 )
	{
		dipswitch = ~readinputport(3+which);
		which = 1-which;
	}
}

static READ_HANDLER( reikaids_vreg_r )
{
	return homedata_vreg[offset];
}

static WRITE_HANDLER( reikaids_7ff0_w )
{
	homedata_vreg[offset] = data;
	if( offset==0xf )
	{
		reikaids_handleblit();
	}
}

READ_HANDLER( reikaids_io_r )
{
	int pc;
	data8_t data;

	switch (offset)
	{
	case 0:
		/* behaves as normal RAM */
		data = homedata_io_ram[0];
		break;

	case 1:
		data = readinputport(0); /* player#1 controls */
		break;

	case 2:
		data = readinputport(1); /* player#2 controls */
		break;

	case 3:
		pc = activecpu_get_pc();

		data = readinputport(2);
		switch( pc )
		{
		case 0x9385: /* coin input */
		case 0x938c: /* coin input */
		case 0x9395: /* coin input */
		case 0x939e: /* coin input */
			return data;

		case 0x93ee: /* visible page */
			return homedata_visible_page*0x08;

		case 0x9cfc: /* sound */
		case 0x9d26: /* sound */
		case 0x9d50: /* sound */
			return 1;

		case 0x9d12: /* sound */
		case 0x9d3c: /* sound */
		case 0x9d66: /* sound */
			return 0;

		case 0xc2a0: /* vblank */
			if( cpu_getiloops()==0 )
			{
				homedata_visible_page = 1-homedata_visible_page;
				return 4;
			}
			else
			{
				return 0;
			}
			break;
		}

		switch( coprocessor_command )
		{
		case 0xdc:
			/* 0x64 reads; least significant bit must be 0 */
			break;

		case 0xea:
			/* 10 reads, value must change */
			data = control_bitcount&1;
			control_bitcount++;
			control_bitcount &= 0xff;
			break;

		case 0x80:
			/* read dip switches one bit at a time */
			switch( control_bitcount%3 )
			{
			case 0: data = 1; break;
			case 1: data = 0; break;
			case 2:
				switch( control_bitcount/3 )
				{
				case 0:
				case 10:
				case 11:
					data = 2;
					break;

				case 1:
				case 12:
					data = 0;
					break;

				default:
					data = ( ( dipswitch>>(control_bitcount/3-2) )&1)*2;
					break;
				}
				break;
			}
			control_bitcount++;
			break;

		default:
			logerror( "unknown read from 0x7803; pc=0x%04x\n", pc );
			break;
		}
		break;

	default:
		data = 0;
		break;
	}

	return data;
}

/********************************************************************************/
/* common functions */

static WRITE_HANDLER( blitter_param_w )
{
	blitter_param[blitter_param_count] = data;
	blitter_param_count++;
	blitter_param_count&=3;
}

static WRITE_HANDLER( homedata_bankswitch_w )
{

	data8_t *RAM = memory_region(REGION_CPU1);
	if( data<7 )
	{
		cpu_setbank(1, &RAM[data*0x4000 + 0x10000]);
	}
	else
	{
		cpu_setbank(1, &RAM[0xc000] );
	}
}

/********************************************************************************/

MEMORY_READ_START( reikaids_readmem )
	{ 0x0000, 0x3fff, MRA_RAM }, /* videoram */
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM }, /* work ram */
	{ 0x7800, 0x7803, reikaids_io_r },
	{ 0x7ff0, 0x7fff, reikaids_vreg_r },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( reikaids_writemem )
	{ 0x0000, 0x3fff, reikaids_videoram_w, &videoram },
	{ 0x4000, 0x5eff, MWA_RAM },
	{ 0x5f00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7800, 0x7803, MWA_RAM, &homedata_io_ram },
	{ 0x7ff0, 0x7fff, reikaids_7ff0_w, &homedata_vreg },
	{ 0x8000, 0x8000, homedata_bankswitch_w },
	{ 0x8001, 0x8001, MWA_ROM },
	{ 0x8002, 0x8002, reikaids_coprocessor_command_w },
	{ 0x8003, 0x8004, MWA_ROM },
	{ 0x8005, 0x8005, reikaids_gfx_bank_w },
	{ 0x8006, 0x8006, blitter_param_w },
	{ 0x8007, 0xffff, MWA_ROM },
MEMORY_END

/**************************************************************************/

MEMORY_READ_START( pteacher_readmem )
	{ 0x0000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6fff, MRA_RAM }, /* work ram */
	{ 0x7800, 0x7803, pteacher_io_r },
	{ 0x7ff0, 0x7fff, pteacher_vreg_r },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START( pteacher_writemem )

	{ 0x0000, 0x1fff, pteacher_videoram_w, &videoram },
	{ 0x2000, 0x3fff, MWA_RAM }, /* videoram2 */
	{ 0x4000, 0x5eff, MWA_RAM },
	{ 0x5f00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7800, 0x7803, MWA_RAM, &homedata_io_ram },
	{ 0x7ff0, 0x7fff, pteacher_7ff0_w, &homedata_vreg },
	{ 0x8000, 0x8000, homedata_bankswitch_w },
	{ 0x8001, 0x8001, MWA_ROM },
	{ 0x8002, 0x8002, pteacher_coprocessor_command_w },
	{ 0x8003, 0x8004, MWA_ROM },
	{ 0x8005, 0x8005, pteacher_blitter_bank_w },
	{ 0x8006, 0x8006, blitter_param_w },
	{ 0x8007, 0x8007, pteacher_gfx_bank_w },
	{ 0x8008, 0xffff, MWA_ROM },
MEMORY_END

/**************************************************************************/

/* 8x8x8 tiles */
static struct GfxLayout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8, /* bits per pixel */
	{0,1,2,3,4,5,6,7}, /* plane offsets */
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8}, /* x offsets */
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64}, /* y offsets */
	8*8*8
};

static struct GfxDecodeInfo reikaids_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,      0x0000, 0x20 },
	{ REGION_GFX2, 0, &tile_layout,      0x2000, 0x20 },
	{ REGION_GFX3, 0, &tile_layout,      0x4000, 0x20 },
	{ REGION_GFX4, 0, &tile_layout,      0x6000, 0x20 },
	{ -1 }
};

static struct GfxDecodeInfo pteacher_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,      0x0000, 0x40 },
	{ REGION_GFX2, 0, &tile_layout,      0x4000, 0x40 },
	{ -1 }
};

/**************************************************************************/

static struct YM2203interface reikaids_ym2203_interface=
{
	1,
	3000000,	/* ? */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0	},
	{ 0 },
	{ NULL }
};

static struct Y8950interface reikaids_y8950_interface =
{
	1,
	3000000,	/* ? */
	{ 100 },
	{ 0 },
	{ REGION_SOUND1 }, /* ROM region */
	{ 0 },  /* keyboard read  */
	{ 0 },  /* keyboard write */
	{ 0 },  /* I/O read  */
	{ 0 }   /* I/O write */
};

static MACHINE_DRIVER_START( reikaids )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 4000000)	/* ? */
	MDRV_CPU_MEMORY(reikaids_readmem,reikaids_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 255-16)
	MDRV_GFXDECODE(reikaids_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH(0x8000)

	MDRV_PALETTE_INIT(reikaids)
	MDRV_VIDEO_START(reikaids)
	MDRV_VIDEO_UPDATE(reikaids)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, reikaids_ym2203_interface)
	MDRV_SOUND_ADD(Y8950, reikaids_y8950_interface)
MACHINE_DRIVER_END


/**************************************************************************/

static struct SN76496interface pteacher_sn76496_interface =
{
	1,
	{ 2000000 },	 /* ? */
	{ 100 }
};

static MACHINE_DRIVER_START( pteacher )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 4000000)	/* ? */
	MDRV_CPU_MEMORY(pteacher_readmem,pteacher_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,16)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1-32)
	MDRV_GFXDECODE(pteacher_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_COLORTABLE_LENGTH(0x8000)

	MDRV_PALETTE_INIT(pteacher)
	MDRV_VIDEO_START(pteacher)
	MDRV_VIDEO_UPDATE(pteacher)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, pteacher_sn76496_interface)	// SN76489 actually
MACHINE_DRIVER_END

/**************************************************************************/

ROM_START( reikaids )
	ROM_REGION( 0x02c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "j82c01.bin", 0x010000, 0x01c000, 0x50fcc451 )
	ROM_CONTINUE(           0x00c000, 0x004000             )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a05.bin",  0x00000, 0x80000, 0xfb65e0e0 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a08.bin",  0x00000, 0x80000, 0x51cfd790 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a09.bin",  0x00000, 0x80000, 0xc496b187 )
	ROM_LOAD( "x82a10.bin",  0x80000, 0x80000, 0x4243fe28 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "x82a13.bin",  0x000000, 0x80000, 0x954c8844 )
	ROM_LOAD( "x82a14.bin",  0x080000, 0x80000, 0xa748305e )
	ROM_LOAD( "x82a15.bin",  0x100000, 0x80000, 0xc50f7047 )
	ROM_LOAD( "x82a16.bin",  0x180000, 0x80000, 0xb270094a )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* 8 bit unsigned samples */
	ROM_LOAD( "x82a04.bin", 0x000000, 0x040000, 0x52c9028a )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "e82a18.bin", 0x00000, 0x8000, 0x1f52a7aa )
	ROM_LOAD16_BYTE( "e82a17.bin", 0x00001, 0x8000, 0xf91d77a1 ) // FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x82a02.bin", 0x00000, 0x040000, 0x90fe700f )

	ROM_REGION( 0x000100, REGION_USER2, 0 )	/* unknown */
	ROM_LOAD( "x82a19.bin", 0x0000, 0x100, 0x7ed947b4 )	// FIXED BITS (0000xxx10000xxxx)
ROM_END

ROM_START( mjkojink )
	ROM_REGION( 0x02c000, REGION_CPU1, 0 ) /* 6809 Code */
	ROM_LOAD( "x83j01.16e", 0x010000, 0xc000, 0x91f90376 )
	ROM_CONTINUE(           0x00c000, 0x4000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x83b14.1f",  0, 0x80000, 0x701254b9 )
	ROM_LOAD32_BYTE( "x83b15.3f",  1, 0x80000, 0xc03751a7 )
	ROM_LOAD32_BYTE( "x83b16.4f",  2, 0x80000, 0x62da6309 )
	ROM_LOAD32_BYTE( "x83b17.6f",  3, 0x80000, 0x144f0229 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "x83b10.1c",  0, 0x80000, 0xfb33032e )
	ROM_LOAD32_BYTE( "x83b11.3c",  1, 0x80000, 0x6f495fb6 )
	ROM_LOAD32_BYTE( "x83b12.4c",  2, 0x80000, 0x01af893b )
	ROM_LOAD32_BYTE( "x83b13.6c",  3, 0x80000, 0xb430f91c )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* 4 bit adpcm */
	ROM_LOAD( "x83b02.9g",  0x00000, 0x80000, 0x45d396c3 )

	ROM_REGION( 0x010000, REGION_PROMS, 0 )	/* static palette */
	ROM_LOAD16_BYTE( "x83a19.4k", 0x00000, 0x8000, 0xd29c9ef0 )
	ROM_LOAD16_BYTE( "x83a18.3k", 0x00001, 0x8000, 0xc3351952 )	// FIXED BITS (xxxxxxx0)

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* blitter data */
	ROM_LOAD( "x83b03.12e", 0x0000, 0x80000, 0xaa28893d )
ROM_END

/**************************************************************************/

INPUT_PORTS_START( reikaids )
	PORT_START	// IN0  - 0x7801
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* punch */
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) /* kick */
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 ) /* jump */
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	// IN1 - 0x7802
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* punch */
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* kick */
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) /* jump */
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - 0x7803
	PORT_BIT(  0x01, IP_ACTIVE_HIGH,IPT_UNKNOWN ) /* coprocessor status */
	PORT_BIT(  0x02, IP_ACTIVE_HIGH,IPT_UNKNOWN ) /* coprocessor data */
	PORT_BIT(  0x04, IP_ACTIVE_HIGH,IPT_UNKNOWN ) /* IPT_VBLANK? */
	PORT_BIT(  0x08, IP_ACTIVE_LOW,	IPT_UNKNOWN ) /* visible page */
	PORT_BIT(  0x10, IP_ACTIVE_LOW,	IPT_COIN1    )
	PORT_BIT(  0x20, IP_ACTIVE_LOW,	IPT_SERVICE1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW,	IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW,	IPT_UNKNOWN  )

	PORT_START	// IN4 -
	PORT_DIPNAME( 0x01, 0x01, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k then every 60k" )
	PORT_DIPSETTING(    0x02, "30k then every 80k" )
	PORT_DIPSETTING(    0x04, "20k" )
	PORT_DIPSETTING(    0x06, "30k" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, "Time" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x20, "45" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	// IN3 -
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_5C ) )
INPUT_PORTS_END

INPUT_PORTS_START( pteacher ) /* all unconfirmed except where noted! */
	PORT_START
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "DIP1 0x01" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, "DIP1 0x06" )
	PORT_DIPSETTING(    0x00, "0x00" )
	PORT_DIPSETTING(    0x02, "0x02" )
	PORT_DIPSETTING(    0x04, "0x04" )
	PORT_DIPSETTING(    0x05, "0x05" )
	PORT_DIPNAME( 0x08, 0x00, "DIP1 0x08" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIP1 0x10" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )

	PORT_START
	PORT_BIT(  0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) ) /* confirmed */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT(  0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 ) /* confirmed */
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 ) /* confirmed */
	PORT_BIT(  0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/**************************************************************************/

/*    YEAR  NAME      PARENT   MACHINE   INPUT     INIT  MONITOR  COMPANY      FULLNAME */
GAMEX( 1988, reikaids, 0,       reikaids, reikaids, 0,    0,      "Home Data", "Reikai Doushi (Japan)", GAME_NO_SOUND )
GAMEX( 1989, mjkojink, 0,       pteacher, pteacher, 0,    0,      "Home Data", "Mahjong Kojinkyouju (Private Teacher)", GAME_NOT_WORKING )
