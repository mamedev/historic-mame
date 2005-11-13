/*************************************************************************

    3dfx Voodoo Graphics SST-1/2 emulator

    emulator by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "voodoo.h"
#include "cpu/mips/mips3.h"
#include <math.h>


/*************************************
 *
 *  Math trickery
 *
 *************************************/

#ifndef _WIN32
#define SETUP_FPU()
#define RESTORE_FPU()
#define TRUNC_TO_INT(f) (float) (floor(f))
#else
#include <float.h>
#define SETUP_FPU() { int oldfpu = _controlfp(_RC_CHOP | _PC_24, _MCW_RC | _MCW_PC)
#define RESTORE_FPU() _controlfp(oldfpu, _MCW_RC | _MCW_PC); }
#define TRUNC_TO_INT(f) ((int)(f))
#endif



/*************************************
 *
 *  Optimization flags
 *
 *************************************/

#define BILINEAR_FILTER			(1)
#define PER_PIXEL_LOD			(1)
#define ALPHA_BLENDING			(1)
#define FOGGING					(1)
#define OPTIMIZATIONS_ENABLED	(1)



/*************************************
 *
 *  Debugging flags
 *
 *************************************/

/* debugging */
#define DISPLAY_DEPTHBUF		(0)
#define LOG_UPDATE_SWAP			(0)
#define LOG_REGISTERS			(0)
#define LOG_COMMANDS			(0)
#define LOG_CMDFIFO				(0)
#define LOG_CMDFIFO_VERBOSE		(0)
#define LOG_MEMFIFO				(0)
#define LOG_TEXTURERAM			(0)
#define LOG_FRAMEBUFFER			(0)
#define LOG_RASTERIZERS			(0)
#define DEBUG_LOD				(0)



/*************************************
 *
 *  Constants
 *
 *************************************/

#define MAX_TMUS				(2)
#define RASTER_HASH_SIZE		(97)

#define FRAMEBUF_WIDTH			(1024)
#define FRAMEBUF_HEIGHT			(1024)

#define MEMFIFO_TRIGGER			(13579)

#define CMDFIFO_SIZE			(2*1024*1024)

#define FBZCOLORPATH_MASK		(0x0fffffff)
#define ALPHAMODE_MASK			(0xffffffff)
#define FOGMODE_MASK			(0x0000001f)
#define FBZMODE_MASK			(0x003fffff)
#define TEXTUREMODE0_MASK		(0xfffff8df)
#define TEXTUREMODE1_MASK		(0x00000000)



/*************************************
 *
 *  Macros
 *
 *************************************/

#define ADD_TO_PIXEL_COUNT(a)	do { if ((a) > 0) pixelcount += (a); } while (0)

/* note that these equations and the dither matrixes have
   been confirmed to be exact matches to the real hardware */
#define DITHER_RB(val,dith)	((((val) << 1) - ((val) >> 4) + ((val) >> 7) + (dith)) >> 1)
#define DITHER_G(val,dith)	((((val) << 2) - ((val) >> 4) + ((val) >> 6) + (dith)) >> 2)



/*************************************
 *
 *  Structures & typedefs
 *
 *************************************/

/* temporary holding for triangle setup */
struct setup_vert
{
	float x,y;
	float a,r,g,b;
	float z,wb;
	float w0,s0,t0;
	float w1,s1,t1;
};

/* information about a rasterizer */
struct rasterizer_info
{
	struct rasterizer_info *next;
	void	(*callback)(void);
	UINT8	is_generic;
	UINT32	hits;
	UINT32	polys;
	UINT32	val_fbzColorPath;
	UINT32	val_alphaMode;
	UINT32	val_fogMode;
	UINT32	val_fbzMode;
	UINT32	val_textureMode0;
	UINT32	val_tlod0;
	UINT32	val_textureMode1;
	UINT32	val_tlod1;
};

/* one FIFO entry */
struct fifo_entry
{
	write32_handler writefunc;
	offs_t offset;
	UINT32 data;
	UINT32 mem_mask;
};

/* a single triangle vertex */
struct tri_vertex
{
	float x, y;
};



/*************************************
 *
 *  Local variables
 *
 *************************************/

/* core constants */
static UINT8 tmus;
static UINT8 voodoo_type;
static offs_t texram_mask;
static void (*client_vblank_callback)(int);
static UINT8 vblank_state;
static UINT8 display_statistics;

/* VRAM and various buffers */
static UINT16 *framebuf[2];
static UINT16 *depthbuf;
static UINT16 *frontbuf;
static UINT16 *backbuf;
static UINT16 **buffer_access[4];
static UINT8 *textureram[MAX_TMUS];
static UINT32 *cmdfifo;
static UINT32 cmdfifo_expected;

static UINT32 *pen_lookup;
static INT16 *lod_lookup;

/* register pointers */
static UINT32 voodoo_regs[0x400];
static float *fvoodoo_regs = (float *)voodoo_regs;

/* color DAC fake registers */
static UINT8 dac_regs[8];
static UINT8 dac_read_result;

/* clut table */
static UINT32 clut_table[33];

static UINT32 init_enable;

/* texel tables */
static UINT32 *texel_lookup[MAX_TMUS][16];
static UINT8 texel_lookup_dirty[MAX_TMUS][16];
static INT32 ncc_y[MAX_TMUS][2][16];
static INT32 ncc_ir[MAX_TMUS][2][4], ncc_ig[MAX_TMUS][2][4], ncc_ib[MAX_TMUS][2][4];
static INT32 ncc_qr[MAX_TMUS][2][4], ncc_qg[MAX_TMUS][2][4], ncc_qb[MAX_TMUS][2][4];

/* fog tables */
static UINT8 fog_blend[64];
static UINT8 fog_delta[64];

/* VBLANK and swapping */
static mame_timer *vblank_timer;
static int vblank_count;
static UINT8 blocked_on_swap;
static UINT8 swap_vblanks;
static UINT8 swap_dont_swap;
static UINT8 num_pending_swaps;

/* FIFOs */
static struct fifo_entry *memory_fifo;
static UINT32 memory_fifo_count;
static UINT32 memory_fifo_size;
static UINT8 memory_fifo_lfb;
static UINT8 memory_fifo_texture;
static UINT8 memory_fifo_in_process;

/* fbzMode variables */
static rectangle *fbz_cliprect;
static rectangle fbz_noclip;
static rectangle fbz_clip;
static UINT8 fbz_dithering;
static UINT8 fbz_rgb_write;
static UINT8 fbz_depth_write;
static const UINT8 *fbz_dither_matrix;
static UINT16 **fbz_draw_buffer;
static UINT8 fbz_invert_y;

/* lfbMode variables */
static UINT8 lfb_write_format;
static UINT16 **lfb_write_buffer;
static UINT16 **lfb_read_buffer;
static UINT8 lfb_flipy;

/* videoDimensions variables */
static int video_width;
static int video_height;

/* fbiInit variables */
static UINT8 triple_buffer;
static UINT32 inverted_yorigin;

/* textureMode variables */
static UINT8 trex_dirty[MAX_TMUS];
static UINT8 trex_format[MAX_TMUS];
static INT32 trex_lodmin[MAX_TMUS];
static INT32 trex_lodmax[MAX_TMUS];
static INT32 trex_lodbias[MAX_TMUS];
static UINT32 trex_width[MAX_TMUS];
static UINT32 trex_height[MAX_TMUS];
static const UINT8 *trex_lod_width_shift[MAX_TMUS];
static UINT8 *trex_lod_start[MAX_TMUS][9];

/* triangle parameters */
static struct tri_vertex tri_va, tri_vb, tri_vc;
static INT32 tri_startr, tri_drdx, tri_drdy;	/* .16 */
static INT32 tri_startg, tri_dgdx, tri_dgdy;	/* .16 */
static INT32 tri_startb, tri_dbdx, tri_dbdy;	/* .16 */
static INT32 tri_starta, tri_dadx, tri_dady;	/* .16 */
static INT32 tri_startz, tri_dzdx, tri_dzdy;	/* .12 */
static float tri_startw, tri_dwdx, tri_dwdy;
static float tri_starts0, tri_ds0dx, tri_ds0dy;
static float tri_startt0, tri_dt0dx, tri_dt0dy;
static float tri_startw0, tri_dw0dx, tri_dw0dy;
static float tri_starts1, tri_ds1dx, tri_ds1dy;
static float tri_startt1, tri_dt1dx, tri_dt1dy;
static float tri_startw1, tri_dw1dx, tri_dw1dy;

/* Voodoo2 triangle setup */
static int setup_count;
static struct setup_vert setup_verts[3];
static struct setup_vert setup_pending;

/* rasterizers */
static struct rasterizer_info *raster_hash[RASTER_HASH_SIZE];

/* optimization flags */
static int skip_count;
static int skip_interval;
static int resolution_mask;

/* debugging/stats */
static UINT32 polycount, wpolycount, pixelcount, lastfps, framecount, totalframes;
static char stats_buffer[10*30];
static UINT16 modes_used;
static offs_t status_lastpc;
static int status_lastpc_count;
static UINT8 loglod = 0;
static int total_swaps;
static UINT8 cheating_allowed;
static int reg_writes;
static int reg_reads;
static int tex_writes;
static int fb_writes;
static int fb_reads;
static int generic_polys;



/*************************************
 *
 *  Dither matrices
 *
 *************************************/

/* these two dither matrices have been confirmed on real hardware */
static const UINT8 dither_matrix_4x4[16] =
{
	 0,  8,  2, 10,
	12,  4, 14,  6,
	 3, 11,  1,  9,
	15,  7, 13,  5
};

static const UINT8 dither_matrix_2x2[16] =
{
	 2, 10,  2, 10,
	14,  6, 14,  6,
	 2, 10,  2, 10,
	14,  6, 14,  6
};

static const INT32 lod_dither_matrix[16] =
{
	 0<<4,  8<<4,  2<<4, 10<<4,
	12<<4,  4<<4, 14<<4,  6<<4,
	 3<<4, 11<<4,  1<<4,  9<<4,
	15<<4,  7<<4, 13<<4,  5<<4
};



/*************************************
 *
 *  LOD tables
 *
 *************************************/

static const UINT32 lod_offset_table[4][16] =
{
	{ 0x00000, 0x10000, 0x14000, 0x15000, 0x15400, 0x15500, 0x15540, 0x15550, 0x15554 },
	{ 0x00000, 0x08000, 0x0a000, 0x0a800, 0x0aa00, 0x0aa80, 0x0aaa0, 0x0aaa8, 0x0aaa8 },
	{ 0x00000, 0x04000, 0x05000, 0x05400, 0x05500, 0x05540, 0x05550, 0x05550, 0x05550 },
	{ 0x00000, 0x02000, 0x02800, 0x02a00, 0x02a80, 0x02aa0, 0x02aa0, 0x02aa0, 0x02aa0 }
};

static const UINT8 lod_width_shift[8][16] =
{
	{ 8,7,6,5,4,3,2,1,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 7,6,5,4,3,2,1,0,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 6,5,4,3,2,1,0,0,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 5,4,3,2,1,0,0,0,0 },
	{ 8,7,6,5,4,3,2,1,0 }
};



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void reset_buffers(void);
static void update_memory_fifo(void);
static void swap_buffers(void);
static void vblank_callback(int scanline);
static void cmdfifo_process_pending(UINT32 old_depth);

/* from vooddraw.h */
static void fastfill(void);
static void draw_triangle(void);
static void setup_and_draw_triangle(void);
static int init_generator(void);



/*************************************
 *
 *  Register constants
 *
 *************************************/

/* 0x000 */
#define status			(0x000/4)
#define intrCtrl		(0x004/4)	/* Voodoo2 only */
#define vertexAx		(0x008/4)
#define vertexAy		(0x00c/4)
#define vertexBx		(0x010/4)
#define vertexBy		(0x014/4)
#define vertexCx		(0x018/4)
#define vertexCy		(0x01c/4)
#define startR			(0x020/4)
#define startG			(0x024/4)
#define startB			(0x028/4)
#define startZ			(0x02c/4)
#define startA			(0x030/4)
#define startS			(0x034/4)
#define startT			(0x038/4)
#define startW			(0x03c/4)

/* 0x040 */
#define dRdX			(0x040/4)
#define dGdX			(0x044/4)
#define dBdX			(0x048/4)
#define dZdX			(0x04c/4)
#define dAdX			(0x050/4)
#define dSdX			(0x054/4)
#define dTdX			(0x058/4)
#define dWdX			(0x05c/4)
#define dRdY			(0x060/4)
#define dGdY			(0x064/4)
#define dBdY			(0x068/4)
#define dZdY			(0x06c/4)
#define dAdY			(0x070/4)
#define dSdY			(0x074/4)
#define dTdY			(0x078/4)
#define dWdY			(0x07c/4)

/* 0x080 */
#define triangleCMD		(0x080/4)
#define fvertexAx		(0x088/4)
#define fvertexAy		(0x08c/4)
#define fvertexBx		(0x090/4)
#define fvertexBy		(0x094/4)
#define fvertexCx		(0x098/4)
#define fvertexCy		(0x09c/4)
#define fstartR			(0x0a0/4)
#define fstartG			(0x0a4/4)
#define fstartB			(0x0a8/4)
#define fstartZ			(0x0ac/4)
#define fstartA			(0x0b0/4)
#define fstartS			(0x0b4/4)
#define fstartT			(0x0b8/4)
#define fstartW			(0x0bc/4)

/* 0x0c0 */
#define fdRdX			(0x0c0/4)
#define fdGdX			(0x0c4/4)
#define fdBdX			(0x0c8/4)
#define fdZdX			(0x0cc/4)
#define fdAdX			(0x0d0/4)
#define fdSdX			(0x0d4/4)
#define fdTdX			(0x0d8/4)
#define fdWdX			(0x0dc/4)
#define fdRdY			(0x0e0/4)
#define fdGdY			(0x0e4/4)
#define fdBdY			(0x0e8/4)
#define fdZdY			(0x0ec/4)
#define fdAdY			(0x0f0/4)
#define fdSdY			(0x0f4/4)
#define fdTdY			(0x0f8/4)
#define fdWdY			(0x0fc/4)

/* 0x100 */
#define ftriangleCMD	(0x100/4)
#define fbzColorPath	(0x104/4)
#define fogMode			(0x108/4)
#define alphaMode		(0x10c/4)
#define fbzMode			(0x110/4)
#define lfbMode			(0x114/4)
#define clipLeftRight	(0x118/4)
#define clipLowYHighY	(0x11c/4)
#define nopCMD			(0x120/4)
#define fastfillCMD		(0x124/4)
#define swapbufferCMD	(0x128/4)
#define fogColor		(0x12c/4)
#define zaColor			(0x130/4)
#define chromaKey		(0x134/4)
#define chromaRange		(0x138/4)	/* Voodoo2 only */
#define userIntrCMD		(0x13c/4)	/* Voodoo2 only */

/* 0x140 */
#define stipple			(0x140/4)
#define color0			(0x144/4)
#define color1			(0x148/4)
#define fbiPixelsIn		(0x14c/4)
#define fbiChromaFail	(0x150/4)
#define fbiZfuncFail	(0x154/4)
#define fbiAfuncFail	(0x158/4)
#define fbiPixelsOut	(0x15c/4)
#define fogTable		(0x160/4)

/* 0x1c0 */
#define cmdFifoBaseAddr	(0x1e0/4)	/* Voodoo2 only */
#define cmdFifoBump		(0x1e4/4)	/* Voodoo2 only */
#define cmdFifoRdPtr	(0x1e8/4)	/* Voodoo2 only */
#define cmdFifoAMin		(0x1ec/4)	/* Voodoo2 only */
#define cmdFifoAMax		(0x1f0/4)	/* Voodoo2 only */
#define cmdFifoDepth	(0x1f4/4)	/* Voodoo2 only */
#define cmdFifoHoles	(0x1f8/4)	/* Voodoo2 only */

/* 0x200 */
#define fbiInit4		(0x200/4)
#define vRetrace		(0x204/4)
#define backPorch		(0x208/4)
#define videoDimensions	(0x20c/4)
#define fbiInit0		(0x210/4)
#define fbiInit1		(0x214/4)
#define fbiInit2		(0x218/4)
#define fbiInit3		(0x21c/4)
#define hSync			(0x220/4)
#define vSync			(0x224/4)
#define clutData		(0x228/4)
#define dacData			(0x22c/4)
#define maxRgbDelta		(0x230/4)
#define hBorder			(0x234/4)	/* Voodoo2 only */
#define vBorder			(0x238/4)	/* Voodoo2 only */
#define borderColor		(0x23c/4)	/* Voodoo2 only */

/* 0x240 */
#define hvRetrace		(0x240/4)	/* Voodoo2 only */
#define fbiInit5		(0x244/4)	/* Voodoo2 only */
#define fbiInit6		(0x248/4)	/* Voodoo2 only */
#define fbiInit7		(0x24c/4)	/* Voodoo2 only */
#define fbiSwapHistory	(0x258/4)	/* Voodoo2 only */
#define fbiTrianglesOut	(0x25c/4)	/* Voodoo2 only */
#define sSetupMode		(0x260/4)	/* Voodoo2 only */
#define sVx				(0x264/4)	/* Voodoo2 only */
#define sVy				(0x268/4)	/* Voodoo2 only */
#define sARGB			(0x26c/4)	/* Voodoo2 only */
#define sRed			(0x270/4)	/* Voodoo2 only */
#define sGreen			(0x274/4)	/* Voodoo2 only */
#define sBlue			(0x278/4)	/* Voodoo2 only */
#define sAlpha			(0x27c/4)	/* Voodoo2 only */

/* 0x280 */
#define sVz				(0x280/4)	/* Voodoo2 only */
#define sWb				(0x284/4)	/* Voodoo2 only */
#define sWtmu0			(0x288/4)	/* Voodoo2 only */
#define sS_W0			(0x28c/4)	/* Voodoo2 only */
#define sT_W0			(0x290/4)	/* Voodoo2 only */
#define sWtmu1			(0x294/4)	/* Voodoo2 only */
#define sS_Wtmu1		(0x298/4)	/* Voodoo2 only */
#define sT_Wtmu1		(0x29c/4)	/* Voodoo2 only */
#define sDrawTriCMD		(0x2a0/4)	/* Voodoo2 only */
#define sBeginTriCMD	(0x2a4/4)	/* Voodoo2 only */

/* 0x2c0 */
#define bltSrcBaseAddr	(0x2c0/4)	/* Voodoo2 only */
#define bltDstBaseAddr	(0x2c4/4)	/* Voodoo2 only */
#define bltXYStrides	(0x2c8/4)	/* Voodoo2 only */
#define bltSrcChromaRange (0x2cc/4)	/* Voodoo2 only */
#define bltDstChromaRange (0x2d0/4)	/* Voodoo2 only */
#define bltClipX		(0x2d4/4)	/* Voodoo2 only */
#define bltClipY		(0x2d8/4)	/* Voodoo2 only */
#define bltSrcXY		(0x2e0/4)	/* Voodoo2 only */
#define bltDstXY		(0x2e4/4)	/* Voodoo2 only */
#define bltSize			(0x2e8/4)	/* Voodoo2 only */
#define bltRop			(0x2ec/4)	/* Voodoo2 only */
#define bltColor		(0x2f0/4)	/* Voodoo2 only */
#define bltCommand		(0x2f8/4)	/* Voodoo2 only */
#define BltData			(0x2fc/4)	/* Voodoo2 only */

/* 0x300 */
#define textureMode		(0x300/4)
#define tLOD			(0x304/4)
#define tDetail			(0x308/4)
#define texBaseAddr		(0x30c/4)
#define texBaseAddr_1	(0x310/4)
#define texBaseAddr_2	(0x314/4)
#define texBaseAddr_3_8	(0x318/4)
#define trexInit0		(0x31c/4)
#define trexInit1		(0x320/4)
#define nccTable		(0x324/4)



/*************************************
 *
 *  Register string table for debug
 *
 *************************************/

static const char *voodoo_reg_name[] =
{
	/* 0x000 */
	"status",		"{intrCtrl}",	"vertexAx",		"vertexAy",
	"vertexBx",		"vertexBy",		"vertexCx",		"vertexCy",
	"startR",		"startG",		"startB",		"startZ",
	"startA",		"startS",		"startT",		"startW",
	/* 0x040 */
	"dRdX",			"dGdX",			"dBdX",			"dZdX",
	"dAdX",			"dSdX",			"dTdX",			"dWdX",
	"dRdY",			"dGdY",			"dBdY",			"dZdY",
	"dAdY",			"dSdY",			"dTdY",			"dWdY",
	/* 0x080 */
	"triangleCMD",	"reserved084",	"fvertexAx",	"fvertexAy",
	"fvertexBx",	"fvertexBy",	"fvertexCx",	"fvertexCy",
	"fstartR",		"fstartG",		"fstartB",		"fstartZ",
	"fstartA",		"fstartS",		"fstartT",		"fstartW",
	/* 0x0c0 */
	"fdRdX",		"fdGdX",		"fdBdX",		"fdZdX",
	"fdAdX",		"fdSdX",		"fdTdX",		"fdWdX",
	"fdRdY",		"fdGdY",		"fdBdY",		"fdZdY",
	"fdAdY",		"fdSdY",		"fdTdY",		"fdWdY",
	/* 0x100 */
	"ftriangleCMD",	"fbzColorPath",	"fogMode",		"alphaMode",
	"fbzMode",		"lfbMode",		"clipLeftRight","clipLowYHighY",
	"nopCMD",		"fastfillCMD",	"swapbufferCMD","fogColor",
	"zaColor",		"chromaKey",	"{chromaRange}","{userIntrCMD}",
	/* 0x140 */
	"stipple",		"color0",		"color1",		"fbiPixelsIn",
	"fbiChromaFail","fbiZfuncFail",	"fbiAfuncFail",	"fbiPixelsOut",
	"fogTable160",	"fogTable164",	"fogTable168",	"fogTable16c",
	"fogTable170",	"fogTable174",	"fogTable178",	"fogTable17c",
	/* 0x180 */
	"fogTable180",	"fogTable184",	"fogTable188",	"fogTable18c",
	"fogTable190",	"fogTable194",	"fogTable198",	"fogTable19c",
	"fogTable1a0",	"fogTable1a4",	"fogTable1a8",	"fogTable1ac",
	"fogTable1b0",	"fogTable1b4",	"fogTable1b8",	"fogTable1bc",
	/* 0x1c0 */
	"fogTable1c0",	"fogTable1c4",	"fogTable1c8",	"fogTable1cc",
	"fogTable1d0",	"fogTable1d4",	"fogTable1d8",	"fogTable1dc",
	"{cmdFifoBaseAddr}","{cmdFifoBump}","{cmdFifoRdPtr}","{cmdFifoAMin}",
	"{cmdFifoAMax}","{cmdFifoDepth}","{cmdFifoHoles}","reserved1fc",
	/* 0x200 */
	"fbiInit4",		"vRetrace",		"backPorch",	"videoDimensions",
	"fbiInit0",		"fbiInit1",		"fbiInit2",		"fbiInit3",
	"hSync",		"vSync",		"clutData",		"dacData",
	"maxRgbDelta",	"{hBorder}",	"{vBorder}",	"{borderColor}",
	/* 0x240 */
	"{hvRetrace}",	"{fbiInit5}",	"{fbiInit6}",	"{fbiInit7}",
	"reserved250",	"reserved254",	"{fbiSwapHistory}","{fbiTrianglesOut}",
	"{sSetupMode}",	"{sVx}",		"{sVy}",		"{sARGB}",
	"{sRed}",		"{sGreen}",		"{sBlue}",		"{sAlpha}",
	/* 0x280 */
	"{sVz}",		"{sWb}",		"{sWtmu0}",		"{sS/Wtmu0}",
	"{sT/Wtmu0}",	"{sWtmu1}",		"{sS/Wtmu1}",	"{sT/Wtmu1}",
	"{sDrawTriCMD}","{sBeginTriCMD}","reserved2a8",	"reserved2ac",
	"reserved2b0",	"reserved2b4",	"reserved2b8",	"reserved2bc",
	/* 0x2c0 */
	"{bltSrcBaseAddr}","{bltDstBaseAddr}","{bltXYStrides}","{bltSrcChromaRange}",
	"{bltDstChromaRange}","{bltClipX}","{bltClipY}","reserved2dc",
	"{bltSrcXY}",	"{bltDstXY}",	"{bltSize}",	"{bltRop}",
	"{bltColor}",	"reserved2f4",	"{bltCommand}",	"{bltData}",
	/* 0x300 */
	"textureMode",	"tLOD",			"tDetail",		"texBaseAddr",
	"texBaseAddr_1","texBaseAddr_2","texBaseAddr_3_8","trexInit0",
	"trexInit1",	"nccTable0.0",	"nccTable0.1",	"nccTable0.2",
	"nccTable0.3",	"nccTable0.4",	"nccTable0.5",	"nccTable0.6",
	/* 0x340 */
	"nccTable0.7",	"nccTable0.8",	"nccTable0.9",	"nccTable0.A",
	"nccTable0.B",	"nccTable1.0",	"nccTable1.1",	"nccTable1.2",
	"nccTable1.3",	"nccTable1.4",	"nccTable1.5",	"nccTable1.6",
	"nccTable1.7",	"nccTable1.8",	"nccTable1.9",	"nccTable1.A",
	/* 0x380 */
	"nccTable1.B"
};



#ifdef __MWERKS__
#pragma mark VIDEO START
#endif

/*************************************
 *
 *  Video start
 *
 *************************************/

int voodoo_start_common(void)
{
	int i, j;

	fvoodoo_regs = (float *)voodoo_regs;

	/* allocate memory for the pen, dither, and depth lookups */
	pen_lookup = auto_malloc(sizeof(pen_lookup[0]) * 65536);
	lod_lookup = auto_malloc(sizeof(lod_lookup[0]) * 65536);
	if (!pen_lookup || !lod_lookup)
		return 1;

	/* allocate memory for the frame a depth buffers */
	framebuf[0] = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	framebuf[1] = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	depthbuf = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	if (!framebuf[0] || !framebuf[1] || !depthbuf)
		return 1;

	/* allocate memory for the memory fifo */
	memory_fifo = auto_malloc(65536 * sizeof(memory_fifo[0]));
	if (!memory_fifo)
		return 1;

	/* allocate memory for the cmdfifo */
	if (voodoo_type >= 2)
	{
		cmdfifo = auto_malloc(CMDFIFO_SIZE);
		if (!cmdfifo)
			return 1;
	}

	/* allocate memory for texture RAM */
	for (i = 0; i < tmus; i++)
	{
		textureram[i] = auto_malloc(texram_mask + 1 + 65536);
		if (!textureram[i])
			return 1;
	}

	/* allocate memory for the lookup tables */
	for (j = 0; j < tmus; j++)
		for (i = 0; i < 16; i++)
		{
			texel_lookup[j][i] = auto_malloc(sizeof(UINT32) * ((i < 8) ? 256 : 65536));
			if (!texel_lookup[j][i])
				return 1;
		}

	/* initialize LOD tables */
	for (i = 0; i < 65536; i++)
	{
		UINT32 bits = i << 16;
		float fval = u2f(bits);
		float flod;

		if (fval <= 0)
			flod = 0;
		else
		{
			flod = log(fval) / log(2.0);
			if (flod <= -100.0)
				flod = -100.0;
			if (flod >= 100.0)
				flod = 100.0;
		}
		lod_lookup[i] = (int)(256.0 * flod);
	}

	/* init the palette */
	for (i = 0; i < 65536; i++)
	{
		int r = (i >> 11) & 0x1f;
		int g = (i >> 5) & 0x3f;
		int b = (i) & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		pen_lookup[i] = (r << 16) | (g << 8) | b;
	}

	/* allocate a vblank timer */
	vblank_timer = timer_alloc(vblank_callback);

	/* reset optimizations */
	skip_count = 0;
	skip_interval = 1;
	resolution_mask = 0;

	voodoo_reset();
	init_generator();
	return 0;
}


static void reset_buffers(void)
{
	/* VRAM and various buffers */
	frontbuf = framebuf[0];
	backbuf = framebuf[1];
	buffer_access[0] = &frontbuf;
	buffer_access[1] = &backbuf;
	buffer_access[2] = &depthbuf;

	fbz_draw_buffer = &frontbuf;
	lfb_write_buffer = &frontbuf;
	lfb_read_buffer = &frontbuf;
}


void voodoo_reset(void)
{
	reset_buffers();

	/* color DAC fake registers */
	memset(dac_regs, 0, sizeof(dac_regs));
	init_enable = 0;

	/* initialize lookup tables */
	memset(texel_lookup_dirty, 1, sizeof(texel_lookup_dirty));
	memset(ncc_y, 0, sizeof(ncc_y));
	memset(ncc_ir, 0, sizeof(ncc_ir));
	memset(ncc_ig, 0, sizeof(ncc_ig));
	memset(ncc_ib, 0, sizeof(ncc_ib));
	memset(ncc_qr, 0, sizeof(ncc_qr));
	memset(ncc_qg, 0, sizeof(ncc_qg));
	memset(ncc_qb, 0, sizeof(ncc_qb));

	/* fog tables */
	memset(fog_blend, 0, sizeof(fog_blend));
	memset(fog_delta, 0, sizeof(fog_delta));

	/* VBLANK and swapping */
	vblank_count = 0;
	num_pending_swaps = 0;
	blocked_on_swap = 0;

	/* fbzMode variables */
	fbz_cliprect = &fbz_noclip;
	fbz_noclip.min_x = fbz_noclip.min_y = 0;
	fbz_noclip.max_x = fbz_noclip.max_y = (1024 << 4) - 1;
	fbz_clip = fbz_noclip;
	fbz_dithering = 0;
	fbz_rgb_write = 0;
	fbz_depth_write = 0;
	fbz_dither_matrix = dither_matrix_4x4;
	fbz_draw_buffer = &frontbuf;
	fbz_invert_y = 0;

	/* lfbMode variables */
	lfb_write_format = 0;
	lfb_write_buffer = &frontbuf;
	lfb_read_buffer = &frontbuf;
	lfb_flipy = 0;

	/* videoDimensions variables */
	video_width = Machine->visible_area.max_x + 1;
	video_height = Machine->visible_area.max_y + 1;

	/* fbiInit variables */
	triple_buffer = 0;
	inverted_yorigin = 0;

	/* textureMode variables */

	/* triangle setup */
	setup_count = 0;

	update_memory_fifo();
}


int voodoo_get_type(void)
{
	return voodoo_type;
}


VIDEO_START( voodoo_1x4mb )
{
	tmus = 1;
	voodoo_type = 1;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo_2x4mb )
{
	tmus = 2;
	voodoo_type = 1;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo2_1x4mb )
{
	tmus = 1;
	voodoo_type = 2;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo2_2x4mb )
{
	tmus = 2;
	voodoo_type = 2;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo3_1x4mb )
{
	tmus = 1;
	voodoo_type = 3;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo3_2x4mb )
{
	tmus = 2;
	voodoo_type = 3;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_STOP( voodoo )
{
	if (0)
	{
		printf("Stall PCI for HWM: %d\n", (voodoo_regs[fbiInit0] >> 4) & 1);
		printf("PCI FIFO Empty Entries LWM: %X\n", (voodoo_regs[fbiInit0] >> 6) & 0x1f);
		printf("LFB -> FIFO: %d\n", (voodoo_regs[fbiInit0] >> 11) & 1);
		printf("Texture -> FIFO: %d\n", (voodoo_regs[fbiInit0] >> 12) & 1);
		printf("Memory FIFO: %d\n", (voodoo_regs[fbiInit0] >> 13) & 1);
		printf("Memory FIFO HWM: %X\n", ((voodoo_regs[fbiInit0] >> 14) & 0x7ff) << 5);
		printf("Memory FIFO Write Burst HWM: %X\n", (voodoo_regs[fbiInit0] >> 25) & 0x3f);
		printf("Memory FIFO LWM for PCI: %X\n", (voodoo_regs[fbiInit4] >> 4) & 0x3f);
		printf("Memory FIFO row start: %X\n", (voodoo_regs[fbiInit4] >> 8) & 0x3ff);
		printf("Memory FIFO row rollover: %X\n", (voodoo_regs[fbiInit4] >> 18) & 0x3ff);
		printf("Video dither subtract: %X\n", (voodoo_regs[fbiInit2] >> 0) & 1);
		printf("DRAM banking: %X\n", (voodoo_regs[fbiInit2] >> 1) & 1);
		printf("Triple buffer: %X\n", (voodoo_regs[fbiInit2] >> 4) & 1);
		printf("Video buffer offset: %X\n", (voodoo_regs[fbiInit2] >> 11) & 0x1ff);
		printf("DRAM banking: %X\n", (voodoo_regs[fbiInit2] >> 20) & 1);
	}
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( voodoo )
{
	int x, y;

	if (LOG_UPDATE_SWAP)
		logerror("--- video update (%d-%d) ---\n", cliprect->min_y, cliprect->max_y);

	/* draw the screen */
#if (OPTIMIZATIONS_ENABLED)
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 *dest = (UINT32 *)bitmap->line[y];
		UINT16 *source = &frontbuf[1024 * (y & ~resolution_mask)];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = pen_lookup[source[x & ~resolution_mask]];
	}
#else
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 *dest = (UINT32 *)bitmap->line[y];
		UINT16 *source = &frontbuf[1024 * y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			*dest++ = pen_lookup[*source++];
	}
#endif

	/* alternately, display the depth buffer in red (debug) */
	if (DISPLAY_DEPTHBUF && code_pressed(KEYCODE_D))
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT16 *source = &depthbuf[1024 * y];
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				*dest++ = pen_lookup[~*source++ & 0xf800];
		}
	}

	/* update options */
	if (OPTIMIZATIONS_ENABLED && code_pressed(KEYCODE_LCONTROL))
	{
		if (code_pressed(KEYCODE_1))
		{
			while (code_pressed(KEYCODE_1)) ;
			ui_popup("Running at full frame rate");
			skip_interval = 1;
		}
		if (code_pressed(KEYCODE_2))
		{
			while (code_pressed(KEYCODE_2)) ;
			ui_popup("Running at 1/2 frame rate");
			skip_interval = 2;
		}
		if (code_pressed(KEYCODE_3))
		{
			while (code_pressed(KEYCODE_3)) ;
			ui_popup("Running at 1/3 frame rate");
			skip_interval = 3;
		}
		if (code_pressed(KEYCODE_4))
		{
			while (code_pressed(KEYCODE_4)) ;
			ui_popup("Running at 1/4 frame rate");
			skip_interval = 4;
		}
		if (code_pressed(KEYCODE_5))
		{
			while (code_pressed(KEYCODE_5)) ;
			ui_popup("Running at 1/5 frame rate");
			skip_interval = 5;
		}
		if (code_pressed(KEYCODE_6))
		{
			while (code_pressed(KEYCODE_6)) ;
			ui_popup("Running at 1/6 frame rate");
			skip_interval = 6;
		}
		if (code_pressed(KEYCODE_F))
		{
			while (code_pressed(KEYCODE_F)) ;
			ui_popup("Full resolution");
			resolution_mask = 0;
		}
		if (code_pressed(KEYCODE_H))
		{
			while (code_pressed(KEYCODE_H)) ;
			ui_popup("Half resolution");
			resolution_mask = 1;
		}
		if (code_pressed(KEYCODE_Q))
		{
			while (code_pressed(KEYCODE_Q)) ;
			ui_popup("Quarter resolution");
			resolution_mask = 3;
		}
	}

	/* update statistics (debug) */
	if (code_pressed(KEYCODE_BACKSLASH))
	{
		while (code_pressed(KEYCODE_BACKSLASH)) ;
		display_statistics = !display_statistics;
	}
	if (display_statistics)
	{
		ui_draw_text(stats_buffer, 0, 0);

		totalframes++;
		if (totalframes == (int)Machine->drv->frames_per_second)
		{
			lastfps = framecount;
			framecount = totalframes = 0;
		}
	}

	/* note of if the LOD logging key is pressed (debug) */
	if (DEBUG_LOD)
	{
		loglod = 0;
		if (code_pressed(KEYCODE_L)) loglod = 1;
	}

#if (LOG_RASTERIZERS)
	if (cpu_getcurrentframe() % 300 == 0)
	{
		printf("----\nFrame = %d, fps = %f\n", cpu_getcurrentframe(), mame_get_performance_info()->frames_per_second);
		while (TRUE)
		{
			struct rasterizer_info *info, *top = NULL;
			int tophits = 0, hash;

			for (hash = 0; hash < RASTER_HASH_SIZE; hash++)
				for (info = raster_hash[hash]; info; info = info->next)
					if (info->hits > tophits)
					{
						top = info;
						tophits = info->hits;
					}

			if (!top)
				break;

			printf("%c%10d %5d: %08X %08X %08X %08X %08X %08X (%d)\n",
				top->is_generic ? '*' : ' ',
				top->hits,
				top->polys,
				top->val_fbzColorPath,
				top->val_alphaMode,
				top->val_fogMode,
				top->val_fbzMode,
				top->val_textureMode0,
				top->val_textureMode1, hash);
			top->hits = top->polys = 0;
		}
	}
#endif
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark MEMORY FIFOS
#endif

/*************************************
 *
 *  Memory FIFO management
 *
 *************************************/

INLINE void add_to_memory_fifo(write32_handler handler, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	struct fifo_entry *entry;

	/* fill in the entry */
 	entry = &memory_fifo[memory_fifo_count++];
 	entry->writefunc = handler;
	entry->offset = offset;
	entry->data = data;
	entry->mem_mask = mem_mask;

	/* if we just ran out of room, we must block the CPU until we empty */
	if (memory_fifo_count >= memory_fifo_size)
	{
		if (LOG_MEMFIFO)
			logerror("Blocking CPU due to running out of memory FIFO entries!\n");
		cpu_spinuntil_trigger(MEMFIFO_TRIGGER);
	}

	/* debugging */
	if (LOG_MEMFIFO && memory_fifo_count % 1000 == 0)
		logerror("Memory FIFO = %d entries\n", memory_fifo_count);
}


static void update_memory_fifo(void)
{
	/* memory FIFO enabled? */
	if ((voodoo_regs[fbiInit0] >> 13) & 1)
	{
		/* yes: get parameters */
		memory_fifo_size = 0xffff;
	}
	else
	{
		/* no: use standard PCI parameters */
		memory_fifo_size = 64;
	}

	/* extract other parameters */
	memory_fifo_lfb = (voodoo_regs[fbiInit0] >> 11) & 1;
	memory_fifo_texture = (voodoo_regs[fbiInit0] >> 12) & 1;
}


static void empty_memory_fifo(void)
{
	int fifo_index;

	/* set a flag so we don't recurse */
	if (memory_fifo_in_process)
		return;
	memory_fifo_in_process = 1;

	if (LOG_MEMFIFO)
		logerror("empty_memory_fifo: %d entries\n", memory_fifo_count);

	/* loop while we're not blocked */
	for (fifo_index = 0; !blocked_on_swap && fifo_index < memory_fifo_count; fifo_index++)
	{
		struct fifo_entry *entry = &memory_fifo[fifo_index];
		(*entry->writefunc)(entry->offset, entry->data, entry->mem_mask);
	}

	/* chomp all the entries */
	if (fifo_index == 0 && memory_fifo_count > 0)
	{
		if (LOG_MEMFIFO)
			logerror("empty_memory_fifo called while still blocked on swap!\n");
	}
	else if (fifo_index != memory_fifo_count)
	{
		if (LOG_MEMFIFO)
			logerror("empty_memory_fifo: consumed %d entries, %d remain\n", fifo_index, memory_fifo_count - fifo_index);
		memmove(&memory_fifo[0], &memory_fifo[fifo_index], (memory_fifo_count - fifo_index) * sizeof(memory_fifo[0]));
	}
	else
	{
		if (LOG_MEMFIFO)
			logerror("empty_memory_fifo: consumed %d entries\n", memory_fifo_count);
	}

	/* update the count */
	memory_fifo_count -= fifo_index;

	/* if we're below the threshhold, get the CPU going again */
	if (memory_fifo_count < memory_fifo_size)
		cpu_trigger(MEMFIFO_TRIGGER);

	memory_fifo_in_process = 0;
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark VBLANK HANDLING
#endif

/*************************************
 *
 *  VBLANK callback/buffer swapping
 *
 *************************************/

static void swap_buffers(void)
{
	UINT16 *temp;

	if (LOG_UPDATE_SWAP)
		logerror("---- swapbuffers\n");

	/* force a partial update */
	force_partial_update(cpu_getscanline());

	/* swap -- but only if we're actually swapping */
	if (!swap_dont_swap)
	{
		if (skip_count == 0)
		{
			temp = frontbuf;
			frontbuf = backbuf;
			backbuf = temp;
		}

		/* update skip info */
		if (++skip_count >= skip_interval)
			skip_count = 0;
	}

	/* no longer blocked */
	blocked_on_swap = 0;
	if (num_pending_swaps > 0)
		num_pending_swaps--;

	/* empty the FIFO */
	empty_memory_fifo();

	/* update the statistics (debug) */
	if (display_statistics)
	{
		int screen_area = (Machine->visible_area.max_x - Machine->visible_area.min_x + 1) * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
		char *statsptr = stats_buffer;
		int i;

		total_swaps++;

		statsptr += sprintf(statsptr, "Poly:%5d\n", polycount);
		statsptr += sprintf(statsptr, "WPol:%5d\n", wpolycount);
		statsptr += sprintf(statsptr, "GPol:%5d\n", generic_polys);
		statsptr += sprintf(statsptr, "Rend:%5d%%\n", pixelcount * 100 / screen_area);
		statsptr += sprintf(statsptr, " FPS:%5d\n", lastfps);
		statsptr += sprintf(statsptr, "Swap:%5d\n", total_swaps);
		statsptr += sprintf(statsptr, "Pend:%5d\n", num_pending_swaps);
		statsptr += sprintf(statsptr, "FIFO:%5d\n", memory_fifo_count);
		statsptr += sprintf(statsptr, "RegW:%5d\n", reg_writes);
		statsptr += sprintf(statsptr, "RegR:%5d\n", reg_reads);
		statsptr += sprintf(statsptr, "LFBW:%5d\n", fb_writes);
		statsptr += sprintf(statsptr, "LFBR:%5d\n", fb_reads);
		statsptr += sprintf(statsptr, "TexW:%5d\n", tex_writes);
		statsptr += sprintf(statsptr, "TexM:");
		for (i = 0; i < 16; i++)
			*statsptr++ = (modes_used & (1 << i)) ? "0123456789ABCDEF"[i] : ' ';
		*statsptr = 0;

		polycount = wpolycount = generic_polys = pixelcount = 0;
		reg_reads = reg_writes = fb_reads = fb_writes = tex_writes = 0;
		modes_used = 0;
		framecount++;
	}
}


static void vblank_off_callback(int param)
{
	vblank_state = 0;

	/* call the client */
	if (client_vblank_callback)
		(*client_vblank_callback)(0);

	/* go to the end of the next frame */
	timer_adjust(vblank_timer, cpu_getscanlinetime(Machine->visible_area.max_y + 1), 0, TIME_NEVER);
}


static void vblank_callback(int scanline)
{
	vblank_state = 1;
	vblank_count++;

	if (LOG_UPDATE_SWAP)
	{
		if (num_pending_swaps > 0)
			logerror("---- vblank (blocked=%d, pend=%d, vnum=%d, vbc=%d)\n", blocked_on_swap, num_pending_swaps, vblank_count, swap_vblanks);
		else
			logerror("---- vblank (blocked=%d, pend=%d)\n", blocked_on_swap, num_pending_swaps);
	}

	/* any pending swapbuffers */
	if (blocked_on_swap && vblank_count > swap_vblanks)
	{
		vblank_count = 0;
		swap_buffers();
	}

	/* set a timer for the next off state */
	timer_set(cpu_getscanlinetime(0), 0, vblank_off_callback);

	/* call the client */
	if (client_vblank_callback)
		(*client_vblank_callback)(1);
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark MISC EXTERNAL ACCESS
#endif

/*************************************
 *
 *  Special PCI I/O
 *
 *************************************/

void voodoo_set_init_enable(UINT32 newval)
{
	init_enable = newval;
}


void voodoo_set_vblank_callback(void (*vblank)(int))
{
	client_vblank_callback = vblank;
}


UINT32 voodoo_fifo_words_left(void)
{
	/* if we're blocked, return a real number */
	if (blocked_on_swap)
		return memory_fifo_size - memory_fifo_count;

	/* otherwise, return infinity */
	return 0x7fffffff;
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark TEXTURE RAM ACCESS
#endif

/*************************************
 *
 *  Recompute dirty texture data
 *
 *************************************/

static void recompute_texture_params(int tmu)
{
	UINT8 *base = textureram[tmu];
	const UINT32 *lod_offset;
	UINT32 data, offset;
	UINT8 shift;

	/* update mode data */
	data = voodoo_regs[0x100 + 0x100*tmu + textureMode];
	trex_format[tmu] = (data >> 8) & 0x0f;
	if ((trex_format[0] & 7) == 1 && (data & 0x20))
		trex_format[0] += 6;
	modes_used |= 1 << trex_format[0];
	shift = (data >> 11) & 1;

	/* update LOD data */
	data = voodoo_regs[0x100 + 0x100*tmu + tLOD];
	trex_lodmin[tmu] = ((data >> 0) & 0x3f) << 6;
	trex_lodmax[tmu] = ((data >> 6) & 0x3f) << 6;
	if (trex_lodmax[tmu] > (8 << 8)) trex_lodmax[tmu] = 8 << 8;
	trex_lodbias[tmu] = ((INT16)(data >> 2) >> 10) << 6;
	trex_width[tmu] = (data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));
	trex_height[tmu] = !(data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));
	trex_lod_width_shift[tmu] = &lod_width_shift[(data >> 20) & 7][0];
	lod_offset = &lod_offset_table[(data >> 21) & 3][0];

	/* update LOD starts */
	offset = voodoo_regs[0x100 + 0x100*tmu + texBaseAddr] * 8;
	trex_lod_start[tmu][0] = base + (offset & texram_mask);

 if (data & 0x00080000)
 	printf("TMU%d: split texture\n", tmu);

	/* single base address */
	if (!(data & 0x01000000))
	{
		trex_lod_start[tmu][1] = base + ((offset + (lod_offset[1] << shift)) & texram_mask);
		trex_lod_start[tmu][2] = base + ((offset + (lod_offset[2] << shift)) & texram_mask);
		trex_lod_start[tmu][3] = base + ((offset + (lod_offset[3] << shift)) & texram_mask);
		trex_lod_start[tmu][4] = base + ((offset + (lod_offset[4] << shift)) & texram_mask);
		trex_lod_start[tmu][5] = base + ((offset + (lod_offset[5] << shift)) & texram_mask);
		trex_lod_start[tmu][6] = base + ((offset + (lod_offset[6] << shift)) & texram_mask);
		trex_lod_start[tmu][7] = base + ((offset + (lod_offset[7] << shift)) & texram_mask);
		trex_lod_start[tmu][8] = base + ((offset + (lod_offset[8] << shift)) & texram_mask);
	}

	/* multi base address */
	else
	{
		offset = voodoo_regs[0x100 + 0x100*tmu + texBaseAddr_1] * 8;
		trex_lod_start[tmu][1] = base + (offset & texram_mask);

		offset = voodoo_regs[0x100 + 0x100*tmu + texBaseAddr_2] * 8;
		trex_lod_start[tmu][2] = base + (offset & texram_mask);

		offset = voodoo_regs[0x100 + 0x100*tmu + texBaseAddr_3_8] * 8;
		trex_lod_start[tmu][3] = base + (offset & texram_mask);
		trex_lod_start[tmu][4] = base + ((offset + ((lod_offset[4] - lod_offset[3]) << shift)) & texram_mask);
		trex_lod_start[tmu][5] = base + ((offset + ((lod_offset[5] - lod_offset[3]) << shift)) & texram_mask);
		trex_lod_start[tmu][6] = base + ((offset + ((lod_offset[6] - lod_offset[3]) << shift)) & texram_mask);
		trex_lod_start[tmu][7] = base + ((offset + ((lod_offset[7] - lod_offset[3]) << shift)) & texram_mask);
		trex_lod_start[tmu][8] = base + ((offset + ((lod_offset[8] - lod_offset[3]) << shift)) & texram_mask);
	}
}



/*************************************
 *
 *  Texture RAM writes (no read access)
 *
 *************************************/

WRITE32_HANDLER( voodoo_textureram_w )
{
	int trex = (offset >> 19) & 0x03;
	int trex_base = 0x100 + 0x100 * trex;
	offs_t tbaseaddr = voodoo_regs[trex_base + texBaseAddr] * 8;
	int lod = (offset >> 13) & 0x3c;
	int t = (offset >> 7) & 0xff;
	int s = (offset << 1) & 0xfe;
	int twidth = trex_width[trex];
	int theight = trex_height[trex];

	tex_writes++;

	/* if we're blocked on a swap, all writes must go into the FIFO */
	if (blocked_on_swap && memory_fifo_texture)
	{
		add_to_memory_fifo(voodoo_textureram_w, offset, data, mem_mask);
		return;
	}

	if (trex >= tmus)
	{
//  Blitz hits this during its POST
//      if (trex != 3)
//          printf("TMU %d write\n", trex);
		return;
	}

	/* recompute any stale data */
	if (trex_dirty[0])
	{
		recompute_texture_params(0);
		trex_dirty[0] = 0;
	}
	if (trex_dirty[1])
	{
		recompute_texture_params(1);
		trex_dirty[1] = 0;
	}

	/* swizzle the data */
	if (voodoo_regs[trex_base + tLOD] & 0x02000000)
		data = (data >> 24) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | (data << 24);
	if (voodoo_regs[trex_base + tLOD] & 0x04000000)
		data = (data >> 16) | (data << 16);

	if (LOG_TEXTURERAM && s == 0 && t == 0)
		logerror("%06X:voodoo_textureram_w[%d,%06X,%d,%02X,%02X]", activecpu_get_pc(), trex, tbaseaddr & texram_mask, lod >> 2, s, t);

	while (lod != 0)
	{
		lod -= 4;

		if (trex_format[trex] < 8)
			tbaseaddr += twidth * theight;
		else
			tbaseaddr += 2 * twidth * theight;

		twidth >>= 1;
		if (twidth == 0)
			twidth = 1;

		theight >>= 1;
		if (theight == 0)
			theight = 1;
	}
	tbaseaddr &= texram_mask;

	if (trex_format[trex] < 8)
	{
		UINT8 *dest = textureram[trex];
		if (voodoo_regs[0x100/*trex_base -- breaks gauntleg */ + textureMode] & 0x80000000)
			tbaseaddr += t * twidth + ((s << 1) & 0xfc);
		else
			tbaseaddr += t * twidth + (s & 0xfc);

		if (LOG_TEXTURERAM && s == 0 && t == 0)
			logerror(" -> %06X = %08X\n", tbaseaddr, data);

		dest[BYTE4_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 1)] = (data >> 8) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 2)] = (data >> 16) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 3)] = (data >> 24) & 0xff;
	}
	else
	{
		UINT16 *dest = (UINT16 *)textureram[trex];
		tbaseaddr /= 2;
		tbaseaddr += t * twidth + s;

		if (LOG_TEXTURERAM && s == 0 && t == 0)
			logerror(" -> %06X = %08X\n", tbaseaddr*2, data);

		dest[BYTE_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xffff;
		dest[BYTE_XOR_LE(tbaseaddr + 1)] = (data >> 16) & 0xffff;
	}
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark REGISTER HANDLERS
#endif


/*************************************
 *
 *  Voodoo function tables
 *
 *************************************/

#include "voodregs.h"



#ifdef __MWERKS__
#pragma mark -
#pragma mark VOODOO 1 REGISTERS
#endif

/*************************************
 *
 *  Voodoo 1 registers
 *
 *************************************/

WRITE32_HANDLER( voodoo_regs_w )
{
	void (*handler)(int, offs_t, UINT32);
	offs_t regnum;
	int chips;

	reg_writes++;

	/* if we're blocked on a swap, all writes must go into the FIFO */
	if (blocked_on_swap)
	{
		add_to_memory_fifo(voodoo_regs_w, offset, data, mem_mask);
		if (offset == swapbufferCMD)
			num_pending_swaps++;
		return;
	}

	/* determine which chips we are addressing */
	chips = (offset >> 8) & 0x0f;
	if (chips == 0) chips = 0x0f;

	/* the first 64 registers can be aliased differently */
	if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
		regnum = register_alias_map[offset & 0x3f];
	else
		regnum = offset & 0xff;

	/* spread data to the chips */
	if (chips & 1)
		voodoo_regs[0x000 + regnum] = data;
	if (chips & 2)
		voodoo_regs[0x100 + regnum] = data;
	if (chips & 4)
		voodoo_regs[0x200 + regnum] = data;

	/* look up the handler and call it if valid */
	handler = voodoo_handler_w[regnum];
	if (handler)
		(*handler)(chips, regnum, data);

	/* debugging */
	status_lastpc = ~0;

	if (LOG_REGISTERS)
	{
		if (regnum < fvertexAx || regnum > fdWdY)
			logerror("%06X:voodoo %s(%d) write = %08X\n", memory_fifo_in_process ? 0 : activecpu_get_pc(), (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, data);
		else
			logerror("%06X:voodoo %s(%d) write = %f\n", memory_fifo_in_process ? 0 : activecpu_get_pc(), (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, u2f(data));
	}
}



/*************************************
 *
 *  MMIO register reads
 *
 *************************************/

READ32_HANDLER( voodoo_regs_r )
{
	UINT32 result;

	reg_reads++;

	if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
		offset = register_alias_map[offset & 0x3f];
	else
		offset &= 0xff;

	result = voodoo_regs[offset];
	switch (offset)
	{
		case status:
		{
			int fifo_space;

			result = 0;

			/* compute free FIFO space */
			fifo_space = memory_fifo_size - memory_fifo_count;
			if (fifo_space < 0)
				fifo_space = 0;

			/* PCI FIFO free space */
			result |= (fifo_space < 0x3f) ? fifo_space : 0x3f;

			/* vertical retrace */
			result |= vblank_state << 6;

			/* FBI graphics engine busy */
			result |= blocked_on_swap << 7;

			/* TREX busy */
			result |= 0 << 8;

			/* SST-1 overall busy */
			result |= (blocked_on_swap || memory_fifo_count > 0) << 9;

			/* buffer displayed (0-2) */
			result |= (frontbuf == framebuf[1]) << 10;

			/* memory FIFO free space */
			result |= ((voodoo_regs[fbiInit0] >> 13) & 1) ? (fifo_space << 12) : (0xffff << 12);

			/* swap buffers pending */
			if (num_pending_swaps < 7)
				result |= num_pending_swaps << 28;
			else
				result |= 7 << 28;

			activecpu_eat_cycles(1000);

			if (LOG_REGISTERS)
			{
				offs_t pc = activecpu_get_pc();
				if (pc == status_lastpc)
					status_lastpc_count++;
				else
				{
					if (status_lastpc_count)
						logerror("%06X:voodoo status read = %08X (x%d)\n", status_lastpc, result, status_lastpc_count);
					status_lastpc_count = 0;
					status_lastpc = pc;
					logerror("%06X:voodoo status read = %08X\n", pc, result);
				}
			}
			break;
		}

		case fbiInit2:
			/* bit 2 of the initEnable register maps this to dacRead */
			if (init_enable & 0x00000004)
				result = dac_read_result;

			if (LOG_REGISTERS)
				logerror("%06X:voodoo fbiInit2 read = %08X\n", activecpu_get_pc(), result);
			break;

		case vRetrace:
			result = cpu_getscanline();
//          if (LOG_REGISTERS)
//              logerror("%06X:voodoo vRetrace read = %08X\n", activecpu_get_pc(), result);
			break;

		/* reserved area in the TMU read by the Vegas startup sequence */
		case hvRetrace:
			result = 0x200 << 16;	/* should be between 0x7b and 0x267 */
			result |= 0x80;			/* should be between 0x17 and 0x103 */
			break;

		default:
			if (LOG_REGISTERS)
				logerror("%06X:voodoo %s read = %08X\n", activecpu_get_pc(), (offset < 0x340/4) ? voodoo_reg_name[offset] : "oob", result);
			break;
	}
	return result;
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark VOODOO 2 COMMAND FIFO
#endif

static void voodoo2_handle_register_w(offs_t offset, UINT32 data);


/*************************************
 *
 *  MMIO register writes
 *
 *************************************/

static int cmdfifo_compute_expected_depth(UINT32 *fifobase, offs_t readptr)
{
	UINT32 command = fifobase[readptr / 4];
	int i, count = 0;

	switch (command & 7)
	{
		/* packet type 0 */
		case 0:
			if (((command >> 3) & 7) == 4)
				return 2;
			return 1;

		/* packet type 1 */
		case 1:
			return 1 + (command >> 16);

		/* packet type 2 */
		case 2:
			for (i = 3; i <= 31; i++)
				if (command & (1 << i)) count++;
			return 1 + count;

		/* packet type 3 */
		case 3:
			count = 2;		/* X/Y */
			if (command & 0x10000000)
			{
				if (command & 0xc00) count++;		/* ARGB */
			}
			else
			{
				if (command & 0x400) count += 3;	/* RGB */
				if (command & 0x800) count++;		/* A */
			}
			if (command & 0x1000) count++;			/* Z */
			if (command & 0x2000) count++;			/* Wb */
			if (command & 0x4000) count++;			/* W0 */
			if (command & 0x8000) count += 2;		/* S0/T0 */
			if (command & 0x10000) count++;			/* W1 */
			if (command & 0x20000) count += 2;		/* S1/T1 */
			count *= (command >> 6) & 15;			/* numverts */
//          if (command & 0xfc00000)                /* smode != 0 */
//              count++;
			return 1 + count + (command >> 29);

		/* packet type 4 */
		case 4:
			for (i = 15; i <= 28; i++)
				if (command & (1 << i)) count++;
			return 1 + count + (command >> 29);

		/* packet type 5 */
		case 5:
			return 2 + ((command >> 3) & 0x7ffff);

		default:
			printf("UNKNOWN PACKET TYPE %d\n", command & 7);
			return 1;
	}
	return 1;
}


static UINT32 cmdfifo_execute(UINT32 *fifobase, offs_t readptr)
{
	UINT32 *src = &fifobase[readptr / 4];
	UINT32 command = *src++;
	int count, inc, code, i;
	offs_t target;

	switch (command & 7)
	{
		/* packet type 0 */
		case 0:
			target = (command >> 4) & 0x1fffffc;
			switch ((command >> 3) & 7)
			{
				case 0:		/* NOP */
					if (LOG_CMDFIFO) logerror("  NOP\n");
					break;

				case 1:		/* JSR */
					if (LOG_CMDFIFO) logerror("  JSR $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;

				case 2:		/* RET */
					if (LOG_CMDFIFO) logerror("  RET $%06X\n", target);
					break;

				case 3:		/* JMP LOCAL FRAME BUFFER */
					if (LOG_CMDFIFO) logerror("  JMP LOCAL FRAMEBUF $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;

				case 4:		/* JMP AGP */
					if (LOG_CMDFIFO) logerror("  JMP AGP $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;

				default:
					logerror("  INVALID JUMP COMMAND\n");
					break;
			}
			break;

		/* packet type 1 */
		case 1:
			count = command >> 16;
			inc = (command >> 15) & 1;
			target = (command >> 3) & 0xfff;

			if (LOG_CMDFIFO) logerror("  PACKET TYPE 1: count=%d inc=%d reg=%04X\n", count, inc, target);
			for (i = 0; i < count; i++, target += inc)
				voodoo2_handle_register_w(target, *src++);
			break;

		/* packet type 2 */
		case 2:
			if (LOG_CMDFIFO) logerror("  PACKET TYPE 2: mask=%X\n", (command >> 3) & 0x1ffffff);
			for (i = 3; i <= 31; i++)
				if (command & (1 << i))
					voodoo2_handle_register_w(bltSrcBaseAddr + (i - 3), *src++);
			break;

		/* packet type 3 */
		case 3:
			count = (command >> 6) & 15;
			code = (command >> 3) & 7;
			if (LOG_CMDFIFO) logerror("  PACKET TYPE 3: count=%d code=%d mask=%03X smode=%02X pc=%d\n", count, code, (command >> 10) & 0xfff, (command >> 22) & 0x3f, (command >> 28) & 1);

			voodoo_regs[sSetupMode] = ((command >> 10) & 0xfff) | ((command >> 6) & 0xf0000);
			for (i = 0; i < count; i++)
			{
				setup_pending.x = (INT16)TRUNC_TO_INT(u2f(*src++) * 16. + 0.5) * (1. / 16.);
				setup_pending.y = (INT16)TRUNC_TO_INT(u2f(*src++) * 16. + 0.5) * (1. / 16.);

				if (command & 0x10000000)
				{
					if (voodoo_regs[sSetupMode] & 0x0003)
					{
						UINT32 argb = *src++;
						if (voodoo_regs[sSetupMode] & 0x0001)
						{
							setup_pending.r = (argb >> 16) & 0xff;
							setup_pending.g = (argb >> 8) & 0xff;
							setup_pending.b = argb & 0xff;
						}
						if (voodoo_regs[sSetupMode] & 0x0002)
							setup_pending.a = argb >> 24;
					}
				}
				else
				{
					if (voodoo_regs[sSetupMode] & 0x0001)
					{
						setup_pending.r = *(float *)src++;
						setup_pending.g = *(float *)src++;
						setup_pending.b = *(float *)src++;
					}
					if (voodoo_regs[sSetupMode] & 0x0002)
						setup_pending.a = *(float *)src++;
				}

				if (voodoo_regs[sSetupMode] & 0x0004)
					setup_pending.z = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0008)
					setup_pending.wb = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0010)
					setup_pending.w0 = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0020)
				{
					setup_pending.s0 = *(float *)src++;
					setup_pending.t0 = *(float *)src++;
				}
				if (voodoo_regs[sSetupMode] & 0x0040)
					setup_pending.w1 = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0080)
				{
					setup_pending.s1 = *(float *)src++;
					setup_pending.t1 = *(float *)src++;
				}

				if ((code == 1 && i == 0) || (code == 0 && i % 3 == 0))
				{
					setup_count = 1;
					setup_verts[0] = setup_verts[1] = setup_verts[2] = setup_pending;
				}
				else
				{
					if (!(voodoo_regs[sSetupMode] & 0x10000))	/* strip mode */
						setup_verts[0] = setup_verts[1];
					setup_verts[1] = setup_verts[2];
					setup_verts[2] = setup_pending;
					if (++setup_count >= 3)
						setup_and_draw_triangle();
				}
			}
			src += command >> 29;
			break;

		/* packet type 4 */
		case 4:
			target = (command >> 3) & 0xfff;

			if (LOG_CMDFIFO) logerror("  PACKET TYPE 4: mask=%X reg=%04X pad=%d\n", (command >> 15) & 0x3fff, target, command >> 29);
			for (i = 15; i <= 28; i++)
				if (command & (1 << i))
					voodoo2_handle_register_w(target + (i - 15), *src++);
			src += command >> 29;
			break;

		/* packet type 5 */
		case 5:
			count = (command >> 3) & 0x7ffff;
			target = *src++ / 4;

			if ((command >> 30) == 2)
			{
				if (LOG_CMDFIFO) logerror("  PACKET TYPE 5: LFB count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);
				for (i = 0; i < count; i++)
					voodoo_framebuf_w(target++, *src++, 0);
			}
			else if ((command >> 30) == 3)
			{
				if (LOG_CMDFIFO) logerror("  PACKET TYPE 5: textureRAM count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);
				for (i = 0; i < count; i++)
					voodoo_textureram_w(target++, *src++, 0);
			}
			break;

		default:
			fprintf(stderr, "PACKET TYPE %d\n", command & 7);
			break;
	}

	/* by default just update the read pointer past all the data we consumed */
	return 4 * (src - fifobase);
}


static void cmdfifo_process_pending(UINT32 old_depth)
{
	/* if we have data, process it */
	if (voodoo_regs[cmdFifoDepth])
	{
		/* if we didn't have data before, use the first word to compute the expected count */
		if (old_depth == 0)
		{
			cmdfifo_expected = cmdfifo_compute_expected_depth(cmdfifo, voodoo_regs[cmdFifoRdPtr]);
			if (LOG_CMDFIFO_VERBOSE) logerror("PACKET TYPE %d, expecting %d words\n", cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7, cmdfifo_expected);
		}

		/* if we got everything, execute */
		if (voodoo_regs[cmdFifoDepth] >= cmdfifo_expected)
		{
			voodoo_regs[cmdFifoRdPtr] = cmdfifo_execute(cmdfifo, voodoo_regs[cmdFifoRdPtr]);
			voodoo_regs[cmdFifoDepth] -= cmdfifo_expected;
		}
	}
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark VOODOO 2 REGISTERS
#endif

/*************************************
 *
 *  Voodoo 2 registers
 *
 *************************************/

static void voodoo2_handle_register_w(offs_t offset, UINT32 data)
{
	void (*handler)(int, offs_t, UINT32);
	offs_t regnum;
	int chips;

	/* determine which chips we are addressing */
	chips = (offset >> 8) & 0x0f;
	if (chips == 0) chips = 0x0f;

	/* the first 64 registers can be aliased differently */
	if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
		regnum = register_alias_map[offset & 0x3f];
	else
		regnum = offset & 0xff;

	/* spread data to the chips */
	if (chips & 1)
		voodoo_regs[0x000 + regnum] = data;
	if (chips & 2)
		voodoo_regs[0x100 + regnum] = data;
	if (chips & 4)
		voodoo_regs[0x200 + regnum] = data;

	/* look up the handler and call it if valid */
	handler = voodoo2_handler_w[regnum];
	if (handler)
		(*handler)(chips, regnum, data);

	/* debugging */
	status_lastpc = ~0;

	if (LOG_REGISTERS)
	{
		if (regnum < fvertexAx || regnum > fdWdY)
			logerror("%06X:voodoo %s(%d) write = %08X\n", memory_fifo_in_process ? 0 : activecpu_get_pc(), (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, data);
		else
			logerror("%06X:voodoo %s(%d) write = %f\n", memory_fifo_in_process ? 0 : activecpu_get_pc(), (regnum < 0x384/4) ? voodoo_reg_name[regnum] : "oob", chips, u2f(data));
	}
}


WRITE32_HANDLER( voodoo2_regs_w )
{
	UINT32 old_depth;
	offs_t addr;

	reg_writes++;

	/* if we're blocked on a swap, all writes must go into the FIFO */
	if (blocked_on_swap)
	{
		add_to_memory_fifo(voodoo2_regs_w, offset, data, mem_mask);
		if (offset == swapbufferCMD)
			num_pending_swaps++;
		return;
	}

	/* handle legacy writes */
	if (!(voodoo_regs[fbiInit7] & 0x100))
	{
		voodoo2_handle_register_w(offset, data);
		return;
	}

	/* handle legacy writes that still work in FIFO mode */
	if (!(offset & 0x80000))
	{
		/* only very particular commands go through in FIFO mode */
		if (voodoo2_cmdfifo_writethrough[offset & 0xff])
			voodoo2_handle_register_w(offset, data);
		else
			logerror("Threw out write to %s\n", ((offset & 0xff) < 0x384/4) ? voodoo_reg_name[(offset & 0xff)] : "oob");

		/* otherwise, drop it on the floor */
		return;
	}

	/* handle writes to the command FIFO */
	addr = ((voodoo_regs[cmdFifoBaseAddr] & 0x3ff) << 12) + ((offset & 0xffff) * 4);
	old_depth = voodoo_regs[cmdFifoDepth];

	/* swizzling */
	if (offset & 0x10000)
		data = (data >> 24) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | (data << 24);
	cmdfifo[addr/4] = data;

	/* count holes? */
	if (!(voodoo_regs[fbiInit7] & 0x400))
	{
		/* in-order, no holes */
		if (voodoo_regs[cmdFifoHoles] == 0 && addr == voodoo_regs[cmdFifoAMin] + 4)
		{
			voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = addr;
			voodoo_regs[cmdFifoDepth]++;
		}

		/* out-of-order, but within the min-max range */
		else if (addr < voodoo_regs[cmdFifoAMax])
		{
			voodoo_regs[cmdFifoHoles]--;
			if (voodoo_regs[cmdFifoHoles] == 0)
			{
				voodoo_regs[cmdFifoDepth] += (voodoo_regs[cmdFifoAMax] - voodoo_regs[cmdFifoAMin]) / 4;
				voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax];
			}
		}

		/* out-of-order, bumping max */
		else
		{
			voodoo_regs[cmdFifoHoles] += (addr - voodoo_regs[cmdFifoAMax]) / 4 - 1;
			voodoo_regs[cmdFifoAMax] = addr;
		}
	}

	if (LOG_CMDFIFO_VERBOSE)
	{
		if ((cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7) == 3)
			logerror("CMDFIFO(%06X)=%f  (min=%06X max=%06X d=%d h=%d)\n", addr, u2f(data), voodoo_regs[cmdFifoAMin], voodoo_regs[cmdFifoAMax], voodoo_regs[cmdFifoDepth], voodoo_regs[cmdFifoHoles]);
		else if ((cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7) != 5)
			logerror("CMDFIFO(%06X)=%08X  (min=%06X max=%06X d=%d h=%d)\n", addr, data, voodoo_regs[cmdFifoAMin], voodoo_regs[cmdFifoAMax], voodoo_regs[cmdFifoDepth], voodoo_regs[cmdFifoHoles]);
	}

	/* process anything pending */
	cmdfifo_process_pending(old_depth);
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark VOODOO 3 REGISTERS
#endif

/*************************************
 *
 *  Voodoo 3 stubs
 *
 *************************************/

READ32_HANDLER( voodoo3_rom_r )
{
	logerror("%08X:voodoo_rom_r(%X) & %08X\n", activecpu_get_pc(), offset*4, ~mem_mask);
	return ~0;
}

WRITE32_HANDLER( voodoo3_ioreg_w )
{
	logerror("%08X:voodoo3_ioreg_w(%X) = %08X\n", activecpu_get_pc(), offset*4, data);
}

READ32_HANDLER( voodoo3_ioreg_r )
{
	logerror("%08X:voodoo3_ioreg_r(%X) & %08X\n", activecpu_get_pc(), offset*4, ~mem_mask);
	return ~0;
}

WRITE32_HANDLER( voodoo3_cmdagp_w )
{
	logerror("%08X:voodoo3_cmdagp_w(%X) = %08X\n", activecpu_get_pc(), offset*4, data);
}

READ32_HANDLER( voodoo3_cmdagp_r )
{
	logerror("%08X:voodoo3_cmdagp_r(%X) & %08X\n", activecpu_get_pc(), offset*4, ~mem_mask);
	return ~0;
}

WRITE32_HANDLER( voodoo3_2d_w )
{
	logerror("%08X:voodoo3_2d_w(%X) = %08X\n", activecpu_get_pc(), offset*4, data);
}

READ32_HANDLER( voodoo3_2d_r )
{
	logerror("%08X:voodoo3_2d_r(%X) & %08X\n", activecpu_get_pc(), offset*4, ~mem_mask);
	return ~0;
}

WRITE32_HANDLER( voodoo3_yuv_w )
{
	logerror("%08X:voodoo3_yuv_w(%X) = %08X\n", activecpu_get_pc(), offset*4, data);
}

READ32_HANDLER( voodoo3_yuv_r )
{
	logerror("%08X:voodoo3_yuv_r(%X) & %08X\n", activecpu_get_pc(), offset*4, ~mem_mask);
	return ~0;
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark OTHER PIECES
#endif

/* include LFB management code */
#include "voodlfb.h"

/* include rasterization code */
#include "vooddraw.h"
