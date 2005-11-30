//fix me -- some calspeed sky still busted
//fix me -- mace text is alpha blended too lightly
//fix me -- carnevil depth buffer not correct
//fix me -- blitz2k dies when starting a game with heavy fog (in DRC)
//fix me -- stats busted in konami games
//optimize -- 565 writes with no pipeline
//optimize -- 64-bit multiplies for S/T

/*************************************************************************

    3dfx Voodoo Graphics SST-1/2 emulator

    emulator by Aaron Giles

    --------------------------

    Specs:

    Voodoo 1:
        2,4MB frame buffer RAM
        1,2,4MB texture RAM
        50MHz clock frequency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        64 entry PCI FIFO
        memory FIFO up to 65536 entries

    Voodoo 2:
        2,4MB frame buffer RAM
        2,4,8,16MB texture RAM
        75MHz clock frquency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        ultrafast clears @ 16 pixels/clock
        128 entry PCI FIFO
        memory FIFO up to 65536 entries

    --------------------------

    still to be implemented:
        * clutData gamma correction
        * trilinear textures

    things to verify:
        * floating Z buffer


    Other notes:
        * Voodoo 1 has 4 bit bilinear resolution
        * Voodoo 2 "increased bilinear resolution to 8 bits"

iterated RGBA = 12.12 [24 bits]
iterated Z    = 20.12 [32 bits]
iterated W    = 18.32 [48 bits]

>mamepm blitz
Stall PCI for HWM: 1
PCI FIFO Empty Entries LWM: D
LFB -> FIFO: 1
Texture -> FIFO: 1
Memory FIFO: 1
Memory FIFO HWM: 2000
Memory FIFO Write Burst HWM: 36
Memory FIFO LWM for PCI: 5
Memory FIFO row start: 120
Memory FIFO row rollover: 3FF
Video dither subtract: 0
DRAM banking: 1
Triple buffer: 0
Video buffer offset: 60
DRAM banking: 1

>mamepm wg3dh
Stall PCI for HWM: 1
PCI FIFO Empty Entries LWM: D
LFB -> FIFO: 1
Texture -> FIFO: 1
Memory FIFO: 1
Memory FIFO HWM: 2000
Memory FIFO Write Burst HWM: 36
Memory FIFO LWM for PCI: 5
Memory FIFO row start: C0
Memory FIFO row rollover: 3FF
Video dither subtract: 0
DRAM banking: 1
Triple buffer: 0
Video buffer offset: 40
DRAM banking: 1


As a point of reference, the 3D engine uses the following algorithm to calculate the linear memory address as a
function of the video buffer offset (fbiInit2 bits(19:11)), the number of 32x32 tiles in the X dimension (fbiInit1
bits(7:4) and bit(24)), X, and Y:

    tilesInX[4:0] = {fbiInit1[24], fbiInit1[7:4], fbiInit6[30]}
    rowBase = fbiInit2[19:11]
    rowStart = ((Y>>5) * tilesInX) >> 1

    if (!(tilesInX & 1))
    {
        rowOffset = (X>>6);
        row[9:0] = rowStart + rowOffset (for color buffer 0)
        row[9:0] = rowBase + rowStart + rowOffset (for color buffer 1)
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for depth/alpha buffer when double color buffering[fbiInit5[10:9]=0])
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for color buffer 2 when triple color buffering[fbiInit5[10:9]=1 or 2])
        row[9:0] = (rowBase<<1) + rowBase + rowStart + rowOffset (for depth/alpha buffer when triple color buffering[fbiInit5[10:9]=2])
        column[8:0] = ((Y % 32) <<4) + ((X % 32)>>1)
        ramSelect[1] = ((X&0x20) ? 1 : 0) (for color buffers)
        ramSelect[1] = ((X&0x20) ? 0 : 1) (for depth/alpha buffers)
    }
    else
    {
        rowOffset = (!(Y&0x20)) ? (X>>6) : ((X>31) ? (((X-32)>>6)+1) : 0)
        row[9:0] = rowStart + rowOffset (for color buffer 0)
        row[9:0] = rowBase + rowStart + rowOffset (for color buffer 1)
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for depth/alpha buffer when double color buffering[fbiInit5[10:9]=0])
        row[9:0] = (rowBase<<1) + rowStart + rowOffset (for color buffer 2 when triple color buffering[fbiInit5[10:9]=1 or 2])
        row[9:0] = (rowBase<<1) + rowBase + rowStart + rowOffset (for depth/alpha buffer when triple color buffering[fbiInit5[10:9]=2])
        column[8:0] = ((Y % 32) <<4) + ((X % 32)>>1)
        ramSelect[1] = (((X&0x20)^(Y&0x20)) ? 1 : 0) (for color buffers)
        ramSelect[1] = (((X&0x20)^(Y&0x20)) ? 0 : 1) (for depth/alpha buffers)
    }
    ramSelect[0] = X % 2
    pixelMemoryAddress[21:0] = (row[9:0]<<12) + (column[8:0]<<3) + (ramSelect[1:0]<<1)
    bankSelect = pixelMemoryAddress[21]

**************************************************************************/

#ifndef EXPAND_RASTERIZERS
#define EXPAND_RASTERIZERS

#include "driver.h"
#include "voodoo.h"
#include "vooddefs.h"
#include <math.h>


/*************************************
 *
 *  Debugging
 *
 *************************************/

#define LOG_FIFO			(0)
#define LOG_FIFO_VERBOSE	(0)
#define LOG_REGISTERS		(0)
#define LOG_LFB				(0)
#define LOG_TEXTURE_RAM		(0)
#define LOG_RASTERIZERS		(0)



/*************************************
 *
 *  Statics
 *
 *************************************/

static voodoo_state *voodoo[MAX_VOODOO];

/* fast reciprocal+log2 lookup */
UINT32 reciplog[(2 << RECIPLOG_LOOKUP_BITS) + 2];



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void init_fbi(voodoo_state *v, fbi_state *f, int fbmem);
static void init_tmu_shared(tmu_shared_state *s);
static void init_tmu(tmu_state *t, tmu_shared_state *s, int type, voodoo_reg *reg, int tmem);
static void soft_reset(voodoo_state *v);
static void check_stalled_cpu(voodoo_state *v, mame_time current_time);
static void flush_fifos(voodoo_state *v, mame_time current_time);
static void stall_cpu_callback(void *param);
static void stall_cpu(voodoo_state *v, int state, mame_time current_time);
static void vblank_callback(void *param);
static UINT32 voodoo_r(voodoo_state *v, offs_t offset);
static void voodoo_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask);
static INT32 fastfill(voodoo_state *v);
static INT32 swapbuffer(voodoo_state *v, UINT32 data);
static INT32 triangle(voodoo_state *v);
static raster_info *add_rasterizer(voodoo_state *v, const raster_info *cinfo);
static raster_info *find_rasterizer(voodoo_state *v, int texcount);
static void dump_rasterizer_stats(voodoo_state *v);

static void raster_generic_0tmu(voodoo_state *v, UINT16 *drawbuf);
static void raster_generic_1tmu(voodoo_state *v, UINT16 *drawbuf);
static void raster_generic_2tmu(voodoo_state *v, UINT16 *drawbuf);



/*************************************
 *
 *  Specific rasterizers
 *
 *************************************/

#define RASTERIZER_ENTRY(fbzcp, alpha, fog, fbz, tex0, tex1) \
	RASTERIZER(fbzcp##_##alpha##_##fog##_##fbz##_##tex0##_##tex1, (((tex0) == 0xffffffff) ? 0 : ((tex1) == 0xffffffff) ? 1 : 2), fbzcp, fbz, alpha, fog, tex0, tex1)

#include "voodoo.c"

#undef RASTERIZER_ENTRY



/*************************************
 *
 *  Rasterizer table
 *
 *************************************/

#define RASTERIZER_ENTRY(fbzcp, alpha, fog, fbz, tex0, tex1) \
	{ NULL, raster_##fbzcp##_##alpha##_##fog##_##fbz##_##tex0##_##tex1, FALSE, 0, 0, fbzcp, alpha, fog, fbz, tex0, tex1 },

static const raster_info predef_raster_table[] =
{
#include "voodoo.c"
	{ 0 }
};

#undef RASTERIZER_ENTRY



/*************************************
 *
 *  Main create routine
 *
 *************************************/

int voodoo_start(int which, int type, int fbmem_in_mb, int tmem0_in_mb, int tmem1_in_mb)
{
	voodoo_state *v;
	int val;

	/* allocate memory */
	v = auto_malloc(sizeof(*v));
	memset(v, 0, sizeof(*v));
	voodoo[which] = v;

	/* create a table of precomputed 1/n and log2(n) values */
	/* n ranges from 1.0000 to 2.0000 */
	for (val = 0; val <= (1 << RECIPLOG_LOOKUP_BITS); val++)
	{
		UINT32 value = (1 << RECIPLOG_LOOKUP_BITS) + val;
		reciplog[val*2 + 0] = (1 << (RECIPLOG_LOOKUP_PREC + RECIPLOG_LOOKUP_BITS)) / value;
		reciplog[val*2 + 1] = (UINT32)(LOGB2((double)value / (double)(1 << RECIPLOG_LOOKUP_BITS)) * (double)(1 << RECIPLOG_LOOKUP_PREC));
	}

	/* set the type, and initialize the chip mask */
	v->index = which;
	v->type = type;
	v->chipmask = 0x01;
	v->freq = (type == VOODOO_1) ? 50000000 : 75000000;
	v->subseconds_per_cycle = MAX_SUBSECONDS / v->freq;
	printf("subseconds/cycle = %08X%08X\n", (UINT32)(v->subseconds_per_cycle >> 32), (UINT32)v->subseconds_per_cycle);
	v->trigger = 51324 + which;

	/* build the rasterizer table */
#ifndef VOODOO_DRC
{
	const raster_info *info;
	for (info = predef_raster_table; info->callback; info++)
		add_rasterizer(v, info);
}
#endif

	/* set up the PCI FIFO */
	v->pci.fifo.base = v->pci.fifo_mem;
	v->pci.fifo.size = 64*2;
	v->pci.fifo.in = v->pci.fifo.out = 0;
	v->pci.stall_state = NOT_STALLED;
	v->pci.continue_timer = timer_alloc_ptr(stall_cpu_callback, v);

	/* set up frame buffer */
	init_fbi(v, &v->fbi, fbmem_in_mb << 20);

	/* build shared TMU tables */
	init_tmu_shared(&v->tmushare);

	/* set up the TMUs */
	init_tmu(&v->tmu[0], &v->tmushare, type, &v->reg[0x100], tmem0_in_mb << 20);
	v->chipmask |= 0x02;
	if (tmem1_in_mb)
	{
		init_tmu(&v->tmu[1], &v->tmushare, type, &v->reg[0x200], tmem1_in_mb << 20);
		v->chipmask |= 0x04;
	}

	/* initialize some registers */
	memset(v->reg, 0, sizeof(v->reg));
	v->pci.init_enable = 0;
	v->reg[fbiInit0].u = (1 << 4) | (0x10 << 6);
	v->reg[fbiInit1].u = (1 << 1) | (1 << 8) | (1 << 12) | (2 << 20);
	v->reg[fbiInit2].u = (1 << 6) | (0x100 << 23);
	v->reg[fbiInit3].u = (2 << 13) | (0xf << 17);
	v->reg[fbiInit4].u = (1 << 0);

	/* do a soft reset to reset everything else */
	soft_reset(v);
	return 0;
}



/*************************************
 *
 *  Video update
 *
 *************************************/

void voodoo_update(int which, mame_bitmap *bitmap, const rectangle *cliprect)
{
	voodoo_state *v = voodoo[which];
	int x, y;

	/* if we are blank, just fill with black */
	if (FBIINIT1_SOFTWARE_BLANK(v->reg[fbiInit1].u))
	{
		fillbitmap(bitmap, 0, cliprect);
		return;
	}

	/* otherwise, copy from the current front buffer */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *src = v->fbi.rgb[v->fbi.frontbuf] + y * v->fbi.rowpixels;
		UINT32 *dst = (UINT32 *)bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dst[x] = v->fbi.pen[src[x]];
	}

	/* display stats */
	if (v->stats.display)
		ui_draw_text(v->stats.buffer, 0, 0);
}



/*************************************
 *
 *  Chip reset
 *
 *************************************/

void voodoo_reset(int which)
{
	soft_reset(voodoo[which]);
}


int voodoo_get_type(int which)
{
	return voodoo[which]->type;
}


int voodoo_is_stalled(int which)
{
	return (voodoo[which]->pci.stall_state != NOT_STALLED);
}


void voodoo_set_init_enable(int which, UINT32 newval)
{
	voodoo[which]->pci.init_enable = newval;
	if (LOG_REGISTERS)
		logerror("VOODOO.%d.REG:initEnable write = %08X\n", which, newval);
}


void voodoo_set_vblank_callback(int which, void (*vblank)(int))
{
	voodoo[which]->fbi.vblank_client = vblank;
}


void voodoo_set_unstall_callback(int which, void (*unstall)(void))
{
	voodoo[which]->pci.unstall_callback = unstall;
}



/*************************************
 *
 *  Specific write handlers
 *
 *************************************/

WRITE32_HANDLER( voodoo_0_w )
{
	voodoo_w(voodoo[0], offset, data, mem_mask);
}


WRITE32_HANDLER( voodoo_1_w )
{
	voodoo_w(voodoo[1], offset, data, mem_mask);
}



/*************************************
 *
 *  Specific read handlers
 *
 *************************************/

READ32_HANDLER( voodoo_0_r )
{
	return voodoo_r(voodoo[0], offset);
}


READ32_HANDLER( voodoo_1_r )
{
	return voodoo_r(voodoo[1], offset);
}



/*************************************
 *
 *  Common initialization
 *
 *************************************/

static void init_fbi(voodoo_state *v, fbi_state *f, int fbmem)
{
	int pen;

	/* allocate frame buffer RAM and set pointers */
	f->ram = auto_malloc(fbmem);
	f->mask = fbmem - 1;
	f->rgb[0] = f->rgb[1] = f->rgb[2] = f->ram;
	f->aux = f->ram;

	/* default to 0x0 */
	f->frontbuf = 0;
	f->backbuf = 1;
	f->width = 512;
	f->height = 384;

	/* init the pens */
	for (pen = 0; pen < 65536; pen++)
	{
		int r = (pen >> 11) & 0x1f;
		int g = (pen >> 5) & 0x3f;
		int b = pen & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		f->pen[pen] = MAKE_RGB(r, g, b);
	}

	/* allocate a VBLANK timer */
	f->vblank_timer = timer_alloc_ptr(vblank_callback, v);
	f->vblank = FALSE;

	/* initialize the memory FIFO */
	f->fifo.base = f->ram;
	f->fifo.size = f->fifo.in = f->fifo.out = 0;
}


static void init_tmu_shared(tmu_shared_state *s)
{
	int val;

	/* build static 8-bit texel tables */
	for (val = 0; val < 256; val++)
	{
		int r, g, b, a;

		/* 8-bit RGB (3-3-2) */
		EXTRACT_332_TO_888(val, r, g, b);
		s->rgb332[val] = MAKE_ARGB(0xff, r, g, b);

		/* 8-bit alpha */
		s->alpha8[val] = MAKE_ARGB(val, val, val, val);

		/* 8-bit intensity */
		s->int8[val] = MAKE_ARGB(0xff, val, val, val);

		/* 8-bit alpha, intensity */
		a = ((val >> 0) & 0xf0) | ((val >> 4) & 0x0f);
		r = ((val << 4) & 0xf0) | ((val << 0) & 0x0f);
		s->ai44[val] = MAKE_ARGB(a, r, r, r);
	}

	/* build static 16-bit texel tables */
	for (val = 0; val < 65536; val++)
	{
		int r, g, b, a;

		/* table 10 = 16-bit RGB (5-6-5) */
		EXTRACT_565_TO_888(val, r, g, b);
		s->rgb565[val] = MAKE_ARGB(0xff, r, g, b);

		/* table 11 = 16 ARGB (1-5-5-5) */
		EXTRACT_1555_TO_8888(val, a, r, g, b);
		s->argb1555[val] = MAKE_ARGB(a, r, g, b);

		/* table 12 = 16-bit ARGB (4-4-4-4) */
		EXTRACT_4444_TO_8888(val, a, r, g, b);
		s->argb4444[val] = MAKE_ARGB(a, r, g, b);
	}
}


static void init_tmu(tmu_state *t, tmu_shared_state *s, int type, voodoo_reg *reg, int tmem)
{
	/* allocate texture RAM */
	t->ram = auto_malloc(tmem);
	t->mask = tmem - 1;
	t->reg = reg;
	t->regdirty = TRUE;

	/* mark the NCC tables dirty and configure their registers */
	t->ncc[0].dirty = t->ncc[1].dirty = TRUE;
	t->ncc[0].reg = &t->reg[nccTable+0];
	t->ncc[1].reg = &t->reg[nccTable+12];

	/* create pointers to all the tables */
	t->texel[0] = s->rgb332;
	t->texel[1] = t->ncc[0].texel;
	t->texel[2] = s->alpha8;
	t->texel[3] = s->int8;
	t->texel[4] = s->ai44;
	t->texel[5] = t->palette;
	t->texel[6] = (type >= VOODOO_2) ? t->palettea : NULL;
	t->texel[7] = NULL;
	t->texel[8] = s->rgb332;
	t->texel[9] = t->ncc[0].texel;
	t->texel[10] = s->rgb565;
	t->texel[11] = s->argb1555;
	t->texel[12] = s->argb4444;
	t->texel[13] = s->int8;
	t->texel[14] = t->palette;
	t->texel[15] = NULL;
	t->lookup = t->texel[0];

	/* attach the palette to NCC table 0 */
	t->ncc[0].palette = t->palette;
	if (type >= VOODOO_2)
		t->ncc[0].palettea = t->palettea;
}



/*************************************
 *
 *  VBLANK management
 *
 *************************************/

static void swap_buffers(voodoo_state *v)
{
	int statskey;
	int count;

	logerror("--- swap_buffers @ %d\n", cpu_getscanline());

	/* force a partial update */
	force_partial_update(cpu_getscanline());

	/* keep a history of swap intervals */
	count = v->fbi.vblank_count;
	if (count > 15)
		count = 15;
	v->reg[fbiSwapHistory].u = (v->reg[fbiSwapHistory].u << 4) | count;

	/* rotate the buffers */
	if (v->type < VOODOO_2 || !v->fbi.vblank_dont_swap)
	{
		if (v->fbi.rgb[2] == NULL)
		{
			v->fbi.frontbuf = 1 - v->fbi.frontbuf;
			v->fbi.backbuf = 1 - v->fbi.frontbuf;
		}
		else
		{
			v->fbi.frontbuf = (v->fbi.frontbuf + 1) % 3;
			v->fbi.backbuf = (v->fbi.frontbuf + 1) % 3;
		}
	}

	/* decrement the pending count and reset our state */
	v->fbi.swaps_pending--;
	v->fbi.vblank_count = 0;
	v->fbi.vblank_swap_pending = FALSE;

	/* reset the last_op_time to now and start processing the next command */
	if (v->pci.op_pending)
	{
		v->pci.op_end_time = mame_timer_get_time();
		flush_fifos(v, v->pci.op_end_time);
	}

	/* we may be able to unstall now */
	if (v->pci.stall_state != NOT_STALLED)
		check_stalled_cpu(v, mame_timer_get_time());

	/* periodically log rasterizer info */
	v->stats.swaps++;
	if (LOG_RASTERIZERS && v->stats.swaps % 100 == 0)
		dump_rasterizer_stats(v);

	/* update the statistics (debug) */
	statskey = (code_pressed(KEYCODE_BACKSLASH) != 0);
	if (statskey && statskey != v->stats.lastkey)
		v->stats.display = !v->stats.display;
	v->stats.lastkey = statskey;

	if (v->stats.display)
	{
		int screen_area = (Machine->visible_area.max_x - Machine->visible_area.min_x + 1) * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
		int pixelcount = v->reg[fbiPixelsOut].u - v->stats.start_pixels_out;
		char *statsptr = v->stats.buffer;
		int i;

		statsptr += sprintf(statsptr, "Swap:%6d\n", v->stats.swaps);
		statsptr += sprintf(statsptr, "Hist:%08X\n", v->reg[fbiSwapHistory].u);
		statsptr += sprintf(statsptr, "Stal:%6d\n", v->stats.stalls);
		statsptr += sprintf(statsptr, "Rend:%6d%%\n", pixelcount * 100 / screen_area);
		statsptr += sprintf(statsptr, "Poly:%6d\n", v->reg[fbiTrianglesOut].u - v->stats.start_triangles);
		statsptr += sprintf(statsptr, "PxIn:%6d\n", v->reg[fbiPixelsIn].u - v->stats.start_pixels_in);
		statsptr += sprintf(statsptr, "POut:%6d\n", v->reg[fbiPixelsOut].u - v->stats.start_pixels_out);
		statsptr += sprintf(statsptr, "Chro:%6d\n", v->reg[fbiChromaFail].u - v->stats.start_chroma_fail);
		statsptr += sprintf(statsptr, "ZFun:%6d\n", v->reg[fbiZfuncFail].u - v->stats.start_zfunc_fail);
		statsptr += sprintf(statsptr, "AFun:%6d\n", v->reg[fbiAfuncFail].u - v->stats.start_afunc_fail);
		statsptr += sprintf(statsptr, "RegW:%6d\n", v->stats.reg_writes);
		statsptr += sprintf(statsptr, "RegR:%6d\n", v->stats.reg_reads);
		statsptr += sprintf(statsptr, "LFBW:%6d\n", v->stats.lfb_writes);
		statsptr += sprintf(statsptr, "LFBR:%6d\n", v->stats.lfb_reads);
		statsptr += sprintf(statsptr, "TexW:%6d\n", v->stats.tex_writes);
		statsptr += sprintf(statsptr, "TexM:");
		for (i = 0; i < 16; i++)
			*statsptr++ = (v->stats.texture_mode[i]) ? "0123456789ABCDEF"[i] : ' ';
		*statsptr = 0;
	}

	/* update statistics */
	v->stats.start_triangles = v->reg[fbiTrianglesOut].u;
	v->stats.start_pixels_in = v->reg[fbiPixelsIn].u;
	v->stats.start_pixels_out = v->reg[fbiPixelsOut].u;
	v->stats.start_chroma_fail = v->reg[fbiChromaFail].u;
	v->stats.start_zfunc_fail = v->reg[fbiZfuncFail].u;
	v->stats.start_afunc_fail = v->reg[fbiAfuncFail].u;
	v->stats.reg_writes = 0;
	v->stats.reg_reads = 0;
	v->stats.lfb_writes = 0;
	v->stats.lfb_reads = 0;
	v->stats.tex_writes = 0;
	memset(v->stats.texture_mode, 0, sizeof(v->stats.texture_mode));
}


static void vblank_off_callback(void *param)
{
	voodoo_state *v = param;

	/* set internal state and call the client */
	v->fbi.vblank = FALSE;
	if (v->fbi.vblank_client)
		(*v->fbi.vblank_client)(FALSE);

	/* go to the end of the next frame */
	timer_adjust_ptr(v->fbi.vblank_timer, cpu_getscanlinetime(Machine->visible_area.max_y + 1), TIME_NEVER);
}


static void vblank_callback(void *param)
{
	voodoo_state *v = param;

	/* flush the pipes */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	/* increment the count */
	v->fbi.vblank_count++;
	if (v->fbi.vblank_count > 250)
		v->fbi.vblank_count = 250;

	/* if we're past the swap count, do the swap */
	if (v->fbi.vblank_swap_pending && v->fbi.vblank_count >= v->fbi.vblank_swap)
		swap_buffers(v);

	/* set a timer for the next off state */
	timer_set_ptr(cpu_getscanlinetime(0), v, vblank_off_callback);

	/* set internal state and call the client */
	v->fbi.vblank = TRUE;
	if (v->fbi.vblank_client)
		(*v->fbi.vblank_client)(TRUE);
}



/*************************************
 *
 *  Chip reset
 *
 *************************************/

static void reset_counters(voodoo_state *v)
{
	v->reg[fbiPixelsIn].u = 0;
	v->reg[fbiChromaFail].u = 0;
	v->reg[fbiZfuncFail].u = 0;
	v->reg[fbiAfuncFail].u = 0;
	v->reg[fbiPixelsOut].u = 0;
	v->reg[fbiTrianglesOut].u = 0;
}


static void soft_reset(voodoo_state *v)
{
	reset_counters(v);
	fifo_reset(&v->fbi.fifo);
	fifo_reset(&v->pci.fifo);
}



/*************************************
 *
 *  Recompute video memory layout
 *
 *************************************/

static void recompute_video_memory(voodoo_state *v)
{
	UINT32 buffer_pages = FBIINIT2_VIDEO_BUFFER_OFFSET(v->reg[fbiInit2].u);
	UINT32 fifo_start_page = FBIINIT4_MEMORY_FIFO_START_ROW(v->reg[fbiInit4].u);
	UINT32 fifo_last_page = FBIINIT4_MEMORY_FIFO_STOP_ROW(v->reg[fbiInit4].u);
	UINT32 memory_config;
	int buf;

	/* memory config is determined differently between V1 and V2 */
	memory_config = FBIINIT2_ENABLE_TRIPLE_BUF(v->reg[fbiInit2].u);
	if (v->type == VOODOO_2 && memory_config == 0)
		memory_config = FBIINIT5_BUFFER_ALLOCATION(v->reg[fbiInit5].u);

	/* tiles are 64x16/32; x_tiles specifies how many half-tiles */
	v->fbi.tile_width = 64;
	v->fbi.tile_height = (v->type == VOODOO_1) ? 16 : 32;
	v->fbi.x_tiles = FBIINIT1_X_VIDEO_TILES(v->reg[fbiInit1].u);
	if (v->type == VOODOO_2)
	{
		v->fbi.x_tiles = (v->fbi.x_tiles << 1) |
						(FBIINIT1_X_VIDEO_TILES_BIT5(v->reg[fbiInit1].u) << 5) |
						(FBIINIT6_X_VIDEO_TILES_BIT0(v->reg[fbiInit6].u));
	}
	v->fbi.rowpixels = v->fbi.tile_width * v->fbi.x_tiles;

	logerror("VOODOO.%d.VIDMEM: buffer_pages=%X  fifo=%X-%X  tiles=%X  rowpix=%d\n", v->index, buffer_pages, fifo_start_page, fifo_last_page, v->fbi.x_tiles, v->fbi.rowpixels);

	/* first RGB buffer always starts at 0 */
	v->fbi.rgb[0] = v->fbi.ram;

	/* second RGB buffer starts immediately afterwards */
	v->fbi.rgb[1] = (UINT16 *)((UINT8 *)v->fbi.ram + buffer_pages * 0x1000);

	/* remaining buffers are based on the config */
	switch (memory_config)
	{
		case 3:	/* reserved */
			logerror("VOODOO.%d.ERROR:Unexpected memory configuration in recompute_video_memory!\n", v->index);

		case 0:	/* 2 color buffers, 1 aux buffer */
			v->fbi.rgb[2] = NULL;
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			break;

		case 1:	/* 3 color buffers, 0 aux buffers */
			v->fbi.rgb[2] = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			v->fbi.aux = NULL;
			break;

		case 2:	/* 3 color buffers, 1 aux buffers */
			v->fbi.rgb[2] = (UINT16 *)((UINT8 *)v->fbi.ram + 2 * buffer_pages * 0x1000);
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + 3 * buffer_pages * 0x1000);
			break;
	}

	/* clamp the RGB buffers to video memory */
	for (buf = 0; buf < 3; buf++)
	{
		v->fbi.rgbmax[buf] = 0;
		if (v->fbi.rgb[buf])
		{
			if ((UINT8 *)v->fbi.rgb[buf] > (UINT8 *)v->fbi.ram + v->fbi.mask)
				v->fbi.rgb[buf] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask);
			v->fbi.rgbmax[buf] = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.rgb[buf];
		}
	}

	/* clamp the aux buffer to video memory */
	v->fbi.auxmax = 0;
	if (v->fbi.aux)
	{
		if ((UINT8 *)v->fbi.aux > (UINT8 *)v->fbi.ram + v->fbi.mask)
			v->fbi.aux = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask);
		v->fbi.auxmax = (UINT16 *)((UINT8 *)v->fbi.ram + v->fbi.mask + 1) - v->fbi.aux;
	}

	logerror("VOODOO.%d.VIDMEM: rgbmax=%X,%X,%X  auxmax=%X\n", v->index, v->fbi.rgbmax[0], v->fbi.rgbmax[1], v->fbi.rgbmax[2], v->fbi.auxmax);

	/* compute the memory FIFO location and size */
	if (fifo_last_page > v->fbi.mask / 0x1000)
		fifo_last_page = v->fbi.mask / 0x1000;

	/* is it valid and enabled? */
	if (fifo_start_page <= fifo_last_page && FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
	{
		v->fbi.fifo.base = (UINT32 *)((UINT8 *)v->fbi.ram + fifo_start_page * 0x1000);
		v->fbi.fifo.size = (fifo_last_page + 1 - fifo_start_page) * 0x1000 / 4;
		if (v->fbi.fifo.size > 65536*2)
			v->fbi.fifo.size = 65536*2;
	}

	/* if not, disable the FIFO */
	else
	{
		v->fbi.fifo.base = NULL;
		v->fbi.fifo.size = 0;
	}

	/* reset the FIFO */
	fifo_reset(&v->fbi.fifo);

	/* reset our front buffer */
	v->fbi.frontbuf = 0;
	v->fbi.backbuf = 1;
}



/*************************************
 *
 *  NCC table management
 *
 *************************************/

static void ncc_table_write(ncc_table *n, offs_t regnum, UINT32 data)
{
	/* I/Q entries reference the plaette if the high bit is set */
	if (regnum >= 4 && (data & 0x80000000) && n->palette)
	{
		int index = ((data >> 23) & 0xfe) | (regnum & 1);

		/* set the ARGB for this palette index */
		n->palette[index] = 0xff000000 | data;

		/* if we have an ARGB palette as well, compute its value */
		if (n->palettea)
		{
			int a = ((data >> 16) & 0xfc) | ((data >> 22) & 0x03);
			int r = ((data >> 10) & 0xfc) | ((data >> 16) & 0x03);
			int g = ((data >>  4) & 0xfc) | ((data >> 10) & 0x03);
			int b = ((data <<  2) & 0xfc) | ((data >>  4) & 0x03);
			n->palettea[index] = MAKE_ARGB(a, r, g, b);
		}

		/* this doesn't dirty the table or go to the registers, so bail */
		return;
	}

	/* if the register matches, don't update */
	if (data == n->reg[regnum].u)
		return;
	n->reg[regnum].u = data;

	/* first four entries are packed Y values */
	if (regnum < 4)
	{
		regnum *= 4;
		n->y[regnum+0] = (data >>  0) & 0xff;
		n->y[regnum+1] = (data >>  8) & 0xff;
		n->y[regnum+2] = (data >> 16) & 0xff;
		n->y[regnum+3] = (data >> 24) & 0xff;
	}

	/* the second four entries are the I RGB values */
	else if (regnum < 8)
	{
		regnum &= 3;
		n->ir[regnum] = (INT32)(data <<  5) >> 23;
		n->ig[regnum] = (INT32)(data << 14) >> 23;
		n->ib[regnum] = (INT32)(data << 23) >> 23;
	}

	/* the final four entries are the Q RGB values */
	else
	{
		regnum &= 3;
		n->qr[regnum] = (INT32)(data <<  5) >> 23;
		n->qg[regnum] = (INT32)(data << 14) >> 23;
		n->qb[regnum] = (INT32)(data << 23) >> 23;
	}

	/* mark the table dirty */
	n->dirty = TRUE;
}


static void ncc_table_update(ncc_table *n)
{
	int r, g, b, i;

	/* generte all 256 possibilities */
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		/* start with the intensity */
		r = g = b = n->y[(i >> 4) & 0x0f];

		/* add the coloring */
		r += n->ir[vi] + n->qr[vq];
		g += n->ig[vi] + n->qg[vq];
		b += n->ib[vi] + n->qb[vq];

		/* clamp */
		CLAMP(r, 0, 255);
		CLAMP(g, 0, 255);
		CLAMP(b, 0, 255);

		/* fill in the table */
		n->texel[i] = MAKE_ARGB(0xff, r, g, b);
	}

	/* no longer dirty */
	n->dirty = FALSE;
}



/*************************************
 *
 *  Faux DAC implementation
 *
 *************************************/

static void dacdata_w(dac_state *d, UINT8 regnum, UINT8 data)
{
	d->reg[regnum] = data;
}


static void dacdata_r(dac_state *d, UINT8 regnum)
{
	UINT8 result = 0xff;

	/* switch off the DAC register requested */
	switch (regnum)
	{
		case 5:
			/* this is just to make startup happy */
			switch (d->reg[7])
			{
				case 0x01:	result = 0x55; break;
				case 0x07:	result = 0x71; break;
				case 0x0b:	result = 0x79; break;
			}
			break;

		default:
			result = d->reg[regnum];
			break;
	}

	/* remember the read result; it is fetched elsewhere */
	d->read_result = result;
}



/*************************************
 *
 *  Texuture parameter computation
 *
 *************************************/

static void recompute_texture_params(tmu_state *t)
{
	int bppscale;
	UINT32 base;
	int lod;

	/* extract LOD parameters */
	t->lodmin = TEXLOD_LODMIN(t->reg[tLOD].u) << 6;
	t->lodmax = TEXLOD_LODMAX(t->reg[tLOD].u) << 6;
	t->lodbias = (INT8)(TEXLOD_LODBIAS(t->reg[tLOD].u) << 2) << 4;

	/* determine which LODs are present */
	t->lodmask = 0x1ff;
	if (TEXLOD_LOD_TSPLIT(t->reg[tLOD].u))
	{
		if (!TEXLOD_LOD_ODD(t->reg[tLOD].u))
			t->lodmask = 0x155;
		else
			t->lodmask = 0x0aa;
	}

	/* determine base texture width/height */
	t->wmask = t->hmask = 0xff;
	if (TEXLOD_LOD_S_IS_WIDER(t->reg[tLOD].u))
		t->hmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);
	else
		t->wmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);

	/* determine the bpp of the texture */
	bppscale = TEXMODE_FORMAT(t->reg[textureMode].u) >> 3;

	/* start with the base of LOD 0 */
	base = t->reg[texBaseAddr].u * 8;
	t->lodoffset[0] = base & t->mask;

	/* LODs 1-3 are different depending on whether we are in multitex mode */
	if (TEXLOD_TMULTIBASEADDR(t->reg[tLOD].u))
	{
		base = t->reg[texBaseAddr_1].u * 8;
		t->lodoffset[1] = base & t->mask;
		base = t->reg[texBaseAddr_2].u * 8;
		t->lodoffset[2] = base & t->mask;
		base = t->reg[texBaseAddr_3_8].u * 8;
		t->lodoffset[3] = base & t->mask;
	}
	else
	{
		if (t->lodmask & (1 << 0))
			base += (((t->wmask >> 0) + 1) * ((t->hmask >> 0) + 1)) << bppscale;
		t->lodoffset[1] = base & t->mask;
		if (t->lodmask & (1 << 1))
			base += (((t->wmask >> 1) + 1) * ((t->hmask >> 1) + 1)) << bppscale;
		t->lodoffset[2] = base & t->mask;
		if (t->lodmask & (1 << 2))
			base += (((t->wmask >> 2) + 1) * ((t->hmask >> 2) + 1)) << bppscale;
		t->lodoffset[3] = base & t->mask;
	}

	/* remaining LODs make sense */
	for (lod = 4; lod <= 8; lod++)
	{
		if (t->lodmask & (1 << (lod - 1)))
		{
			UINT32 size = ((t->wmask >> (lod - 1)) + 1) * ((t->hmask >> (lod - 1)) + 1);
			if (size < 4) size = 4;
			base += size << bppscale;
		}
		t->lodoffset[lod] = base & t->mask;
	}

	/* set the NCC lookup appropriately */
	t->texel[1] = t->texel[9] = t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)].texel;

	/* pick the lookup table */
	t->lookup = t->texel[TEXMODE_FORMAT(t->reg[textureMode].u)];

	/* compute the detail parameters */
	t->detailmax = TEXDETAIL_DETAIL_MAX(t->reg[tDetail].u);
	t->detailbias = (INT8)(TEXDETAIL_DETAIL_BIAS(t->reg[tDetail].u) << 2) << 6;
	t->detailscale = TEXDETAIL_DETAIL_SCALE(t->reg[tDetail].u);

	/* no longer dirty */
	t->regdirty = FALSE;
}


INLINE void prepare_tmu(tmu_state *t)
{
	INT64 texdx, texdy;

	/* if the texture parameters are dirty, update them */
	if (t->regdirty)
		recompute_texture_params(t);

	/* ensure that the NCC tables are up to date */
	if ((TEXMODE_FORMAT(t->reg[textureMode].u) & 7) == 1)
	{
		ncc_table *n = &t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)];
		t->texel[1] = t->texel[9] = n->texel;
		if (n->dirty)
			ncc_table_update(n);
	}

	/* compute (ds^2 + dt^2) in both X and Y as 28.36 numbers */
	texdx = (INT64)(t->dsdx >> 14) * (INT64)(t->dsdx >> 14) + (INT64)(t->dtdx >> 14) * (INT64)(t->dtdx >> 14);
	texdy = (INT64)(t->dsdy >> 14) * (INT64)(t->dsdy >> 14) + (INT64)(t->dtdy >> 14) * (INT64)(t->dtdy >> 14);

	/* pick whichever is larger and shift off some high bits -> 28.20 */
	if (texdx < texdy)
		texdx = texdy;
	texdx >>= 16;

	/* use our fast reciprocal/log on this value; it expects input as a */
	/* 16.32 number, and returns the log of the reciprocal, so we have to */
	/* adjust the result: negative to get the log of the original value */
	/* plus 12 to account for the extra exponent, and divided by 2 to */
	/* get the log of the square root of texdx */
	(void)fast_reciplog(texdx, &t->lodbase);
	t->lodbase = (-t->lodbase + (12 << 8)) / 2;
}



/*************************************
 *
 *  Stall the activecpu until we are
 *  ready
 *
 *************************************/

static void stall_cpu_callback(void *param)
{
	check_stalled_cpu(param, mame_timer_get_time());
}


static void check_stalled_cpu(voodoo_state *v, mame_time current_time)
{
	int resume = FALSE;

	/* flush anything we can */
	if (v->pci.op_pending)
		flush_fifos(v, current_time);

	/* if we're just stalled until the LWM is passed, see if we're ok now */
	if (v->pci.stall_state == STALLED_UNTIL_FIFO_LWM)
	{
		/* if there's room in the memory FIFO now, we can proceed */
		if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
		{
			if (fifo_items(&v->fbi.fifo) < 2 * 32 * FBIINIT0_MEMORY_FIFO_HWM(v->reg[fbiInit0].u))
				resume = TRUE;
		}
		else if (fifo_space(&v->pci.fifo) > 2 * FBIINIT0_PCI_FIFO_LWM(v->reg[fbiInit0].u))
			resume = TRUE;
	}

	/* if we're stalled until the FIFOs are empty, check now */
	else if (v->pci.stall_state == STALLED_UNTIL_FIFO_EMPTY)
	{
		if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
		{
			if (fifo_empty(&v->fbi.fifo) && fifo_empty(&v->pci.fifo))
				resume = TRUE;
		}
		else if (fifo_empty(&v->pci.fifo))
			resume = TRUE;
	}

	/* resume if necessary */
	if (resume || !v->pci.op_pending)
	{
		if (LOG_FIFO) logerror("VOODOO.%d.FIFO:Stall condition cleared; resuming\n", v->index);
		v->pci.stall_state = NOT_STALLED;
		if (v->pci.unstall_callback)
			(*v->pci.unstall_callback)();
		cpu_trigger(v->trigger);
	}

	/* if not, set a timer for the next one */
	else
	{
		mame_timer_adjust_ptr(v->pci.continue_timer, sub_mame_times(v->pci.op_end_time, current_time), time_never);
	}
}


static void stall_cpu(voodoo_state *v, int state, mame_time current_time)
{
	v->stats.stalls++;
	cpu_spinuntil_trigger(v->trigger);
	v->pci.stall_state = state;
	if (!v->pci.op_pending) osd_die("FIFOs not empty, no op pending!");
	mame_timer_adjust_ptr(v->pci.continue_timer, sub_mame_times(v->pci.op_end_time, current_time), time_never);
}



/*************************************
 *
 *  Voodoo register writes
 *
 *************************************/

static INT32 register_w(voodoo_state *v, offs_t offset, UINT32 data)
{
	UINT32 origdata = data;
	INT32 cycles = 0;
	UINT8 regnum;
	UINT8 chips;

	/* statistics */
	v->stats.reg_writes++;

	/* determine which chips we are addressing */
	chips = (offset >> 8) & 0xf;
	if (chips == 0)
		chips = 0xf;
	chips &= v->chipmask;

	/* the first 64 registers can be aliased differently */
	if ((offset & 0x800c0) == 0x80000 && FBIINIT3_TRI_REGISTER_REMAP(v->reg[fbiInit3].u))
		regnum = register_alias_map[offset & 0x3f];
	else
		regnum = offset & 0xff;

	/* switch off the register */
	switch (regnum)
	{
		/* Vertex data is 12.4 formatted fixed point */
		case fvertexAx:
			FLOAT2FIXED(data, 4);
		case vertexAx:
			if (chips & 1) v->fbi.ax = (INT16)data;
			break;

		case fvertexAy:
			FLOAT2FIXED(data, 4);
		case vertexAy:
			if (chips & 1) v->fbi.ay = (INT16)data;
			break;

		case fvertexBx:
			FLOAT2FIXED(data, 4);
		case vertexBx:
			if (chips & 1) v->fbi.bx = (INT16)data;
			break;

		case fvertexBy:
			FLOAT2FIXED(data, 4);
		case vertexBy:
			if (chips & 1) v->fbi.by = (INT16)data;
			break;

		case fvertexCx:
			FLOAT2FIXED(data, 4);
		case vertexCx:
			if (chips & 1) v->fbi.cx = (INT16)data;
			break;

		case fvertexCy:
			FLOAT2FIXED(data, 4);
		case vertexCy:
			if (chips & 1) v->fbi.cy = (INT16)data;
			break;

		/* RGB data is 12.12 formatted fixed point */
		case fstartR:
			FLOAT2FIXED(data, 12);
		case startR:
			if (chips & 1) v->fbi.startr = (INT32)(data << 8) >> 8;
			break;

		case fstartG:
			FLOAT2FIXED(data, 12);
		case startG:
			if (chips & 1) v->fbi.startg = (INT32)(data << 8) >> 8;
			break;

		case fstartB:
			FLOAT2FIXED(data, 12);
		case startB:
			if (chips & 1) v->fbi.startb = (INT32)(data << 8) >> 8;
			break;

		case fstartA:
			FLOAT2FIXED(data, 12);
		case startA:
			if (chips & 1) v->fbi.starta = (INT32)(data << 8) >> 8;
			break;

		case fdRdX:
			FLOAT2FIXED(data, 12);
		case dRdX:
			if (chips & 1) v->fbi.drdx = (INT32)(data << 8) >> 8;
			break;

		case fdGdX:
			FLOAT2FIXED(data, 12);
		case dGdX:
			if (chips & 1) v->fbi.dgdx = (INT32)(data << 8) >> 8;
			break;

		case fdBdX:
			FLOAT2FIXED(data, 12);
		case dBdX:
			if (chips & 1) v->fbi.dbdx = (INT32)(data << 8) >> 8;
			break;

		case fdAdX:
			FLOAT2FIXED(data, 12);
		case dAdX:
			if (chips & 1) v->fbi.dadx = (INT32)(data << 8) >> 8;
			break;

		case fdRdY:
			FLOAT2FIXED(data, 12);
		case dRdY:
			if (chips & 1) v->fbi.drdy = (INT32)(data << 8) >> 8;
			break;

		case fdGdY:
			FLOAT2FIXED(data, 12);
		case dGdY:
			if (chips & 1) v->fbi.dgdy = (INT32)(data << 8) >> 8;
			break;

		case fdBdY:
			FLOAT2FIXED(data, 12);
		case dBdY:
			if (chips & 1) v->fbi.dbdy = (INT32)(data << 8) >> 8;
			break;

		case fdAdY:
			FLOAT2FIXED(data, 12);
		case dAdY:
			if (chips & 1) v->fbi.dady = (INT32)(data << 8) >> 8;
			break;

		/* Z data is 20.12 formatted fixed point */
		case fstartZ:
			FLOAT2FIXED(data, 12);
		case startZ:
			if (chips & 1) v->fbi.startz = (INT32)data;
			break;

		case fdZdX:
			FLOAT2FIXED(data, 12);
		case dZdX:
			if (chips & 1) v->fbi.dzdx = (INT32)data;
			break;

		case fdZdY:
			FLOAT2FIXED(data, 12);
		case dZdY:
			if (chips & 1) v->fbi.dzdy = (INT32)data;
			break;

		/* S,T data is 14.18 formatted fixed point, converted to 16.32 internally */
		case fstartS:
			if (chips & 2) v->tmu[0].starts = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].starts = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case startS:
			if (chips & 2) v->tmu[0].starts = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].starts = (INT64)(INT32)data << 14;
			break;

		case fstartT:
			if (chips & 2) v->tmu[0].startt = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].startt = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case startT:
			if (chips & 2) v->tmu[0].startt = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].startt = (INT64)(INT32)data << 14;
			break;

		case fdSdX:
			if (chips & 2) v->tmu[0].dsdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dsdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dSdX:
			if (chips & 2) v->tmu[0].dsdx = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dsdx = (INT64)(INT32)data << 14;
			break;

		case fdTdX:
			if (chips & 2) v->tmu[0].dtdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dtdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dTdX:
			if (chips & 2) v->tmu[0].dtdx = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dtdx = (INT64)(INT32)data << 14;
			break;

		case fdSdY:
			if (chips & 2) v->tmu[0].dsdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dsdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dSdY:
			if (chips & 2) v->tmu[0].dsdy = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dsdy = (INT64)(INT32)data << 14;
			break;

		case fdTdY:
			if (chips & 2) v->tmu[0].dtdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dtdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dTdY:
			if (chips & 2) v->tmu[0].dtdy = (INT64)(INT32)data << 14;
			if (chips & 4) v->tmu[1].dtdy = (INT64)(INT32)data << 14;
			break;

		/* W data is 2.30 formatted fixed point, converted to 16.32 internally */
		case fstartW:
			if (chips & 1) v->fbi.startw = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 2) v->tmu[0].startw = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].startw = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case startW:
			if (chips & 1) v->fbi.startw = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].startw = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].startw = (INT64)(INT32)data << 2;
			break;

		case fdWdX:
			if (chips & 1) v->fbi.dwdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 2) v->tmu[0].dwdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dwdx = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dWdX:
			if (chips & 1) v->fbi.dwdx = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].dwdx = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].dwdx = (INT64)(INT32)data << 2;
			break;

		case fdWdY:
			if (chips & 1) v->fbi.dwdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 2) v->tmu[0].dwdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			if (chips & 4) v->tmu[1].dwdy = (INT64)(u2f(data) * (float)(1 << 16) * (float)(1 << 16));
			break;
		case dWdY:
			if (chips & 1) v->fbi.dwdy = (INT64)(INT32)data << 2;
			if (chips & 2) v->tmu[0].dwdy = (INT64)(INT32)data << 2;
			if (chips & 4) v->tmu[1].dwdy = (INT64)(INT32)data << 2;
			break;

		/* triangle drawing */
		case triangleCMD:
			v->fbi.cheating_allowed = (v->fbi.ax != 0 || v->fbi.ay != 0 || v->fbi.bx > 50 || v->fbi.by != 0 || v->fbi.cx != 0 || v->fbi.cy > 50);
			v->fbi.sign = data;
			cycles = triangle(v);
			break;

		case ftriangleCMD:
			v->fbi.cheating_allowed = TRUE;
			v->fbi.sign = data;
			cycles = triangle(v);
			break;

		/* other commands */
		case nopCMD:
			reset_counters(v);
			break;

		case fastfillCMD:
			cycles = fastfill(v);
			break;

		case swapbufferCMD:
			cycles = swapbuffer(v, data);
			break;

		/* external DAC access */
		case dacData:
			if (!(data & 0x800))
				dacdata_w(&v->dac, (data >> 8) & 7, data & 0xff);
			else
				dacdata_r(&v->dac, (data >> 8) & 7);
			break;

		/* video dimensions */
		case videoDimensions:
			if (data & 0x3ff)
				v->fbi.width = data & 0x3ff;
			if (data & 0x3ff0000)
				v->fbi.height = (data >> 16) & 0x3ff;
			set_visible_area(0, v->fbi.width - 1, 0, v->fbi.height - 1);
			timer_adjust_ptr(v->fbi.vblank_timer, cpu_getscanlinetime(v->fbi.height), TIME_NEVER);
			recompute_video_memory(v);
			break;

		/* vertical sync rate */
		case vSync:
			if (v->reg[hSync].u != 0 && v->reg[vSync].u != 0)
			{
				float vtotal = (v->reg[vSync].u >> 16) + (v->reg[vSync].u & 0xffff);
				float stdfps = 15750 / vtotal, stddiff = fabs(stdfps - Machine->drv->frames_per_second);
				float medfps = 25000 / vtotal, meddiff = fabs(medfps - Machine->drv->frames_per_second);
				float vgafps = 31500 / vtotal, vgadiff = fabs(vgafps - Machine->drv->frames_per_second);

				if (stddiff < meddiff && stddiff < vgadiff)
					set_refresh_rate(stdfps);
				else if (meddiff < vgadiff)
					set_refresh_rate(medfps);
				else
					set_refresh_rate(vgafps);
			}
			break;

		/* fbiInit0 can only be written if initEnable says we can */
		case fbiInit0:
			if ((chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[fbiInit0].u = data;
				if (FBIINIT0_GRAPHICS_RESET(data))
					soft_reset(v);
				if (FBIINIT0_FIFO_RESET(data))
					fifo_reset(&v->pci.fifo);
				recompute_video_memory(v);
			}
			break;

		/* fbiInit5-7 are Voodoo 2-only; ignore them on anything else */
		case fbiInit5:
		case fbiInit6:
		case fbiInit7:
			if (v->type < VOODOO_2)
				break;
			/* else fall through... */

		/* fbiInitX can only be written if initEnable says we can */
		/* most of these affect memory layout, so always recompute that when done */
		case fbiInit1:
		case fbiInit2:
		case fbiInit3:
		case fbiInit4:
			if ((chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
			{
				v->reg[regnum].u = data;
				recompute_video_memory(v);
			}
			break;

		/* nccTable entries are processed and expanded immediately */
		case nccTable+0:
		case nccTable+1:
		case nccTable+2:
		case nccTable+3:
		case nccTable+4:
		case nccTable+5:
		case nccTable+6:
		case nccTable+7:
		case nccTable+8:
		case nccTable+9:
		case nccTable+10:
		case nccTable+11:
			if (chips & 2) ncc_table_write(&v->tmu[0].ncc[0], regnum - nccTable, data);
			if (chips & 4) ncc_table_write(&v->tmu[1].ncc[0], regnum - nccTable, data);
			break;

		case nccTable+12:
		case nccTable+13:
		case nccTable+14:
		case nccTable+15:
		case nccTable+16:
		case nccTable+17:
		case nccTable+18:
		case nccTable+19:
		case nccTable+20:
		case nccTable+21:
		case nccTable+22:
		case nccTable+23:
			if (chips & 2) ncc_table_write(&v->tmu[0].ncc[1], regnum - (nccTable+12), data);
			if (chips & 4) ncc_table_write(&v->tmu[1].ncc[1], regnum - (nccTable+12), data);
			break;

		/* fogTable entries are processed and expanded immediately */
		case fogTable+0:
		case fogTable+1:
		case fogTable+2:
		case fogTable+3:
		case fogTable+4:
		case fogTable+5:
		case fogTable+6:
		case fogTable+7:
		case fogTable+8:
		case fogTable+9:
		case fogTable+10:
		case fogTable+11:
		case fogTable+12:
		case fogTable+13:
		case fogTable+14:
		case fogTable+15:
		case fogTable+16:
		case fogTable+17:
		case fogTable+18:
		case fogTable+19:
		case fogTable+20:
		case fogTable+21:
		case fogTable+22:
		case fogTable+23:
		case fogTable+24:
		case fogTable+25:
		case fogTable+26:
		case fogTable+27:
		case fogTable+28:
		case fogTable+29:
		case fogTable+30:
		case fogTable+31:
			if (chips & 1)
			{
				int base = 2 * (regnum - fogTable);
				v->fbi.fogdelta[base + 0] = (data >> 0) & 0xff;
				v->fbi.fogblend[base + 0] = (data >> 8) & 0xff;
				v->fbi.fogdelta[base + 1] = (data >> 16) & 0xff;
				v->fbi.fogblend[base + 1] = (data >> 24) & 0xff;
			}
			break;

		/* texture modifications cause us to recompute everything */
		case textureMode:
		case tLOD:
		case tDetail:
		case texBaseAddr:
		case texBaseAddr_1:
		case texBaseAddr_2:
		case texBaseAddr_3_8:
			if (chips & 2)
			{
				v->tmu[0].reg[regnum].u = data;
				v->tmu[0].regdirty = TRUE;
			}
			if (chips & 4)
			{
				v->tmu[1].reg[regnum].u = data;
				v->tmu[1].regdirty = TRUE;
			}
			break;

		/* by default, just feed the data to the chips */
		default:
			if (chips & 1) v->reg[0x000 + regnum].u = data;
			if (chips & 2) v->reg[0x100 + regnum].u = data;
			if (chips & 4) v->reg[0x200 + regnum].u = data;
			if (chips & 8) v->reg[0x300 + regnum].u = data;
			break;
	}

	if (LOG_REGISTERS)
	{
		if (regnum < fvertexAx || regnum > fdWdY)
			logerror("VOODOO.%d.REG:%s(%d) write = %08X\n", v->index, (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, origdata);
		else
			logerror("VOODOO.%d.REG:%s(%d) write = %f\n", v->index, (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, u2f(origdata));
	}

	return cycles;
}



/*************************************
 *
 *  Voodoo LFB writes
 *
 *************************************/

static INT32 lfb_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	UINT16 *dest, *depth;
	UINT32 destmax, depthmax;
	int sr[2], sg[2], sb[2], sa[2], sw[2];
	int sixteen_bit = 0, pixel;
	int x, y, scry, mask;

	/* statistics */
	v->stats.lfb_writes++;

	/* byte swizzling */
	if (LFBMODE_BYTE_SWIZZLE_WRITES(v->reg[lfbMode].u))
	{
		data = FLIPENDIAN_INT32(data);
		mem_mask = FLIPENDIAN_INT32(mem_mask);
	}

	/* word swapping */
	if (LFBMODE_WORD_SWAP_WRITES(v->reg[lfbMode].u))
	{
		data = (data << 16) | (data >> 16);
		mem_mask = (mem_mask << 16) | (mem_mask >> 16);
	}

	/* extract default depth and alpha values */
	sw[0] = sw[1] = v->reg[zaColor].u & 0xffff;
	sa[0] = sa[1] = v->reg[zaColor].u >> 24;

	/* first extract A,R,G,B from the data */
	switch (LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u) + 16 * LFBMODE_RGBA_LANES(v->reg[lfbMode].u))
	{
		case 16*0 + 0:		/* ARGB, 16-bit RGB 5-6-5 */
		case 16*2 + 0:		/* RGBA, 16-bit RGB 5-6-5 */
			EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_565_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;
		case 16*1 + 0:		/* ABGR, 16-bit RGB 5-6-5 */
		case 16*3 + 0:		/* BGRA, 16-bit RGB 5-6-5 */
			EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_565_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;

		case 16*0 + 1:		/* ARGB, 16-bit RGB x-5-5-5 */
			EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_x555_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;
		case 16*1 + 1:		/* ABGR, 16-bit RGB x-5-5-5 */
			EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_x555_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;
		case 16*2 + 1:		/* RGBA, 16-bit RGB x-5-5-5 */
			EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
			EXTRACT_555x_TO_888(data >> 16, sr[1], sg[1], sb[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;
		case 16*3 + 1:		/* BGRA, 16-bit RGB x-5-5-5 */
			EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
			EXTRACT_555x_TO_888(data >> 16, sb[1], sg[1], sr[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT;
			break;

		case 16*0 + 2:		/* ARGB, 16-bit ARGB 1-5-5-5 */
			EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			EXTRACT_1555_TO_8888(data >> 16, sa[1], sr[1], sg[1], sb[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*1 + 2:		/* ABGR, 16-bit ARGB 1-5-5-5 */
			EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			EXTRACT_1555_TO_8888(data >> 16, sa[1], sb[1], sg[1], sr[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*2 + 2:		/* RGBA, 16-bit ARGB 1-5-5-5 */
			EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			EXTRACT_5551_TO_8888(data >> 16, sr[1], sg[1], sb[1], sa[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*3 + 2:		/* BGRA, 16-bit ARGB 1-5-5-5 */
			EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			EXTRACT_5551_TO_8888(data >> 16, sb[1], sg[1], sr[1], sa[1]);
			sixteen_bit = 1;
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;

		case 16*0 + 4:		/* ARGB, 32-bit RGB x-8-8-8 */
			EXTRACT_x888_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*1 + 4:		/* ABGR, 32-bit RGB x-8-8-8 */
			EXTRACT_x888_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*2 + 4:		/* RGBA, 32-bit RGB x-8-8-8 */
			EXTRACT_888x_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT;
			break;
		case 16*3 + 4:		/* BGRA, 32-bit RGB x-8-8-8 */
			EXTRACT_888x_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT;
			break;

		case 16*0 + 5:		/* ARGB, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*1 + 5:		/* ABGR, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*2 + 5:		/* RGBA, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;
		case 16*3 + 5:		/* BGRA, 32-bit ARGB 8-8-8-8 */
			EXTRACT_8888_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
			break;

		case 16*0 + 12:		/* ARGB, 32-bit depth+RGB 5-6-5 */
		case 16*2 + 12:		/* RGBA, 32-bit depth+RGB 5-6-5 */
			sw[0] = data >> 16;
			EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*1 + 12:		/* ABGR, 32-bit depth+RGB 5-6-5 */
		case 16*3 + 12:		/* BGRA, 32-bit depth+RGB 5-6-5 */
			sw[0] = data >> 16;
			EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;

		case 16*0 + 13:		/* ARGB, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*1 + 13:		/* ABGR, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*2 + 13:		/* RGBA, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*3 + 13:		/* BGRA, 32-bit depth+RGB x-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT;
			break;

		case 16*0 + 14:		/* ARGB, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*1 + 14:		/* ABGR, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*2 + 14:		/* RGBA, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT;
			break;
		case 16*3 + 14:		/* BGRA, 32-bit depth+ARGB 1-5-5-5 */
			sw[0] = data >> 16;
			EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
			mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT;
			break;

		case 16*0 + 15:		/* ARGB, 16-bit depth */
		case 16*1 + 15:		/* ARGB, 16-bit depth */
		case 16*2 + 15:		/* ARGB, 16-bit depth */
		case 16*3 + 15:		/* ARGB, 16-bit depth */
			sw[0] = data & 0xffff;
			sw[1] = data >> 16;
			sixteen_bit = 1;
			mask = LFB_DEPTH_PRESENT;
			break;

		default:			/* reserved */
			return 0;
	}

	/* compute X,Y */
	if (sixteen_bit)
	{
		x = (offset << 1) & 0x3fe;
		y = (offset >> 9) & 0x3ff;
	}
	else
	{
		x = (offset << 0) & 0x3ff;
		y = (offset >> 10) & 0x3ff;
	}

	/* select the target buffer */
	switch (LFBMODE_WRITE_BUFFER_SELECT(v->reg[lfbMode].u))
	{
		case 0:			/* front buffer */
			dest = v->fbi.rgb[v->fbi.frontbuf];
			destmax = v->fbi.rgbmax[v->fbi.frontbuf];
			break;

		case 1:			/* back buffer */
			dest = v->fbi.rgb[v->fbi.backbuf];
			destmax = v->fbi.rgbmax[v->fbi.backbuf];
			break;

		default:		/* reserved */
			return 0;
	}
	depth = v->fbi.aux;
	depthmax = v->fbi.auxmax;

	/* simple case: no pipeline */
	if (!LFBMODE_ENABLE_PIXEL_PIPELINE(v->reg[lfbMode].u))
	{
		UINT32 bufoffs;

		if (LOG_LFB) logerror("VOODOO.%d.LFB:write raw mode %X (%d,%d) = %08X & %08X\n", v->index, LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x, y, data, mem_mask);

		/* determine the screen Y */
		scry = y;
		if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
			scry = (FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u) - y) & 0x3ff;

		/* advance pointers to the proper row */
		bufoffs = scry * v->fbi.rowpixels + x;

		/* loop over up to two pixels */
		for (pixel = 0; pixel <= sixteen_bit; pixel++)
		{
			/* make sure we care about this pixel */
			if (!(mem_mask & 0xffff))
			{
				/* write to the RGB buffer */
				if ((mask & LFB_RGB_PRESENT) && bufoffs < destmax)
				{
					/* apply dithering and write to the screen */
					APPLY_DITHER(v->reg[fbzMode].u, x, y, sr[pixel], sg[pixel], sb[pixel]);
					dest[bufoffs] = ((sr[pixel] & 0xf8) << 8) | ((sg[pixel] & 0xfc) << 3) | ((sb[pixel] & 0xf8) >> 3);
				}

				/* make sure we have an aux buffer to write to */
				if (depth && bufoffs < depthmax)
				{
					/* write to the alpha buffer */
					if ((mask & LFB_ALPHA_PRESENT) && FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u))
						depth[bufoffs] = sa[pixel];

					/* write to the depth buffer */
					if ((mask & LFB_DEPTH_PRESENT) && !FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u))
						depth[bufoffs] = sw[pixel];
				}

				/* track pixel writes to the frame buffer regardless of mask */
				v->reg[fbiPixelsOut].u++;
			}

			/* advance our pointers */
			bufoffs++;
			x++;
			mem_mask >>= 16;
		}
	}

	/* tricky case: run the full pixel pipeline on the pixel */
	else
	{
		if (LOG_LFB) logerror("VOODOO.%d.LFB:write pipelined mode %X (%d,%d) = %08X & %08X\n", v->index, LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x, y, data, mem_mask);

		/* determine the screen Y */
		scry = y;
		if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
			scry = (FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u) - y) & 0x3ff;

		/* advance pointers to the proper row */
		dest += scry * v->fbi.rowpixels;
		if (depth)
			depth += scry * v->fbi.rowpixels;

		/* loop over up to two pixels */
		for (pixel = 0; pixel <= sixteen_bit; pixel++)
		{
			/* make sure we care about this pixel */
			if (!(mem_mask & 0xffff))
			{
				INT64 iterw = sw[pixel] << (30-16);
				INT32 iterz = sw[pixel] << 12;

				/* pixel pipeline part 1 handles depth testing and stippling */
				PIXEL_PIPELINE_BEGIN(v, x, y, scry, v->reg[fbzMode].u, iterz, iterw);

				/* use the RGBA we stashed above */
				r = sr[pixel];
				g = sg[pixel];
				b = sb[pixel];
				a = sa[pixel];

				/* apply chroma key, alpha mask, and alpha testing */
				if (FBZMODE_ENABLE_CHROMAKEY(v->reg[fbzMode].u))
					APPLY_CHROMAKEY(v, MAKE_RGB(r, g, b));
				if (FBZMODE_ENABLE_ALPHA_MASK(v->reg[fbzMode].u))
					APPLY_ALPHAMASK(v, a);
				if (ALPHAMODE_ALPHATEST(v->reg[alphaMode].u))
					APPLY_ALPHATEST(v, a, v->reg[alphaMode].u);

				/* pixel pipeline part 2 handles color combine, fog, alpha, and final output */
				PIXEL_PIPELINE_END(v, x, y, dest, depth, v->reg[fbzMode].u, v->reg[alphaMode].u, v->reg[fogMode].u, iterz, (v->reg[zaColor].u >> 24));
			}

			/* advance our pointers */
			x++;
			mem_mask >>= 16;
		}
	}

	return 0;
}



/*************************************
 *
 *  Voodoo texture RAM writes
 *
 *************************************/

static INT32 texture_w(voodoo_state *v, offs_t offset, UINT32 data)
{
	int tmunum = (offset >> 19) & 0x03;
	int lod = (offset >> 15) & 0x0f;
	int tt = (offset >> 7) & 0xff;
	int ts = (offset << 1) & 0xfe;
	UINT32 tbaseaddr;
	tmu_state *t;

	/* statistics */
	v->stats.tex_writes++;

	/* validate parameters */
	if (lod > 8)
		return 0;

	/* point to the right TMU */
	if (!(v->chipmask & (2 << tmunum)))
		return 0;
	t = &v->tmu[tmunum];

	/* update texture info if dirty */
	if (t->regdirty)
		recompute_texture_params(t);

	/* swizzle the data */
	if (TEXLOD_TDATA_SWIZZLE(t->reg[tLOD].u))
		data = FLIPENDIAN_INT32(data);
	if (TEXLOD_TDATA_SWAP(t->reg[tLOD].u))
		data = (data >> 16) | (data << 16);

	/* compute the base address */
	tbaseaddr = t->lodoffset[lod];

	/* 8-bit texture case */
	if (TEXMODE_FORMAT(t->reg[textureMode].u) < 8)
	{
		UINT8 *dest = t->ram;

		/* determine how 8-bit textures are handled the texture */
		/* old code has a bit about how this is broken in gauntleg unless we always look at TMU0 */
		if (TEXMODE_SEQ_8_DOWNLD(t->reg[textureMode].u))
			tbaseaddr += tt * ((t->wmask >> lod) + 1) + ((ts << 1) & 0xfc);
		else
			tbaseaddr += tt * ((t->wmask >> lod) + 1) + (ts & 0xfc);

		if (LOG_TEXTURE_RAM) logerror("Texture 8-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);

		/* write the four bytes in little-endian order */
		dest[BYTE4_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 1)] = (data >> 8) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 2)] = (data >> 16) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 3)] = (data >> 24) & 0xff;
	}

	/* 16-bit texture case */
	else
	{
		UINT16 *dest = (UINT16 *)t->ram;

		if (LOG_TEXTURE_RAM) logerror("Texture 16-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);

		/* compute the base address */
		tbaseaddr /= 2;
		tbaseaddr += tt * ((t->wmask >> lod) + 1) + ts;

		/* write the two words in little-endian order */
		dest[BYTE_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xffff;
		dest[BYTE_XOR_LE(tbaseaddr + 1)] = (data >> 16) & 0xffff;
	}

	return 0;
}



/*************************************
 *
 *  Flush data from the FIFOs
 *
 *************************************/

static void flush_fifos(voodoo_state *v, mame_time current_time)
{
	if (!v->pci.op_pending) osd_die("flush_fifos called with no pending operation");

	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos start -- pending=%d.%08X%08X cur=%d.%08X%08X\n", v->index,
		v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds,
		current_time.seconds, (UINT32)(current_time.subseconds >> 32), (UINT32)current_time.subseconds);

	/* loop while we still have cycles to burn */
	while (compare_mame_times(v->pci.op_end_time, current_time) <= 0)
	{
		INT32 cycles;

		/* loop over 0-cycle stuff; this constitutes the bulk of our writes */
		do
		{
			fifo_state *fifo;
			UINT32 address;
			UINT32 data;

			/* choose which FIFO to read from */
			if (!fifo_empty(&v->fbi.fifo))
				fifo = &v->fbi.fifo;
			else if (!fifo_empty(&v->pci.fifo))
				fifo = &v->pci.fifo;
			else
			{
				v->pci.op_pending = FALSE;
				if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- FIFOs empty\n", v->index);
				return;
			}

			/* extract address and data */
			address = fifo_remove(fifo);
			data = fifo_remove(fifo);

			/* target the appropriate location */
			if ((address & (0xc00000/4)) == 0)
				cycles = register_w(v, address, data);
			else if (address & (0x800000/4))
				cycles = texture_w(v, address, data);
			else
			{
				UINT32 mem_mask = 0;

				/* compute mem_mask */
				if (address & 0x80000000)
					mem_mask |= 0xffff0000;
				if (address & 0x40000000)
					mem_mask |= 0x0000ffff;
				address &= 0xffffff;

				cycles = lfb_w(v, address, data, mem_mask);
			}
		}
		while (cycles == 0);

		/* account for those cycles */
		v->pci.op_end_time = add_subseconds_to_mame_time(v->pci.op_end_time, (subseconds_t)cycles * v->subseconds_per_cycle);

		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:update -- pending=%d.%08X%08X cur=%d.%08X%08X\n", v->index,
			v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds,
			current_time.seconds, (UINT32)(current_time.subseconds >> 32), (UINT32)current_time.subseconds);
	}

	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:flush_fifos end -- pending command complete at %d.%08X%08X\n", v->index,
		v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds);
}



/*************************************
 *
 *  Handle a write to the Voodoo
 *  memory space
 *
 *************************************/

static void voodoo_w(voodoo_state *v, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	int stall = FALSE;

	/* special handling for registers */
	if ((offset & 0xc00000/4) == 0)
	{
		/* check the access behavior; note that the table works even if the */
		/* alternate mapping is used */
		UINT8 access = register_access[offset & 0xff];

		/* ignore if writes aren't allowed */
		if (!(access & REGISTER_WRITE))
			return;

		/* if this is a non-FIFO command, let it go to the FIFO, but stall until it completes */
		if (!(access & REGISTER_FIFO))
			stall = TRUE;

		/* track swap buffers */
		if ((offset & 0xff) == swapbufferCMD)
			v->fbi.swaps_pending++;
	}

	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	/* if we don't have anything pending, or if FIFOs are disabled, just execute */
	if (!v->pci.op_pending || !INITEN_ENABLE_PCI_FIFO(v->pci.init_enable))
	{
		int cycles;

		/* target the appropriate location */
		if ((offset & (0xc00000/4)) == 0)
			cycles = register_w(v, offset, data);
		else if (offset & (0x800000/4))
			cycles = texture_w(v, offset, data);
		else
			cycles = lfb_w(v, offset, data, mem_mask);

		/* if we ended up with cycles, mark the operation pending */
		if (cycles)
		{
			v->pci.op_pending = TRUE;
			v->pci.op_end_time = add_subseconds_to_mame_time(mame_timer_get_time(), (subseconds_t)cycles * v->subseconds_per_cycle);

			if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:direct write start at %d.%08X%08X end at %d.%08X%08X\n", v->index,
				mame_timer_get_time().seconds, (UINT32)(mame_timer_get_time().subseconds >> 32), (UINT32)mame_timer_get_time().subseconds,
				v->pci.op_end_time.seconds, (UINT32)(v->pci.op_end_time.subseconds >> 32), (UINT32)v->pci.op_end_time.subseconds);
		}
		return;
	}

	/* modify the offset based on the mem_mask */
	if (mem_mask)
	{
		if (mem_mask & 0xffff0000)
			offset |= 0x80000000;
		if (mem_mask & 0x0000ffff)
			offset |= 0x40000000;
	}

	/* if there's room in the PCI FIFO, add there */
	if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w adding to PCI FIFO @ %08X=%08X\n", v->index, offset, data);
	if (!fifo_full(&v->pci.fifo))
	{
		fifo_add(&v->pci.fifo, offset);
		fifo_add(&v->pci.fifo, data);
	}
	else
		osd_die("PCI FIFO full");

	/* handle flushing to the memory FIFO */
	if (FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u) &&
		fifo_space(&v->pci.fifo) <= 2 * FBIINIT4_MEMORY_FIFO_LWM(v->reg[fbiInit4].u))
	{
		UINT8 valid[4];

		/* determine which types of data can go to the memory FIFO */
		valid[0] = TRUE;
		valid[1] = FBIINIT0_LFB_TO_MEMORY_FIFO(v->reg[fbiInit0].u);
		valid[2] = valid[3] = FBIINIT0_TEXMEM_TO_MEMORY_FIFO(v->reg[fbiInit0].u);

		/* flush everything we can */
		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w moving PCI FIFO to memory FIFO\n", v->index);
		while (!fifo_empty(&v->pci.fifo) && valid[(fifo_peek(&v->pci.fifo) >> 22) & 3])
		{
			fifo_add(&v->fbi.fifo, fifo_remove(&v->pci.fifo));
			fifo_add(&v->fbi.fifo, fifo_remove(&v->pci.fifo));
		}

		/* if we're above the HWM as a result, stall */
		if (FBIINIT0_STALL_PCIE_FOR_HWM(v->reg[fbiInit0].u) &&
			fifo_items(&v->fbi.fifo) >= 2 * 32 * FBIINIT0_MEMORY_FIFO_HWM(v->reg[fbiInit0].u))
		{
			if (LOG_FIFO) logerror("VOODOO.%d.FIFO:voodoo_w hit memory FIFO HWM -- stalling\n", v->index);
			stall_cpu(v, STALLED_UNTIL_FIFO_LWM, mame_timer_get_time());
		}
	}

	/* if we're at the LWM for the PCI FIFO, stall */
	if (FBIINIT0_STALL_PCIE_FOR_HWM(v->reg[fbiInit0].u) &&
		fifo_space(&v->pci.fifo) <= 2 * FBIINIT0_PCI_FIFO_LWM(v->reg[fbiInit0].u))
	{
		if (LOG_FIFO) logerror("VOODOO.%d.FIFO:voodoo_w hit PCI FIFO free LWM -- stalling\n", v->index);
		stall_cpu(v, STALLED_UNTIL_FIFO_LWM, mame_timer_get_time());
	}

	/* if we weren't ready, and this is a non-FIFO access, stall until the FIFOs are clear */
	if (stall)
	{
		if (LOG_FIFO_VERBOSE) logerror("VOODOO.%d.FIFO:voodoo_w wrote non-FIFO register -- stalling until clear\n", v->index);
		stall_cpu(v, STALLED_UNTIL_FIFO_EMPTY, mame_timer_get_time());
	}
}



/*************************************
 *
 *  Handle a register read
 *
 *************************************/

static UINT32 register_r(voodoo_state *v, offs_t offset)
{
	int regnum = offset & 0xff;
	UINT32 result;

	/* statistics */
	v->stats.reg_reads++;

	/* first make sure this register is readable */
	if (!(register_access[regnum] & REGISTER_READ))
	{
		logerror("VOODOO.%d.ERROR:Invalid attempt to read %s\n", v->index, voodoo_reg_name[regnum]);
		return 0xffffffff;
	}

	/* default result is the FBI register value */
	result = v->reg[regnum].u;

	/* some registers are dynamic; compute them */
	switch (regnum)
	{
		case status:

			/* start with a blank slate */
			result = 0;

			/* bits 5:0 are the PCI FIFO free space */
			if (fifo_empty(&v->pci.fifo))
				result |= 0x3f << 0;
			else
			{
				int temp = fifo_space(&v->pci.fifo)/2;
				if (temp > 0x3f)
					temp = 0x3f;
				result |= temp << 0;
			}

			/* bit 6 is the vertical retrace */
			result |= v->fbi.vblank << 6;

			/* bit 7 is FBI graphics engine busy */
			if (!fifo_empty(&v->pci.fifo))
				result |= 1 << 7;

			/* bit 8 is TREX busy */
			if (!fifo_empty(&v->pci.fifo))
				result |= 1 << 8;

			/* bit 9 is overall busy */
			if (!fifo_empty(&v->pci.fifo))
				result |= 1 << 9;

			/* bits 11:10 specifies which buffer is visible */
			result |= 1 << v->fbi.frontbuf;

			/* bits 27:12 indicate memory FIFO freespace */
			if (!FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u) || fifo_empty(&v->fbi.fifo))
				result |= 0xffff << 12;
			else
			{
				int temp = fifo_space(&v->fbi.fifo)/2;
				if (temp > 0xffff)
					temp = 0xffff;
				result |= temp << 12;
			}

			/* bits 30:28 are the number of pending swaps */
			if (v->fbi.swaps_pending > 7)
				result |= 7 << 28;
			else
				result |= v->fbi.swaps_pending << 28;

			/* bit 31 is not used */

			/* eat some cycles since people like polling here */
			activecpu_eat_cycles(1000);
			break;

		case fbiInit2:

			/* bit 2 of the initEnable register maps this to dacRead */
			if (INITEN_REMAP_INIT_TO_DAC(v->pci.init_enable))
				result = v->dac.read_result;
			break;

		case vRetrace:

			/* return the current scanline for now */
			result = cpu_getscanline();
			break;

		/* reserved area in the TMU read by the Vegas startup sequence */
		case hvRetrace:
			result = 0x200 << 16;	/* should be between 0x7b and 0x267 */
			result |= 0x80;			/* should be between 0x17 and 0x103 */
			break;

		case fbiTrianglesOut:
		case fbiPixelsIn:
		case fbiChromaFail:
		case fbiZfuncFail:
		case fbiAfuncFail:
		case fbiPixelsOut:

			/* all counters are 24-bit only */
			result &= 0xffffff;
			break;
	}

	if (LOG_REGISTERS)
	{
		int logit = TRUE;

		/* don't log multiple identical status reads from the same address */
		if (regnum == status)
		{
			offs_t pc = activecpu_get_pc();
			if (pc == v->last_status_pc && result == v->last_status_value)
				logit = FALSE;
			v->last_status_pc = pc;
			v->last_status_value = result;
		}

		if (logit)
			logerror("VOODOO.%d.REG:%s read = %08X\n", v->index, voodoo_reg_name[regnum], result);
	}

	return result;
}



/*************************************
 *
 *  Handle an LFB read
 *
 *************************************/

static UINT32 lfb_r(voodoo_state *v, offs_t offset)
{
	UINT16 *buffer;
	UINT32 bufmax;
	UINT32 bufoffs;
	UINT32 data;
	int x, y, scry;

	/* statistics */
	v->stats.lfb_reads++;

	/* compute X,Y */
	x = (offset << 1) & 0x3fe;
	y = (offset >> 9) & 0x3ff;

	/* select the target buffer */
	switch (LFBMODE_READ_BUFFER_SELECT(v->reg[lfbMode].u))
	{
		case 0:			/* front buffer */
			buffer = v->fbi.rgb[v->fbi.frontbuf];
			bufmax = v->fbi.rgbmax[v->fbi.frontbuf];
			break;

		case 1:			/* back buffer */
			buffer = v->fbi.rgb[v->fbi.backbuf];
			bufmax = v->fbi.rgbmax[v->fbi.backbuf];
			break;

		case 2:			/* aux buffer */
			buffer = v->fbi.aux;
			bufmax = v->fbi.auxmax;
			if (!buffer)
				return 0xffffffff;
			break;

		default:		/* reserved */
			return 0xffffffff;
	}

	/* determine the screen Y */
	scry = y;
	if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
		scry = (FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u) - y) & 0x3ff;

	/* advance pointers to the proper row */
	bufoffs = scry * v->fbi.rowpixels + x;
	if (bufoffs >= bufmax)
		return 0xffffffff;

	/* compute the data */
	data = buffer[bufoffs + 0] | (buffer[bufoffs + 1] << 16);

	/* word swapping */
	if (LFBMODE_WORD_SWAP_READS(v->reg[lfbMode].u))
		data = (data << 16) | (data >> 16);

	/* byte swizzling */
	if (LFBMODE_BYTE_SWIZZLE_READS(v->reg[lfbMode].u))
		data = FLIPENDIAN_INT32(data);

	if (LOG_LFB) logerror("VOODOO.%d.LFB:read (%d,%d) = %08X\n", v->index, x, y, data);
	return data;
}



/*************************************
 *
 *  Handle a read from the Voodoo
 *  memory space
 *
 *************************************/

static UINT32 voodoo_r(voodoo_state *v, offs_t offset)
{
	/* if we have something pending, flush the FIFOs up to the current time */
	if (v->pci.op_pending)
		flush_fifos(v, mame_timer_get_time());

	/* target the appropriate location */
	if (!(offset & (0xc00000/4)))
		return register_r(v, offset);
	else if (!(offset & (0x800000/4)))
		return lfb_r(v, offset);

	return 0xffffffff;
}



/*************************************
 *
 *  Command handlers
 *
 *************************************/

static INT32 fastfill(voodoo_state *v)
{
	int sx = (v->reg[clipLeftRight].u >> 16) & 0x3ff;
	int ex = (v->reg[clipLeftRight].u >> 0) & 0x3ff;
	int sy = (v->reg[clipLowYHighY].u >> 16) & 0x3ff;
	int ey = (v->reg[clipLowYHighY].u >> 0) & 0x3ff;
	int x, y;

	/* if we're not clearing either, take no time */
	if (!FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u) &&
		!FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u))
		return 0;

	/* are we clearing the RGB buffer? */
	if (FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u))
	{
		UINT16 dither[16];
		UINT16 *drawbuf;

		/* determine the draw buffer */
		switch (FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u))
		{
			case 0:		/* front buffer */
				drawbuf = v->fbi.rgb[v->fbi.frontbuf];
				break;

			case 1:		/* back buffer */
				drawbuf = v->fbi.rgb[v->fbi.backbuf];
				break;

			default:	/* reserved */
				return 0;
		}

		/* determine the dither pattern */
		for (y = 0; y < 4; y++)
			for (x = 0; x < 4; x++)
			{
				int r = RGB_RED(v->reg[color1].u);
				int g = RGB_GREEN(v->reg[color1].u);
				int b = RGB_BLUE(v->reg[color1].u);

				APPLY_DITHER(v->reg[fbzMode].u, x, y, r, g, b);
				dither[y*4 + x] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
			}

		/* now do the fill */
		for (y = sy; y < ey; y++)
		{
			UINT16 *ditherow = &dither[(y & 3) * 4];
			UINT16 *dest;
			int scry;

			/* determine the screen Y */
			scry = y;
			if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
				scry = (FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u) - y) & 0x3ff;

			/* fill this row */
			dest = drawbuf + scry * v->fbi.rowpixels;
			for (x = sx; x < ex; x++)
				dest[x] = ditherow[x & 3];
		}

		/* track pixel writes to the frame buffer regardless of mask */
		v->reg[fbiPixelsOut].u += (ey - sy) * (ex - sx);
	}

	/* are we clearing the depth buffer? */
	if (FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u) && v->fbi.aux)
	{
		UINT16 color = v->reg[zaColor].u;

		/* now do the fill */
		for (y = sy; y < ey; y++)
		{
			UINT16 *dest;
			int scry;

			/* determine the screen Y */
			scry = y;
			if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
				scry = (FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u) - y) & 0x3ff;

			/* fill this row */
			dest = v->fbi.aux + scry * v->fbi.rowpixels;
			for (x = sx; x < ex; x++)
				dest[x] = color;
		}
	}

	/* 2 pixels per clock */
	return ((ey - sy) * (ex - sx)) / 2;
}


static INT32 swapbuffer(voodoo_state *v, UINT32 data)
{
	/* set the don't swap value for Voodoo 2 */
	v->fbi.vblank_swap_pending = TRUE;
	v->fbi.vblank_swap = (data >> 1) & 0xff;
	v->fbi.vblank_dont_swap = (data >> 9) & 1;

	/* if we're not syncing to the retrace, process the command immediately */
	if (!(data & 1))
	{
		swap_buffers(v);
		return 0;
	}

	/* determine how many cycles to wait; we deliberately overshoot here because */
	/* the final count gets updated on the VBLANK */
	return v->fbi.vblank_swap * v->freq / 30;
}


static INT32 triangle(voodoo_state *v)
{
	INT32 start_pixels_in = v->reg[fbiPixelsIn].u;
	raster_info *info;
	int texcount = 0;
	UINT16 *drawbuf;

	/* perform subpixel adjustments */
	if (FBZCP_CCA_SUBPIXEL_ADJUST(v->reg[fbzColorPath].u))
	{
		INT32 dx = 8 - (v->fbi.ax & 15);
		INT32 dy = 8 - (v->fbi.ay & 15);

		v->fbi.startr += (dy * v->fbi.drdy + dx * v->fbi.drdx) >> 4;
		v->fbi.startg += (dy * v->fbi.dgdy + dx * v->fbi.dgdx) >> 4;
		v->fbi.startb += (dy * v->fbi.dbdy + dx * v->fbi.dbdx) >> 4;
		v->fbi.starta += (dy * v->fbi.dady + dx * v->fbi.dadx) >> 4;
		v->fbi.startw += (dy * v->fbi.dwdy + dx * v->fbi.dwdx) >> 4;
		v->fbi.startz += fixed_mul_shift(dy, v->fbi.dzdy, 4) + fixed_mul_shift(dx, v->fbi.dzdx, 4);

		v->tmu[0].startw += (dy * v->tmu[0].dwdy + dx * v->tmu[0].dwdx) >> 4;
		v->tmu[0].starts += (dy * v->tmu[0].dsdy + dx * v->tmu[0].dsdx) >> 4;
		v->tmu[0].startt += (dy * v->tmu[0].dtdy + dx * v->tmu[0].dtdx) >> 4;

		if (v->chipmask & 0x04)
		{
			v->tmu[1].startw += (dy * v->tmu[1].dwdy + dx * v->tmu[1].dwdx) >> 4;
			v->tmu[1].starts += (dy * v->tmu[1].dsdy + dx * v->tmu[1].dsdx) >> 4;
			v->tmu[1].startt += (dy * v->tmu[1].dtdy + dx * v->tmu[1].dtdx) >> 4;
		}
	}

	/* determine the draw buffer */
	switch (FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u))
	{
		case 0:		/* front buffer */
			drawbuf = v->fbi.rgb[v->fbi.frontbuf];
			break;

		case 1:		/* back buffer */
			drawbuf = v->fbi.rgb[v->fbi.backbuf];
			break;

		default:	/* reserved */
			return TRIANGLE_SETUP_CLOCKS;
	}

	/* set up the textures */
	texcount = 0;
	if (!FBIINIT3_DISABLE_TMUS(v->reg[fbiInit3].u) && FBZCP_TEXTURE_ENABLE(v->reg[fbzColorPath].u))
	{
		texcount = 1;
		prepare_tmu(&v->tmu[0]);
		v->stats.texture_mode[TEXMODE_FORMAT(v->tmu[0].reg[textureMode].u)]++;

		/* see if we need to deal with a second TMU */
		if (v->chipmask & 0x04)
		{
			texcount = 2;
			prepare_tmu(&v->tmu[1]);
			v->stats.texture_mode[TEXMODE_FORMAT(v->tmu[1].reg[textureMode].u)]++;
		}
	}

	/* find a rasterizer that matches our current state */
	info = find_rasterizer(v, texcount);
	(*info->callback)(v, drawbuf);
	info->polys++;
	info->hits += v->reg[fbiPixelsIn].u - start_pixels_in;

	if (LOG_REGISTERS) logerror("cycles = %d\n", TRIANGLE_SETUP_CLOCKS + v->reg[fbiPixelsIn].u - start_pixels_in);

	/* 1 pixel per clock, plus some setup time */
	return TRIANGLE_SETUP_CLOCKS + v->reg[fbiPixelsIn].u - start_pixels_in;
}



/*************************************
 *
 *  Rasterizer management
 *
 *************************************/

static raster_info *add_rasterizer(voodoo_state *v, const raster_info *cinfo)
{
	raster_info *info = auto_malloc(sizeof(*info));
	int hash = compute_raster_hash(cinfo);

	/* make a copy of the info */
	*info = *cinfo;

	/* fill in the data */
	info->hits = 0;
	info->polys = 0;

	/* hook us into the hash table */
	info->next = v->raster_hash[hash];
	v->raster_hash[hash] = info;

	if (LOG_RASTERIZERS)
		printf("Adding rasterizer @ %08X : %08X %08X %08X %08X %08X %08X (hash=%d)\n",
				(UINT32)info->callback,
				info->eff_color_path, info->eff_alpha_mode, info->eff_fog_mode, info->eff_fbz_mode,
				info->eff_tex_mode_0, info->eff_tex_mode_1, hash);

	return info;
}


static raster_info *find_rasterizer(voodoo_state *v, int texcount)
{
	raster_info *info, *prev = NULL;
	raster_info curinfo;
	int hash;

	/* build an info struct with all the parameters */
	curinfo.eff_color_path = normalize_color_path(v->reg[fbzColorPath].u);
	curinfo.eff_alpha_mode = normalize_alpha_mode(v->reg[alphaMode].u);
	curinfo.eff_fog_mode = normalize_fog_mode(v->reg[fogMode].u);
	curinfo.eff_fbz_mode = normalize_fbz_mode(v->reg[fbzMode].u);
	curinfo.eff_tex_mode_0 = (texcount >= 1) ? normalize_tex_mode(v->tmu[0].reg[textureMode].u) : 0xffffffff;
	curinfo.eff_tex_mode_1 = (texcount >= 2) ? normalize_tex_mode(v->tmu[1].reg[textureMode].u) : 0xffffffff;

	/* compute the hash */
	hash = compute_raster_hash(&curinfo);

	/* find the appropriate hash entry */
	for (info = v->raster_hash[hash]; info; prev = info, info = info->next)
		if (info->eff_color_path == curinfo.eff_color_path &&
			info->eff_alpha_mode == curinfo.eff_alpha_mode &&
			info->eff_fog_mode == curinfo.eff_fog_mode &&
			info->eff_fbz_mode == curinfo.eff_fbz_mode &&
			info->eff_tex_mode_0 == curinfo.eff_tex_mode_0 &&
			info->eff_tex_mode_1 == curinfo.eff_tex_mode_1)
		{
			/* got it, move us to the head of the list */
			if (prev)
			{
				prev->next = info->next;
				info->next = v->raster_hash[hash];
				v->raster_hash[hash] = info;
			}

			/* return the result */
			return info;
		}

	/* attempt to generate one */
#ifdef VOODOO_DRC
	curinfo.callback = generate_rasterizer(v);
	curinfo.is_generic = FALSE;
	if (curinfo.callback == NULL)
#endif
	{
		curinfo.callback = (texcount == 0) ? raster_generic_0tmu : (texcount == 1) ? raster_generic_1tmu : raster_generic_2tmu;
		curinfo.is_generic = TRUE;
	}

	return add_rasterizer(v, &curinfo);
}


static void dump_rasterizer_stats(voodoo_state *v)
{
	raster_info *cur, *best;
	int hash;

	printf("----\n");

	/* loop until we've displayed everything */
	while (1)
	{
		best = NULL;

		/* find the highest entry */
		for (hash = 0; hash < RASTER_HASH_SIZE; hash++)
			for (cur = v->raster_hash[hash]; cur; cur = cur->next)
				if (!best || cur->hits > best->hits)
					best = cur;

		/* if we're done, we're done */
		if (!best || best->hits == 0)
			break;

		/* print it */
		printf("%9d  %10d %c  ( 0x%08X,  0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X )\n",
			best->polys,
			best->hits,
			best->is_generic ? '*' : ' ',
			best->eff_color_path,
			best->eff_alpha_mode,
			best->eff_fog_mode,
			best->eff_fbz_mode,
			best->eff_tex_mode_0,
			best->eff_tex_mode_1);

		/* reset */
		best->hits = best->polys = 0;
	}
}



/*************************************
 *
 *  Generic rasterizers
 *
 *************************************/

RASTERIZER(generic_0tmu, 0, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, 0, 0)

RASTERIZER(generic_1tmu, 1, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, v->tmu[0].reg[textureMode].u, 0)

RASTERIZER(generic_2tmu, 2, v->reg[fbzColorPath].u, v->reg[fbzMode].u, v->reg[alphaMode].u,
			v->reg[fogMode].u, v->tmu[0].reg[textureMode].u, v->tmu[1].reg[textureMode].u)



#else

/*************************************
 *
 *  Specific rasterizers
 *
 *************************************/

/* blitz -------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B073B, 0x0C261ACF, 0xFFFFFFFF )	/* title */
RASTERIZER_ENTRY( 0x00582C35,  0x00515110, 0x00000000, 0x000B0739, 0x0C261ACF, 0xFFFFFFFF )	/* video */
RASTERIZER_ENTRY( 0x00000035,  0x00000000, 0x00000000, 0x000B0739, 0x0C261A0F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00002C35,  0x00515110, 0x00000000, 0x000B07F9, 0x0C261A0F, 0xFFFFFFFF )	/* select screen */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B07F9, 0x0C261ACF, 0xFFFFFFFF )	/* select screens */

/* blitz2k -----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x01420039,  0x00000000, 0x00000000, 0x000B0739, 0x0C261ACF, 0xFFFFFFFF )	/* select screens */

/* carnevil ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00002435,  0x04045119, 0x00000000, 0x00030279, 0x0C261A0F, 0xFFFFFFFF )	/* clown */
RASTERIZER_ENTRY( 0x00002425,  0x00045119, 0x00000000, 0x00030679, 0x0C261A0F, 0xFFFFFFFF )	/* clown */

/* calspeed ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00002815,  0x05045119, 0x00000001, 0x000A0723, 0x0C261ACF, 0xFFFFFFFF )	/* movie */
RASTERIZER_ENTRY( 0x01022C19,  0x05000009, 0x00000001, 0x000B0739, 0x0C26100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00002C15,  0x05045119, 0x00000001, 0x000B0739, 0x0C26180F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x01022C19,  0x05000009, 0x00000001, 0x000B073B, 0x0C26100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00480015,  0x05045119, 0x00000001, 0x000B0339, 0x0C26100F, 0xFFFFFFFF )	/* in-game */

/* mace --------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x082418DF, 0xFFFFFFFF )	/* character screen */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x0824100F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x0824180F, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x082410CF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00600C09,  0x00045119, 0x00000000, 0x000B0779, 0x082418CF, 0xFFFFFFFF )	/* in-game */

/* wg3dh -------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x082410DF, 0xFFFFFFFF )	/* title */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x0824101F, 0xFFFFFFFF )	/* credits */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x08241ADF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00480035,  0x00045119, 0x00000000, 0x000B0779, 0x082418DF, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00000035,  0x00045119, 0x00000000, 0x000B0779, 0x0824189F, 0xFFFFFFFF )	/* in-game */

/* gradius4 ----> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x02420002,  0x00000009, 0x00000000, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x01420021,  0x00005119, 0x00000000, 0x00030F7B, 0x14261AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x00000005,  0x00005119, 0x00000000, 0x00030F7B, 0x14261A87, 0xFFFFFFFF )	/* in-game */

/* nbapbp ------> fbzColorPath alphaMode   fogMode,    fbzMode,    texMode0,   texMode1  */
RASTERIZER_ENTRY( 0x00424219,  0x00000000, 0x00000001, 0x00030B7B, 0x08241AC7, 0xFFFFFFFF )	/* intro */
RASTERIZER_ENTRY( 0x00002809,  0x00004110, 0x00000001, 0x00030FFB, 0x08241AC7, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x00424219,  0x00000000, 0x00000001, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* in-game */
RASTERIZER_ENTRY( 0x0200421A,  0x00001510, 0x00000001, 0x00030F7B, 0x08241AC7, 0xFFFFFFFF )	/* in-game */

#endif
