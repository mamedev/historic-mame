#include "x86drc.h"
#define LOG_CODE		(1)

#define BITFIELD(var,shift,bits) \
	(((var) >> (shift)) & ((1 << (bits)) - 1))

#define FBZCOLORPATH_BITS(start,len) \
	BITFIELD(voodoo_regs[fbzColorPath], start, len)

#define FOGMODE_BITS(start,len) \
	BITFIELD(voodoo_regs[fogMode], start, len)

#define ALPHAMODE_BITS(start,len) \
	BITFIELD(voodoo_regs[alphaMode], start, len)

#define FBZMODE_BITS(start,len) \
	BITFIELD(voodoo_regs[fbzMode], start, len)

#define TEXTUREMODE_BITS(tmu,start,len) \
	BITFIELD(voodoo_regs[0x100 + (tmu*0x100) + textureMode], start, len)

#define TEXTUREMODE0_BITS(start,len) \
	BITFIELD(voodoo_regs[0x100 + textureMode], start, len)

#define TEXTUREMODE1_BITS(start,len) \
	BITFIELD(voodoo_regs[0x200 + textureMode], start, len)

static const float _1_over_256 = 1.0 / 256.0;
static void *_vmin, *_vmid;
static UINT32 _curry, _stopy, _stopx;
static UINT32 _lod, _depthval;
static float _dxdy_midmax, _dxdy_minmid, _dxdy_minmax;
static float _wfloat;

struct aligned_render_data
{
	/* dynamic variables */
	float start_argb[4];
	float dargb_dx[4];
	UINT32 dargb_idx[4];
	float dargb_dy[4];
	float start_st01[4];
	float dst01_dx[4];
	float dst01_dy[4];
	float start_w01[4];
	float dw01_dx[4];
	float dw01_dy[4];
	float start_wz[4];
	float dwz_dx[4];
	float dwz_dy[4];
	float lodbase[4];
	float lodresult[4];
	UINT32 texmask[4];
	UINT16 texmax[8];
	UINT32 tempsave1[4];
	UINT32 tempsave2[4];
	UINT32 uvsave[4];

	/* misc constant values */
	float wz_scale[4];
	float zw_scale[4];
	float zfw_scale[4];
	UINT32 splus1[9][4];
	UINT32 tplus1[9][4];
	UINT32 invert_colors[4];
	UINT32 invert_alpha[4];
	UINT32 invert_all[4];

	/* constant fp values */
	float f32_0[4];
	float f32_0p5[4];
	float f32_1[4];
	float f32_256[4];

	/* constant UINT32 values */
	UINT32 i32_0xff[4];

	/* constant UINT16 values */
	UINT16 i16_0[8];
	UINT16 i16_1[8];
	UINT16 i16_0xff[8];

	/* big tables */
	UINT16 dither_4x4_expand[16][8];
	UINT16 dither_2x2_expand[16][8];
	UINT16 bilinear_table[65536][4];	/* 512k */
	UINT16 uncolor_table[65536][4];		/* 512k */
};

static struct aligned_render_data *ard;
static struct aligned_render_data *ard_alloc;
static drc_core *voodoo_drc;

static void cache_reset(drc_core *drc)
{
	/* reset the cache */
	memset(raster_hash, 0, sizeof(raster_hash));
}

static int init_generator(void)
{
	static drc_config config =
	{
		1024*1024,		/* size of cache to allocate */
		1,				/* maximum instructions per sequence */
		1,				/* number of live address bits in the PC */
		0,				/* number of LSBs to ignore on the PC */
		0,				/* true if we need the FP unit */
		0,				/* true if we need the SSE unit */
		0,				/* true if the PC is stored in memory */
		0,				/* true if the icount is stored in memory */

		NULL,			/* pointer to where the PC is stored */
		NULL,			/* pointer to where the icount is stored */
		NULL,			/* pointer to where the volatile data in ESI is stored */

		cache_reset,	/* callback when the cache is reset */
		NULL,			/* callback when code needs to be recompiled */
		NULL			/* callback before generating the dispatcher on entry */
	};
	int i;

	/* allocate a code generator */
	voodoo_drc = drc_init(0, &config);
	if (!voodoo_drc)
		return 1;

	/* allocate aligned data */
	ard_alloc = malloc(sizeof(*ard) + 16);
	if (!ard_alloc)
		return 1;

	/* align it */
	ard = (struct aligned_render_data *)(((FPTR)ard_alloc + 0x0f) & ~0x0f);
	memset(ard, 0, sizeof(*ard));

	/* initialize misc constants */
	ard->wz_scale[0] = (float)(1 << 28);
	ard->wz_scale[1] = 1.0 / 4096.0;

	ard->zw_scale[0] = 1.0 / 4096.0;
	ard->zw_scale[1] = (float)(1 << 28);

	ard->zfw_scale[0] = 16.0;
	ard->zfw_scale[1] = (float)(1 << 28);

	for (i = 0; i <= 8; i++)
	{
		ard->splus1[i][0] = ard->splus1[i][2] = 0 << i;
		ard->splus1[i][1] = ard->splus1[i][3] = 1 << i;
		ard->tplus1[i][0] = ard->tplus1[i][2] = 1 << i;
		ard->tplus1[i][1] = ard->tplus1[i][3] = 0 << i;
	}

	ard->invert_colors[0] = 0x00ffffff;
	ard->invert_alpha[0] = 0xff000000;
	ard->invert_all[0] = 0xffffffff;

	/* initialize constant values */
	for (i = 0; i < 4; i++)
	{
		ard->f32_0[i]   = 0.0;
		ard->f32_0p5[i] = 0.5;
		ard->f32_1[i]   = 1.0;
		ard->f32_256[i] = 256.0;

		ard->i32_0xff[i] = 0xff;
	}

	for (i = 0; i < 8; i++)
	{
		ard->i16_0[i] = 0;
		ard->i16_1[i] = 1;
		ard->i16_0xff[i] = 0xff;
	}

	/* initialize the dither tables */
	for (i = 0; i < 16; i++)
	{
		ard->dither_4x4_expand[i][0] = ard->dither_4x4_expand[i][1] =
		ard->dither_4x4_expand[i][2] = ard->dither_4x4_expand[i][3] = dither_matrix_4x4[i];
		ard->dither_2x2_expand[i][0] = ard->dither_2x2_expand[i][1] =
		ard->dither_2x2_expand[i][2] = ard->dither_2x2_expand[i][3] = dither_matrix_2x2[i];
	}

	/* initialize the bilinear table */
	for (i = 0; i < 65536; i++)
	{
		int s = (i >> 8);
		int t = i & 0xff;
		int tot;
		tot = ard->bilinear_table[i][0] = ((0x100 - s) * (0x100 - t)) >> 8;
		tot += ard->bilinear_table[i][1] = (s * (0x100 - t)) >> 8;
		tot += ard->bilinear_table[i][2] = (s * t) >> 8;
		ard->bilinear_table[i][3] = 0x100 - tot;
	}

	/* initialize the uncolor table */
	for (i = 0; i < 65536; i++)
	{
		int r = (i >> 11) & 0x1f;
		int g = (i >> 5) & 0x3f;
		int b = i & 0x1f;
		ard->uncolor_table[i][3] = 0;
		ard->uncolor_table[i][2] = (r << 3) | (r >> 2);
		ard->uncolor_table[i][1] = (g << 2) | (g >> 4);
		ard->uncolor_table[i][0] = (b << 3) | (b >> 2);
	}
	return 0;
}


static void generate_load_texel(drc_core *drc, int tmu, void *lookup)
{
	int clamp_both = (TEXTUREMODE_BITS(tmu,6,1) && TEXTUREMODE_BITS(tmu,7,1));
	int wrap_both = (!TEXTUREMODE_BITS(tmu,6,1) && !TEXTUREMODE_BITS(tmu,7,1));
	int needs_point = 0, needs_bilinear = 0;
	link_info bilinear_link1 = {0};

	/* determine what we need to know about the filtering */
	if (BILINEAR_FILTER)
	{
		switch (TEXTUREMODE_BITS(tmu,1,2))
		{
			case 0:		/* both min and mag are point sampled */
				needs_point = 1;
				needs_bilinear = 0;
				break;

			case 1:		/* min is bilinear, mag is point sampled */
				if (PER_PIXEL_LOD)
					needs_point = needs_bilinear = 1;
				else
				{
					needs_point = 0;
					needs_bilinear = 1;
				}
				break;

			case 2:		/* min is point sampled, mag is bilinear */
				if (PER_PIXEL_LOD)
					needs_point = needs_bilinear = 1;
				else
				{
					needs_point = 1;
					needs_bilinear = 0;
				}
				break;

			case 3:		/* both min and mag are bilinear */
				needs_point = 0;
				needs_bilinear = 1;
				break;
		}
	}
	else
	{
		needs_point = 1;
		needs_bilinear = 0;
	}

	/* lookup LOD */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (PER_PIXEL_LOD)
	{
		_movzx_r32_m16abs(REG_ECX, (UINT16 *)&ard->lodresult[2-2*tmu] + 1);	// movzx    ecx,_lodresult[2-2*tmu].upper16
		_movzx_r32_m16isd(REG_ECX, REG_ECX, 2, &lod_lookup[0]);			// movzx    ecx,lod_lookup[ecx*2]

		/* clamp LOD */
		/* ECX = lod, ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
		_add_r32_m32abs(REG_ECX, &trex_lodbias[tmu]);					// add      ecx,trex_lodbias[tmu]
		if (TEXTUREMODE_BITS(tmu,4,1))
		{
			_mov_r32_m32abs(REG_EBX, &_curry);							// mov      ebx,_curry
			_mov_r32_r32(REG_EDX, REG_EBP);								// mov      edx,ebp
			_and_r32_imm(REG_EBX, 3);									// and      ebx,3
			_and_r32_imm(REG_EDX, 3);									// and      edx,3
			_movzx_r32_m8bisd(REG_EBX, REG_EDX, REG_EBX, 4, &lod_dither_matrix[0]);// movzx ebx,[edx+ebx*4+lod_dither_matrix]
			_add_r32_r32(REG_ECX, REG_EBX);								// add      ecx,ebx
		}
		_cmp_r32_m32abs(REG_ECX, &trex_lodmin[tmu]);					// cmp      ecx,trex_lodmin[tmu]
		_cmov_r32_m32abs(COND_L, REG_ECX, &trex_lodmin[tmu]);			// cmovl    ecx,trex_lodmin[tmu]
		if (needs_point == needs_bilinear)
			_setcc_r8(COND_L, REG_AL);									// setl     al
		_cmp_r32_m32abs(REG_ECX, &trex_lodmax[tmu]);					// cmp      ecx,trex_lodmax[tmu]
		_cmov_r32_m32abs(COND_G, REG_ECX, &trex_lodmax[tmu]);			// cmovg    ecx,trex_lodmax[tmu]
	}
	else
		_mov_r32_m32abs(REG_ECX, &trex_lodmin[tmu]);					// mov      ecx,trex_lodmin[tmu]

	/* compute texture base */
	/* AL = testmag, ECX = lod, ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	_mov_m32abs_r32(&_lod, REG_ECX);									// mov      _lod,ecx
	_shr_r32_imm(REG_ECX, 8);											// shr      ecx,8
	_mov_r32_m32isd(REG_EDX, REG_ECX, 4, &trex_lod_start[tmu][0]);		// mov      edx,[ecx*4+trex_lod_start[tmu]]

	/* determine which filter to use; we only need to check for the case where min/mag options are different */
	/* AL = testmag, ECX = lod, EDX -> texbase, ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (needs_point == needs_bilinear)
	{
		_test_r8_imm(REG_AL, 0xff);										// test     al,0xff
		if (TEXTUREMODE_BITS(tmu,1,1))
			_jcc_near_link(COND_Z, &bilinear_link1);					// jz       bilinear
		else
			_jcc_near_link(COND_NZ, &bilinear_link1);					// jnz      bilinear
	}

	/* point-sampled filter */
	/* ECX = lod, EDX -> texbase, ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (needs_point)
	{
		/* get lodwidth value in CL */
		_mov_r32_m32abs(REG_EAX, &trex_lod_width_shift[tmu]);			// mov      eax,trex_lod_width_shift[tmu]
		_movzx_r32_m8bisd(REG_EAX, REG_EAX, REG_ECX, 1, 0);				// mov      eax,[eax+ecx]
		_bswap_r32(REG_ECX);											// bswap    ecx
		_or_r32_r32(REG_ECX, REG_EAX);									// or       ecx,eax

		/* clamp W */
		if (TEXTUREMODE_BITS(tmu,3,1))
		{
			_xorps_r128_r128(REG_XMM0, REG_XMM0);						// xorps    xmm0,xmm0
			_cmpps_r128_r128(REG_XMM0, REG_XMM5, 1);					// cmpps    xmm0,xmm5,lt
			_pshufd_r128_r128(REG_XMM0, REG_XMM0, (tmu == 0) ? 0xaa : 0x00);// pshufd   xmm0,xmm0,(tmu == 0) ? 0xaa : 0x00
			_pand_r128_r128(REG_XMM2, REG_XMM0);						// pand     xmm2,xmm0
		}

		/* get final S,T values */
		_psrld_r128_imm(REG_XMM2, 8);									// psrld    xmm2,8
		if (!clamp_both)
			_pand_r128_m128abs(REG_XMM2, &ard->texmask[0]);				// pand     xmm2,ard->texmask
		_packssdw_r128_r128(REG_XMM2, REG_XMM2);						// packsswd xmm2,xmm2
		if (!wrap_both)
		{
			_pminsw_r128_m128abs(REG_XMM2, &ard->texmax[0]);			// pminsw   xmm2,ard->texmax
			_pmaxsw_r128_m128abs(REG_XMM2, &ard->i16_0[0]);				// pmaxsw   xmm2,ard->i16_0
		}
		_bswap_r32(REG_ECX);											// bswap    ecx
		_pextrw_r32_r128(REG_EBX, REG_XMM2, (tmu == 0) ? 2 : 0);		// pextrw   ebx,xmm2,2 or 0
		_pextrw_r32_r128(REG_EAX, REG_XMM2, (tmu == 0) ? 3 : 1);		// pextrw   eax,xmm2,3 or 1
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_bswap_r32(REG_ECX);											// bswap    ecx
		_shl_r32_cl(REG_EBX);											// shl      ebx,cl
		_add_r32_r32(REG_EBX, REG_EAX);									// add      ebx,eax

		/* fetch raw texel data */
		if (!TEXTUREMODE_BITS(tmu,11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EDX, REG_EBX, 1, 0);			// movzx    eax,[edx+ebx+0]
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EDX, REG_EBX, 2, 0);		// movzx    eax,[edx+ebx*2+0]

		/* convert to ARGB */
		_movd_r128_m32isd(REG_XMM2, REG_EAX, 4, lookup);				// movd     xmm2,[eax*4+lookup]
		_pxor_r128_r128(REG_XMM1, REG_XMM1);							// pxor     xmm1,xmm1
		_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);						// punpcklbw xmm2,xmm1
	}

	/* bilinear filter */
	/* ECX = lod, EDX -> texbase, ESI -> depth, EDI -> draw, EBP = x, XMM2 = u0v0u1v1, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (needs_bilinear)
	{
		if (needs_point)
			_resolve_link(&bilinear_link1);								// bilinear:

		_movaps_m128abs_r128(&ard->tempsave1[0], REG_XMM4);				// movaps   ard->tempsave1,xmm4
		_movaps_m128abs_r128(&ard->tempsave2[0], REG_XMM5);				// movaps   ard->tempsave2,xmm5

		/* subtract 128 << ilod from the S/T values */
		_mov_r32_imm(REG_EAX, 128);										// mov      eax,128
		_shl_r32_cl(REG_EAX);											// shl      eax,cl
		_movd_r128_r32(REG_XMM0, REG_EAX);								// movd     xmm0,eax
		_pshufd_r128_r128(REG_XMM0, REG_XMM0, 0x00);					// pshufd   xmm0,xmm0,0
		_psubd_r128_r128(REG_XMM2, REG_XMM0);							// psubd    xmm2,xmm0

		/* clamp W */
		if (TEXTUREMODE_BITS(tmu,3,1))
		{
			_xorps_r128_r128(REG_XMM0, REG_XMM0);						// xorps    xmm0,xmm0
			_cmpps_r128_r128(REG_XMM0, REG_XMM5, 1);					// cmpps    xmm0,xmm5,lt
			_pshufd_r128_r128(REG_XMM0, REG_XMM0, (tmu == 0) ? 0xaa : 0x00);// pshufd   xmm0,xmm0,(tmu == 0) ? 0xaa : 0x00
			_pand_r128_r128(REG_XMM2, REG_XMM0);						// pand     xmm2,xmm0
		}

		/* get the address of the bilinear multipliers in EBP */
		_pextrw_r32_r128(REG_EAX, REG_XMM2, (tmu == 0) ? 6 : 2);		// pextrw   eax,xmm2,6 or 2
		_pextrw_r32_r128(REG_EBX, REG_XMM2, (tmu == 0) ? 4 : 0);		// pextrw   ebx,xmm2,4 or 0
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_and_r32_imm(REG_EAX, 0xff);									// and      eax,0xff
		_and_r32_imm(REG_EBX, 0xff);									// and      ebx,0xff
		_shl_r32_imm(REG_EAX, 11);										// shl      eax,11
		_movq_r128_m64bisd(REG_XMM4, REG_EAX, REG_EBX, 8, ard->bilinear_table);// movq xmm4,[eax+ebx*8+ard->bilinear_table]

		/* get lodwidth value in CL */
		_mov_r32_m32abs(REG_EAX, &trex_lod_width_shift[tmu]);			// mov      eax,trex_lod_width_shift[tmu]
		_movzx_r32_m8bisd(REG_EAX, REG_EAX, REG_ECX, 1, 0);				// mov      eax,[eax+ecx]
		_bswap_r32(REG_ECX);											// bswap    ecx
		_or_r32_r32(REG_ECX, REG_EAX);									// or       ecx,eax

		/* pre-shift XMM2 */
		_psrld_r128_imm(REG_XMM2, 8);									// psrld    xmm2,8

		/* get final S,T values */
		_movdqa_r128_r128(REG_XMM1, REG_XMM2);							// movdqa   xmm1,xmm2
		if (!clamp_both)
			_pand_r128_m128abs(REG_XMM1, &ard->texmask[0]);				// pand     xmm1,ard->texmask
		_packssdw_r128_r128(REG_XMM1, REG_XMM1);						// packsswd xmm1,xmm1
		if (!wrap_both)
		{
			_pminsw_r128_m128abs(REG_XMM1, &ard->texmax[0]);			// pminsw   xmm1,ard->texmax
			_pmaxsw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// pmaxsw   xmm1,ard->i16_0
		}
		_bswap_r32(REG_ECX);											// bswap    ecx
		_pextrw_r32_r128(REG_EBX, REG_XMM1, (tmu == 0) ? 2 : 0);		// pextrw   ebx,xmm1,2 or 0
		_pextrw_r32_r128(REG_EAX, REG_XMM1, (tmu == 0) ? 3 : 1);		// pextrw   eax,xmm1,3 or 1
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_bswap_r32(REG_ECX);											// bswap    ecx
		_shl_r32_cl(REG_EBX);											// shl      ebx,cl
		_add_r32_r32(REG_EBX, REG_EAX);									// add      ebx,eax

		/* fetch raw texel data */
		if (!TEXTUREMODE_BITS(tmu,11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EDX, REG_EBX, 1, 0);			// movzx    eax,[edx+ebx+0]
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EDX, REG_EBX, 2, 0);		// movzx    eax,[edx+ebx*2+0]

		/* convert to ARGB */
		_movd_r128_m32isd(REG_XMM0, REG_EAX, 4, lookup);				// movd     xmm0,[eax*4+lookup]
		_mov_r32_r32(REG_EBX, REG_ECX);									// mov      ebx,ecx
		_pshuflw_r128_r128(REG_XMM5, REG_XMM4, 0x00);					// pshuflw  xmm5,xmm4,0x00
		_shr_r32_imm(REG_EBX, 20);										// shr      ebx,20
		_punpcklbw_r128_m128abs(REG_XMM0, &ard->i16_0[0]);				// punpcklbw xmm0,ard->i16_0
		_paddd_r128_m128bd(REG_XMM2, REG_EBX, &ard->splus1[0][0]);		// paddd    xmm2,[ebx+ard->splus1]
		_pmullw_r128_r128(REG_XMM0, REG_XMM5);							// pmullw   xmm0,xmm5

		/* get final S,T values */
		_movdqa_r128_r128(REG_XMM1, REG_XMM2);							// movdqa   xmm1,xmm2
		if (!clamp_both)
			_pand_r128_m128abs(REG_XMM1, &ard->texmask[0]);				// pand     xmm1,ard->texmask
		_packssdw_r128_r128(REG_XMM1, REG_XMM1);						// packsswd xmm1,xmm1
		if (!wrap_both)
		{
			_pminsw_r128_m128abs(REG_XMM1, &ard->texmax[0]);			// pminsw   xmm1,ard->texmax
			_pmaxsw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// pmaxsw   xmm1,ard->i16_0
		}
		_bswap_r32(REG_ECX);											// bswap    ecx
		_pextrw_r32_r128(REG_EBX, REG_XMM1, (tmu == 0) ? 2 : 0);		// pextrw   ebx,xmm1,2 or 0
		_pextrw_r32_r128(REG_EAX, REG_XMM1, (tmu == 0) ? 3 : 1);		// pextrw   eax,xmm1,3 or 1
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_bswap_r32(REG_ECX);											// bswap    ecx
		_shl_r32_cl(REG_EBX);											// shl      ebx,cl
		_add_r32_r32(REG_EBX, REG_EAX);									// add      ebx,eax

		/* fetch raw texel data */
		if (!TEXTUREMODE_BITS(tmu,11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EDX, REG_EBX, 1, 0);			// movzx    eax,[edx+ebx+0]
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EDX, REG_EBX, 2, 0);		// movzx    eax,[edx+ebx*2+0]

		/* convert to ARGB */
		_movd_r128_m32isd(REG_XMM1, REG_EAX, 4, lookup);				// movd     xmm1,[eax*4+lookup]
		_mov_r32_r32(REG_EBX, REG_ECX);									// mov      ebx,ecx
		_pshuflw_r128_r128(REG_XMM5, REG_XMM4, 0x55);					// pshuflw  xmm5,xmm4,0x55
		_shr_r32_imm(REG_EBX, 20);										// shr      ebx,20
		_punpcklbw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// punpcklbw xmm1,ard->i16_0
		_paddd_r128_m128bd(REG_XMM2, REG_EBX, &ard->tplus1[0][0]);		// paddd    xmm2,[ebx+ard->tplus1]
		_pmullw_r128_r128(REG_XMM1, REG_XMM5);							// pmullw   xmm1,xmm5
		_paddw_r128_r128(REG_XMM0, REG_XMM1);							// paddw    xmm0,xmm1

		/* get final S,T values */
		_movdqa_r128_r128(REG_XMM1, REG_XMM2);							// movdqa   xmm1,xmm2
		if (!clamp_both)
			_pand_r128_m128abs(REG_XMM1, &ard->texmask[0]);				// pand     xmm1,ard->texmask
		_packssdw_r128_r128(REG_XMM1, REG_XMM1);						// packsswd xmm1,xmm1
		if (!wrap_both)
		{
			_pminsw_r128_m128abs(REG_XMM1, &ard->texmax[0]);			// pminsw   xmm1,ard->texmax
			_pmaxsw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// pmaxsw   xmm1,ard->i16_0
		}
		_bswap_r32(REG_ECX);											// bswap    ecx
		_pextrw_r32_r128(REG_EBX, REG_XMM1, (tmu == 0) ? 2 : 0);		// pextrw   ebx,xmm1,2 or 0
		_pextrw_r32_r128(REG_EAX, REG_XMM1, (tmu == 0) ? 3 : 1);		// pextrw   eax,xmm1,3 or 1
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_bswap_r32(REG_ECX);											// bswap    ecx
		_shl_r32_cl(REG_EBX);											// shl      ebx,cl
		_add_r32_r32(REG_EBX, REG_EAX);									// add      ebx,eax

		/* fetch raw texel data */
		if (!TEXTUREMODE_BITS(tmu,11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EDX, REG_EBX, 1, 0);			// movzx    eax,[edx+ebx+0]
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EDX, REG_EBX, 2, 0);		// movzx    eax,[edx+ebx*2+0]

		/* convert to ARGB */
		_movd_r128_m32isd(REG_XMM1, REG_EAX, 4, lookup);				// movd     xmm1,[eax*4+lookup]
		_mov_r32_r32(REG_EBX, REG_ECX);									// mov      ebx,ecx
		_pshuflw_r128_r128(REG_XMM5, REG_XMM4, 0xaa);					// pshuflw  xmm5,xmm4,0xaa
		_shr_r32_imm(REG_EBX, 20);										// shr      ebx,20
		_punpcklbw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// punpcklbw xmm1,ard->i16_0
		_psubd_r128_m128bd(REG_XMM2, REG_EBX, &ard->splus1[0][0]);		// psubd    xmm2,[ebx+ard->splus1]
		_pmullw_r128_r128(REG_XMM1, REG_XMM5);							// pmullw   xmm1,xmm5
		_paddw_r128_r128(REG_XMM0, REG_XMM1);							// paddw    xmm0,xmm1

		/* get final S,T values */
		_movdqa_r128_r128(REG_XMM1, REG_XMM2);							// movdqa   xmm1,xmm2
		if (!clamp_both)
			_pand_r128_m128abs(REG_XMM1, &ard->texmask[0]);				// pand     xmm1,ard->texmask
		_packssdw_r128_r128(REG_XMM1, REG_XMM1);						// packsswd xmm1,xmm1
		if (!wrap_both)
		{
			_pminsw_r128_m128abs(REG_XMM1, &ard->texmax[0]);			// pminsw   xmm1,ard->texmax
			_pmaxsw_r128_m128abs(REG_XMM1, &ard->i16_0[0]);				// pmaxsw   xmm1,ard->i16_0
		}
		_bswap_r32(REG_ECX);											// bswap    ecx
		_pextrw_r32_r128(REG_EBX, REG_XMM1, (tmu == 0) ? 2 : 0);		// pextrw   ebx,xmm1,2 or 0
		_pextrw_r32_r128(REG_EAX, REG_XMM1, (tmu == 0) ? 3 : 1);		// pextrw   eax,xmm1,3 or 1
		_shr_r32_cl(REG_EBX);											// shr      ebx,cl
		_shr_r32_cl(REG_EAX);											// shr      eax,cl
		_bswap_r32(REG_ECX);											// bswap    ecx
		_shl_r32_cl(REG_EBX);											// shl      ebx,cl
		_add_r32_r32(REG_EBX, REG_EAX);									// add      ebx,eax

		/* fetch raw texel data */
		if (!TEXTUREMODE_BITS(tmu,11,1))
			_movzx_r32_m8bisd(REG_EAX, REG_EDX, REG_EBX, 1, 0);			// movzx    eax,[edx+ebx+0]
		else
			_movzx_r32_m16bisd(REG_EAX, REG_EDX, REG_EBX, 2, 0);		// movzx    eax,[edx+ebx*2+0]

		/* convert to ARGB */
		_movd_r128_m32isd(REG_XMM2, REG_EAX, 4, lookup);				// movd     xmm2,[eax*4+lookup]
		_pshuflw_r128_r128(REG_XMM5, REG_XMM4, 0xff);					// pshuflw  xmm5,xmm4,0xff
		_punpcklbw_r128_m128abs(REG_XMM2, &ard->i16_0[0]);				// punpcklbw xmm2,ard->i16_0
		_movaps_r128_m128abs(REG_XMM4, &ard->tempsave1[0]);				// movaps   xmm4,ard->tempsave1
		_pmullw_r128_r128(REG_XMM2, REG_XMM5);							// pmullw   xmm2,xmm5
		_paddw_r128_r128(REG_XMM2, REG_XMM0);							// paddw    xmm2,xmm0
		_movaps_r128_m128abs(REG_XMM5, &ard->tempsave2[0]);				// movaps   xmm5,ard->tempsave2
		_psrlw_r128_imm(REG_XMM2, 8);									// psrlw    xmm2,8
	}

	/* zero/other selection */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (!TEXTUREMODE_BITS(tmu,12,1))				/* tc_zero_other */
		_movdqa_r128_r128(REG_XMM1, REG_XMM3);							// movdqa   xmm1,xmm3
	else
		_pxor_r128_r128(REG_XMM1, REG_XMM1);							// pxor     xmm1,xmm1

	/* subtract local color */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,13,1))				/* tc_sub_clocal */
		_psubw_r128_r128(REG_XMM1, REG_XMM2);							// psubw    xmm1,xmm2

	/* scale RGB */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,14,3) != 0)			/* tc_mselect mux */
	{
		switch (TEXTUREMODE_BITS(tmu,14,3))
		{
			case 1:		/* tc_mselect mux == c_local */
				_movdqa_r128_r128(REG_XMM0, REG_XMM2);					// movdqa   xmm0,xmm2
				break;

			case 2:		/* tc_mselect mux == a_other */
				_pshuflw_r128_r128(REG_XMM0, REG_XMM3, 0xff);			// pshuflw  xmm0,xmm3,0xff
				break;

			case 3:		/* tc_mselect mux == a_local */
				_pshuflw_r128_r128(REG_XMM0, REG_XMM2, 0xff);			// pshuflw  xmm0,xmm2,0xff
				break;

			case 4:		/* tc_mselect mux == LOD */
				_mov_r32_m32abs(REG_EAX, &_lod);						// mov      eax,_lod
				_shr_r32_imm(REG_EAX, 3);								// shr      eax,3
				_movd_r128_r32(REG_XMM0, REG_EAX);						// movd     xmm0,eax
				_pshuflw_r128_r128(REG_XMM0, REG_XMM0, 0x00);			// pshuflw  xmm0,xmm0,0x00
				break;

			case 5:		/* tc_mselect mux == LOD frac */
				_mov_r32_m32abs(REG_EAX, &_lod);						// mov      eax,_lod
				_and_r32_imm(REG_EAX, 0xff);							// and      eax,0xff
				_movd_r128_r32(REG_XMM0, REG_EAX);						// movd     xmm0,eax
				_pshuflw_r128_r128(REG_XMM0, REG_XMM0, 0x00);			// pshuflw  xmm0,xmm0,0x00
				break;

			default:
				_pxor_r128_r128(REG_XMM0, REG_XMM0);
				break;
		}

		if (!TEXTUREMODE_BITS(tmu,17,1))			/* tc_reverse_blend */
			_pxor_r128_m128abs(REG_XMM0, &ard->i16_0xff[0]);			// pxor     xmm0,0x00ff x 4

		_paddw_r128_m128abs(REG_XMM0, &ard->i16_1[0]);					// paddw    xmm0,0x0001 x 4
		_pmullw_r128_r128(REG_XMM1, REG_XMM0);							// pmullw   xmm1,xmm0
		_psrlw_r128_imm(REG_XMM1, 8);									// psrlw    xmm1,8
	}

	/* add local color */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,18,1))				/* tc_add_clocal */
		_paddw_r128_r128(REG_XMM1, REG_XMM2);							// paddw    xmm1,xmm3

	/* add local alpha */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	else if (TEXTUREMODE_BITS(tmu,19,1))			/* tc_add_alocal */
	{
		_pshuflw_r128_r128(REG_XMM0, REG_XMM2, 0xff);					// pshuflw  xmm0,xmm2,0xff
		_paddw_r128_r128(REG_XMM1, REG_XMM0);							// paddw    xmm1,xmm0
	}

	/* zero/other selection */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (!TEXTUREMODE_BITS(tmu,21,1))				/* tca_zero_other */
		_pextrw_r32_r128(REG_EDX, REG_XMM3, 3);							// pextrw   edx,xmm3,3
	else
		_mov_r32_imm(REG_EDX, 0);										// mov      edx,0

	/* subtract local alpha */
	/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,22,1))				/* tca_sub_clocal */
	{
		_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);							// pextrw   ebx,xmm2,3
		_sub_r32_r32(REG_EDX, REG_EBX);									// sub      edx,ebx
	}

	/* scale alpha */
	/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,23,3) != 0)			/* tca_mselect mux */
	{
		switch (TEXTUREMODE_BITS(tmu,23,3))
		{
			case 1:		/* tca_mselect mux == c_local */
			case 3:		/* tca_mselect mux == a_local */
				_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);					// pextrw   ebx,xmm2,3
				break;

			case 2:		/* tca_mselect mux == a_other */
				_pextrw_r32_r128(REG_EBX, REG_XMM3, 3);					// pextrw   ebx,xmm3,3
				break;

			case 4:		/* tca_mselect mux == LOD */
				_mov_r32_m32abs(REG_EBX, &_lod);						// mov      ebx,_lod
				_shr_r32_imm(REG_EBX, 3);								// shr      ebx,3
				break;

			case 5:		/* tca_mselect mux == LOD frac */
				_movzx_r32_m8abs(REG_EBX, &_lod);						// movzx    ebx,byte ptr _lod
				break;

			default:
				_mov_r32_imm(REG_EBX, 0);								// mov      ebx,0
				break;
		}

		if (!TEXTUREMODE_BITS(tmu,26,1))			/* tca_reverse_blend */
			_xor_r32_imm(REG_EBX, 0xff);								// xor      ebx,0xff
		_add_r32_imm(REG_EBX, 1);										// add      ebx,1
		_imul_r32_r32(REG_EDX, REG_EBX);								// imul     edx,ebx
		_shr_r32_imm(REG_EDX, 8);										// shr      edx,8
	}

	/* add local color */
	/* how do you add c_local to the alpha???? */
	/* CalSpeed does this in its FMV */
	/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,27,1))				/* tca_add_clocal */
	{
		_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);							// pextrw   ebx,xmm2,3
		_add_r32_r32(REG_EDX, REG_EBX);									// add      edx,ebx
	}
	if (TEXTUREMODE_BITS(tmu,28,1))				/* tca_add_alocal */
	{
		_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);							// pextrw   ebx,xmm2,3
		_add_r32_r32(REG_EDX, REG_EBX);									// add      edx,ebx
	}

	/* clamp */
	/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	_pinsrw_r128_r32(REG_XMM1, REG_EDX, 3);								// pinsrw   xmm1,edx,3
	_packuswb_r128_r128(REG_XMM1, REG_XMM1);							// packuswb xmm1,xmm1

	/* invert */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	if (TEXTUREMODE_BITS(tmu,20,1) && TEXTUREMODE_BITS(tmu,29,1))
		_pxor_r128_m128abs(REG_XMM1, &ard->invert_all);					// pxor     xmm1,ard->invert_all
	else if (TEXTUREMODE_BITS(tmu,20,1))
		_pxor_r128_m128abs(REG_XMM1, &ard->invert_colors);				// pxor     xmm1,ard->invert_colors
	else if (TEXTUREMODE_BITS(tmu,29,1))
		_pxor_r128_m128abs(REG_XMM1, &ard->invert_alpha);				// pxor     xmm1,ard->invert_alpha

	/* unpack to XMM3 */
	/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = argb, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
	_movdqa_r128_r128(REG_XMM3, REG_XMM1);								// movdqa   xmm3,xmm1
	_pxor_r128_r128(REG_XMM0, REG_XMM0);								// pxor     xmm0,xmm0
	_punpcklbw_r128_r128(REG_XMM3, REG_XMM0);							// punpcklbw xmm3,xmm0
}


struct rasterizer_info *generate_rasterizer(void)
{
	drc_core *drc = voodoo_drc;
	void *ylooptop, *xlooptop;
	link_info nodraw_link1 = {0}, nodraw_link2 = {0}, nodraw_link3 = {0}, nodraw_link4 = {0}, nodraw_link5 = {0}, nodraw_link6 = {0};
	link_info yloopend_link1 = {0}, yloopend_link2 = {0}, yloopend_link3 = {0};
	link_info funcend_link1 = {0};
	link_info link1, link2, link3;
	void *lookup0 = NULL, *lookup1 = NULL;
	int do_tmu0 = needs_tmu0();
	int do_tmu1 = do_tmu0 && needs_tmu1();
	int needs_w_depth = 0;
	int needs_w_fog = 0;
	int needs_z_depth = 0;
	int needs_z_float_depth = 0;
	void *start = drc->cache_top;

	/* determine if we need W at all */
	if (FBZMODE_BITS(10,1) || FBZMODE_BITS(4,1))
	{
		if (!FBZMODE_BITS(3,1))
			needs_z_depth = 1;
		else if (!FBZMODE_BITS(21,1))
			needs_w_depth = 1;
		else
			needs_z_float_depth = 1;
	}
	if (FOGMODE_BITS(0,1) && !FOGMODE_BITS(5,1) && !FOGMODE_BITS(4,1) && !FOGMODE_BITS(3,1))
		needs_w_fog = 1;
/*{
 void *dest = drc->cache_top;
 OP1(0xcc);
 _mov_m8abs_imm(dest, 0x90);
}*/

	/* prologue */
	_push_r32(REG_EBX);													// push     ebx
	_push_r32(REG_ESI);													// push     esi
	_push_r32(REG_EDI);													// push     edi
	_push_r32(REG_EBP);													// push     ebp

	/* sort the verticies */
	/* no assumptions */
	_mov_r32_imm(REG_EAX, &tri_va);										// mov      eax,&tri_va
	_mov_r32_imm(REG_EBX, &tri_vb);										// mov      ebx,&tri_vb
	_mov_r32_imm(REG_ECX, &tri_vc);										// mov      ecx,&tri_vc
	_movss_r128_m32abs(REG_XMM0, &tri_vb.y);							// movss    xmm0,tri_vb.y
	_comiss_r128_m32abs(REG_XMM0, &tri_va.y);							// comiss   xmm0,tri_va.y
	_cmov_r32_r32(COND_B, REG_EDX, REG_EAX);							// cmovb    edx,eax
	_cmov_r32_r32(COND_B, REG_EAX, REG_EBX);							// cmovb    eax,ebx
	_cmov_r32_r32(COND_B, REG_EBX, REG_EDX);							// cmovb    ebx,edx
	_movss_r128_m32abs(REG_XMM0, &tri_vc.y);							// movss    xmm0,tri_vc.y
	_comiss_r128_m32bd(REG_XMM0, REG_EAX, offsetof(struct tri_vertex, y));// comiss xmm0,eax->y
	_cmov_r32_r32(COND_B, REG_EDX, REG_EAX);							// cmovb    edx,eax
	_cmov_r32_r32(COND_B, REG_EAX, REG_ECX);							// cmovb    eax,ecx
	_cmov_r32_r32(COND_B, REG_ECX, REG_EDX);							// cmovb    ecx,edx
	_movss_r128_m32bd(REG_XMM0, REG_ECX, offsetof(struct tri_vertex, y));// movss   xmm0,ecx->y
	_comiss_r128_m32bd(REG_XMM0, REG_EBX, offsetof(struct tri_vertex, y));// comiss xmm0,ebx->y
	_cmov_r32_r32(COND_B, REG_EDX, REG_EBX);							// cmovb    edx,ebx
	_cmov_r32_r32(COND_B, REG_EBX, REG_ECX);							// cmovb    ebx,ecx
	_cmov_r32_r32(COND_B, REG_ECX, REG_EDX);							// cmovb    ecx,edx

	/* compute the clipped start/end y */
	/* EAX = vmin, EBX = vmid, ECX = vmax */
	_movss_r128_m32bd(REG_XMM0, REG_EAX, offsetof(struct tri_vertex, y));// movss   xmm0,eax->y
	_movss_r128_m32bd(REG_XMM1, REG_ECX, offsetof(struct tri_vertex, y));// movss   xmm1,ecx->y
	_addss_r128_m32abs(REG_XMM0, &ard->f32_0p5[0]);						// addss    xmm0,0.5
	_addss_r128_m32abs(REG_XMM1, &ard->f32_0p5[0]);						// addss    xmm1,0.5
	_cvttss2si_r32_r128(REG_ESI, REG_XMM0);								// cvttss2si esi,xmm0
	_cvttss2si_r32_r128(REG_EDI, REG_XMM1);								// cvttss2si edi,xmm1
	_cmp_r32_m32abs(REG_ESI, &fbz_cliprect->min_y);						// cmp      esi,fbz_cliprect->min_y
	_cmov_r32_m32abs(COND_L, REG_ESI, &fbz_cliprect->min_y);			// cmovl    esi,fbz_cliprect->min_y
	_cmp_r32_m32abs(REG_EDI, &fbz_cliprect->max_y);						// cmp      edi,fbz_cliprect->max_y
	_cmov_r32_m32abs(COND_G, REG_EDI, &fbz_cliprect->max_y);			// cmovg    edi,fbz_cliprect->max_y
	_cmp_r32_r32(REG_ESI, REG_EDI);										// cmp      esi,edi
	_jcc_near_link(COND_GE, &funcend_link1);							// jge      funcend

	/* compute the slopes */
	/* EAX = vmin, EBX = vmid, ECX = vmax, ESI = starty, EDI = stopy */
	_movss_r128_m32bd(REG_XMM2, REG_ECX, offsetof(struct tri_vertex, y));// movss   xmm2,ecx->y
	_movss_r128_m32bd(REG_XMM1, REG_EBX, offsetof(struct tri_vertex, y));// movss   xmm1,ebx->y
	_movss_r128_m32bd(REG_XMM0, REG_EAX, offsetof(struct tri_vertex, y));// movss   xmm0,eax->y
	_xorps_r128_r128(REG_XMM4, REG_XMM4);								// xorps    xmm4,xmm4
	_movss_r128_r128(REG_XMM3, REG_XMM2);								// movss    xmm3,xmm2
	_subss_r128_r128(REG_XMM2, REG_XMM1);								// subss    xmm2,xmm1 = max->y - mid->y
	_subss_r128_r128(REG_XMM1, REG_XMM0);								// subss    xmm1,xmm0 = mid->y - min->y
	_subss_r128_r128(REG_XMM3, REG_XMM0);								// subss    xmm3,xmm0 = max->y - min->y
	_comiss_r128_r128(REG_XMM2, REG_XMM4);								// comiss   xmm2,xmm4
	_jcc_short_link(COND_NE, &link1);									// jne      skip
	_movaps_r128_m128abs(REG_XMM2, &ard->f32_1[0]);						// movaps   xmm2,1.0
	_resolve_link(&link1);
	_comiss_r128_r128(REG_XMM1, REG_XMM4);								// comiss   xmm1,xmm4
	_jcc_short_link(COND_NE, &link1);									// jne      skip
	_movaps_r128_m128abs(REG_XMM1, &ard->f32_1[0]);						// movaps   xmm1,1.0
	_resolve_link(&link1);
	_comiss_r128_r128(REG_XMM3, REG_XMM4);								// comiss   xmm3,xmm4
	_jcc_short_link(COND_NE, &link1);									// jne      skip
	_movaps_r128_m128abs(REG_XMM3, &ard->f32_1[0]);						// movaps   xmm3,1.0
	_resolve_link(&link1);
	_movss_r128_m32bd(REG_XMM6, REG_ECX, offsetof(struct tri_vertex, x));// movss   xmm6,ecx->x
	_movss_r128_m32bd(REG_XMM5, REG_EBX, offsetof(struct tri_vertex, x));// movss   xmm5,ebx->x
	_movss_r128_m32bd(REG_XMM4, REG_EAX, offsetof(struct tri_vertex, x));// movss   xmm4,eax->x
	_movss_r128_r128(REG_XMM7, REG_XMM6);								// movss    xmm7,xmm6
	_subss_r128_r128(REG_XMM6, REG_XMM5);								// subss    xmm6,xmm5 = max->x - mid->x
	_subss_r128_r128(REG_XMM5, REG_XMM4);								// subss    xmm5,xmm4 = mid->x - min->x
	_subss_r128_r128(REG_XMM7, REG_XMM4);								// subss    xmm7,xmm4 = max->x - min->x
	_divss_r128_r128(REG_XMM6, REG_XMM2);								// divss    xmm6,xmm2
	_divss_r128_r128(REG_XMM5, REG_XMM1);								// divss    xmm5,xmm1
	_divss_r128_r128(REG_XMM7, REG_XMM3);								// divss    xmm7,xmm3
	_movss_m32abs_r128(&_dxdy_midmax, REG_XMM6);						// movss    _dxdy_midmax,xmm6
	_movss_m32abs_r128(&_dxdy_minmid, REG_XMM5);						// movss    _dxdy_minmid,xmm5
	_movss_m32abs_r128(&_dxdy_minmax, REG_XMM7);						// movss    _dxdy_minmax,xmm7

	_mov_m32abs_r32(&_vmin, REG_EAX);									// mov      _vmin,eax
	_mov_m32abs_r32(&_vmid, REG_EBX);									// mov      _vmid,ebx
	_mov_m32abs_r32(&_curry, REG_ESI);									// mov      _curry,esi
	_mov_m32abs_r32(&_stopy, REG_EDI);									// mov      _stopy,edi

	/* setup texture */
	if (do_tmu0)
	{
		int clamp_both, wrap_both;
		int texmode;

		/* determine the lookup */
		texmode = TEXTUREMODE0_BITS(8,4);
		if ((texmode & 7) == 1 && TEXTUREMODE0_BITS(5,1))
			texmode += 6;

		/* handle dirty tables */
		_test_m8abs_imm(&texel_lookup_dirty[0][texmode], 0xff);			// test     texel_lookup_dirty[0][texmode],0xff
		_jcc_short_link(COND_Z, &link1);								// jz       skip
		_push_imm(0);													// push     0
		_call(update_texel_lookup[texmode]);							// call     update_texel_lookup[texmode]
		_add_r32_imm(REG_ESP, 4);										// add      esp,4
		_mov_m8abs_imm(&texel_lookup_dirty[0][texmode], 0);				// mov      texel_lookup_dirty[0][texmode],0
		_resolve_link(&link1);											// skip:

		lookup0 = &texel_lookup[0][texmode][0];

		/* same for TMU1 */
		if (do_tmu1)
		{
			_test_m8abs_imm(&texel_lookup_dirty[1][texmode], 0xff);		// test     texel_lookup_dirty[1][texmode],0xff
			_jcc_short_link(COND_Z, &link1);							// jz       skip
			_push_imm(1);												// push     1
			_call(update_texel_lookup[texmode]);						// call     update_texel_lookup[texmode]
			_add_r32_imm(REG_ESP, 4);									// add      esp,4
			_mov_m8abs_imm(&texel_lookup_dirty[1][texmode], 0);			// mov      texel_lookup_dirty[1][texmode],0
			_resolve_link(&link1);										// skip:

			lookup1 = &texel_lookup[1][texmode][0];
		}

		/* initialize texmask and texmax for TMU0 */
		_mov_r32_m32abs(REG_EAX, &trex_width[0]);						// mov      eax,trex_width[0]
		_mov_r32_m32abs(REG_EBX, &trex_height[0]);						// mov      ebx,trex_height[0]
		_sub_r32_imm(REG_EAX, 1);										// sub      eax,1
		_sub_r32_imm(REG_EBX, 1);										// sub      ebx,1

		clamp_both = (TEXTUREMODE0_BITS(6,1) && TEXTUREMODE0_BITS(7,1));
		wrap_both = (!TEXTUREMODE0_BITS(6,1) && !TEXTUREMODE0_BITS(7,1));
		if (!clamp_both)
		{
			if (TEXTUREMODE0_BITS(6,1))
				_mov_m32abs_imm(&ard->texmask[3], ~0);					// mov      ard->texmask[3],~0
			else
				_mov_m32abs_r32(&ard->texmask[3], REG_EAX);				// mov      ard->texmask[3],eax

			if (TEXTUREMODE0_BITS(7,1))
				_mov_m32abs_imm(&ard->texmask[2], ~0);					// mov      ard->texmask[2],~0
			else
				_mov_m32abs_r32(&ard->texmask[2], REG_EBX);				// mov      ard->texmask[2],ebx
		}
		if (!wrap_both)
		{
			_mov_m16abs_r16(&ard->texmax[3], REG_AX);					// mov      ard->texmax[3],ax
			_mov_m16abs_r16(&ard->texmax[2], REG_BX);					// mov      ard->texmax[2],bx
		}

		/* initialize texmask and texmax for TMU1 */
		if (do_tmu1)
		{
			_mov_r32_m32abs(REG_EAX, &trex_width[1]);					// mov      eax,trex_width[1]
			_mov_r32_m32abs(REG_EBX, &trex_height[1]);					// mov      ebx,trex_height[1]
			_sub_r32_imm(REG_EAX, 1);									// sub      eax,1
			_sub_r32_imm(REG_EBX, 1);									// sub      ebx,1

			clamp_both = (TEXTUREMODE1_BITS(6,1) && TEXTUREMODE1_BITS(7,1));
			wrap_both = (!TEXTUREMODE1_BITS(6,1) && !TEXTUREMODE1_BITS(7,1));
			if (!clamp_both)
			{
				if (TEXTUREMODE1_BITS(6,1))
					_mov_m32abs_imm(&ard->texmask[1], ~0);				// mov      ard->texmask[1],~0
				else
					_mov_m32abs_r32(&ard->texmask[1], REG_EAX);			// mov      ard->texmask[1],eax

				if (TEXTUREMODE1_BITS(7,1))
					_mov_m32abs_imm(&ard->texmask[0], ~0);				// mov      ard->texmask[0],~0
				else
					_mov_m32abs_r32(&ard->texmask[0], REG_EBX);			// mov      ard->texmask[0],ebx
			}
			if (!wrap_both)
			{
				_mov_m16abs_r16(&ard->texmax[1], REG_AX);				// mov      ard->texmax[1],ax
				_mov_m16abs_r16(&ard->texmax[0], REG_BX);				// mov      ard->texmax[0],bx
			}
		}

		/* compute lodbase values */
		if (PER_PIXEL_LOD)
		{
			_cvtsi2ss_r128_m32abs(REG_XMM0, &trex_width[0]);			// cvtsi2ss xmm0,trex_width[0]
			_cvtsi2ss_r128_m32abs(REG_XMM1, &trex_height[0]);			// cvtsi2ss xmm1,trex_height[0]
			if (do_tmu1)
			{
				_cvtsi2ss_r128_m32abs(REG_XMM4, &trex_width[1]);		// cvtsi2ss xmm4,trex_width[1]
				_cvtsi2ss_r128_m32abs(REG_XMM5, &trex_height[1]);		// cvtsi2ss xmm5,trex_height[1]
			}
			_mulss_r128_m32abs(REG_XMM0, &_1_over_256);					// mulss    xmm0,(1.0 / 256.0)
			_mulss_r128_m32abs(REG_XMM1, &_1_over_256);					// mulss    xmm1,(1.0 / 256.0)
			if (do_tmu1)
			{
				_mulss_r128_m32abs(REG_XMM4, &_1_over_256);				// mulss    xmm4,(1.0 / 256.0)
				_mulss_r128_m32abs(REG_XMM5, &_1_over_256);				// mulss    xmm5,(1.0 / 256.0)
			}
			_movss_r128_m32abs(REG_XMM2, &tri_ds0dx);					// movss    xmm2,tri_ds0dx
			_movss_r128_m32abs(REG_XMM3, &tri_dt0dx);					// movss    xmm3,tri_dt0dx
			if (do_tmu1)
			{
				_movss_r128_m32abs(REG_XMM6, &tri_ds1dx);				// movss    xmm6,tri_ds1dx
				_movss_r128_m32abs(REG_XMM7, &tri_dt1dx);				// movss    xmm7,tri_dt1dx
			}
			_mulss_r128_r128(REG_XMM2, REG_XMM0);						// mulss    xmm2,xmm0
			_mulss_r128_r128(REG_XMM3, REG_XMM1);						// mulss    xmm3,xmm1
			if (do_tmu1)
			{
				_mulss_r128_r128(REG_XMM6, REG_XMM4);					// mulss    xmm6,xmm4
				_mulss_r128_r128(REG_XMM7, REG_XMM5);					// mulss    xmm7,xmm5
			}
			_mulss_r128_m32abs(REG_XMM0, &tri_ds0dy);					// mulss    xmm0,tri_ds0dy
			_mulss_r128_m32abs(REG_XMM1, &tri_dt0dy);					// mulss    xmm1,tri_dt0dy
			if (do_tmu1)
			{
				_mulss_r128_m32abs(REG_XMM4, &tri_ds1dy);				// mulss    xmm0,tri_ds1dy
				_mulss_r128_m32abs(REG_XMM5, &tri_dt1dy);				// mulss    xmm1,tri_dt1dy
			}
			_mulss_r128_r128(REG_XMM2, REG_XMM2);						// mulss    xmm2,xmm2
			_mulss_r128_r128(REG_XMM3, REG_XMM3);						// mulss    xmm3,xmm3
			if (do_tmu1)
			{
				_mulss_r128_r128(REG_XMM6, REG_XMM6);					// mulss    xmm6,xmm6
				_mulss_r128_r128(REG_XMM7, REG_XMM7);					// mulss    xmm7,xmm7
			}
			_mulss_r128_r128(REG_XMM0, REG_XMM0);						// mulss    xmm0,xmm0
			_mulss_r128_r128(REG_XMM1, REG_XMM1);						// mulss    xmm1,xmm1
			if (do_tmu1)
			{
				_mulss_r128_r128(REG_XMM4, REG_XMM4);					// mulss    xmm4,xmm4
				_mulss_r128_r128(REG_XMM5, REG_XMM5);					// mulss    xmm5,xmm5
			}
			_addss_r128_r128(REG_XMM2, REG_XMM3);						// addss    xmm2,xmm3
			_addss_r128_r128(REG_XMM0, REG_XMM1);						// addss    xmm0,xmm1
			if (do_tmu1)
			{
				_addss_r128_r128(REG_XMM6, REG_XMM7);					// addss    xmm6,xmm7
				_addss_r128_r128(REG_XMM4, REG_XMM5);					// addss    xmm4,xmm5
			}
			_maxss_r128_r128(REG_XMM0, REG_XMM2);						// maxss    xmm0,xmm2
			_movss_m32abs_r128(&ard->lodbase[2], REG_XMM0);				// movss    ard->lodbase[2],xmm0
			if (do_tmu1)
			{
				_maxss_r128_r128(REG_XMM4, REG_XMM6);					// maxss    xmm4,xmm6
				_movss_m32abs_r128(&ard->lodbase[0], REG_XMM4);			// movss    ard->lodbase[0],xmm4
			}
		}
	}

	/* prepare the remaining structures before the Y loop */
	_movd_r128_m32abs(REG_XMM0, &tri_starta);							// movd     xmm0,tri_starta
	_movd_r128_m32abs(REG_XMM1, &tri_startr);							// movd     xmm1,tri_startr
	_movd_r128_m32abs(REG_XMM2, &tri_startg);							// movd     xmm2,tri_startg
	_movd_r128_m32abs(REG_XMM3, &tri_startb);							// movd     xmm3,tri_startb
	_movd_r128_m32abs(REG_XMM4, &tri_dadx);								// movd     xmm4,tri_dadx
	_movd_r128_m32abs(REG_XMM5, &tri_drdx);								// movd     xmm5,tri_drdx
	_movd_r128_m32abs(REG_XMM6, &tri_dgdx);								// movd     xmm6,tri_dgdx
	_movd_r128_m32abs(REG_XMM7, &tri_dbdx);								// movd     xmm7,tri_dbdx
	_punpckldq_r128_r128(REG_XMM1, REG_XMM0);							// punpckldq xmm1,xmm0
	_punpckldq_r128_r128(REG_XMM5, REG_XMM4);							// punpckldq xmm5,xmm4
	_punpckldq_r128_r128(REG_XMM3, REG_XMM2);							// punpckldq xmm3,xmm2
	_punpckldq_r128_r128(REG_XMM7, REG_XMM6);							// punpckldq xmm7,xmm6
	_punpcklqdq_r128_r128(REG_XMM3, REG_XMM1);							// punpcklqdq xmm3,xmm1
	_punpcklqdq_r128_r128(REG_XMM7, REG_XMM5);							// punpcklqdq xmm7,xmm5
	_cvtdq2ps_r128_r128(REG_XMM3, REG_XMM3);							// cvtdq2ps xmm3,xmm3
	_movdqa_m128abs_r128(&ard->dargb_idx[0], REG_XMM7);					// movdqa   ard->dargb_idx,xmm7
	_cvtdq2ps_r128_r128(REG_XMM7, REG_XMM7);							// cvtdq2ps xmm7,xmm7
	_movaps_m128abs_r128(&ard->start_argb[0], REG_XMM3);				// movaps   ard->start_argb,xmm3
	_movaps_m128abs_r128(&ard->dargb_dx[0], REG_XMM7);					// movaps   ard->dargb_dx,xmm7

	_movd_r128_m32abs(REG_XMM0, &tri_dady);								// movd     xmm0,tri_dady
	_movd_r128_m32abs(REG_XMM1, &tri_drdy);								// movd     xmm1,tri_drdy
	_movd_r128_m32abs(REG_XMM2, &tri_dgdy);								// movd     xmm2,tri_dgdy
	_movd_r128_m32abs(REG_XMM3, &tri_dbdy);								// movd     xmm3,tri_dbdy
	if (do_tmu0)
	{
		_movd_r128_m32abs(REG_XMM4, &tri_starts0);						// movd     xmm4,tri_starts0
		_movd_r128_m32abs(REG_XMM5, &tri_startt0);						// movd     xmm5,tri_startt0
		_movd_r128_m32abs(REG_XMM6, &tri_starts1);						// movd     xmm6,tri_starts1
		_movd_r128_m32abs(REG_XMM7, &tri_startt1);						// movd     xmm7,tri_startt1
	}
	_punpckldq_r128_r128(REG_XMM1, REG_XMM0);							// punpckldq xmm1,xmm0
	if (do_tmu0)
		_punpckldq_r128_r128(REG_XMM5, REG_XMM4);						// punpckldq xmm5,xmm4
	_punpckldq_r128_r128(REG_XMM3, REG_XMM2);							// punpckldq xmm3,xmm2
	if (do_tmu0)
		_punpckldq_r128_r128(REG_XMM7, REG_XMM6);						// punpckldq xmm7,xmm6
	_punpcklqdq_r128_r128(REG_XMM3, REG_XMM1);							// punpcklqdq xmm3,xmm1
	if (do_tmu0)
		_punpcklqdq_r128_r128(REG_XMM7, REG_XMM5);						// punpcklqdq xmm7,xmm5
	_cvtdq2ps_r128_r128(REG_XMM3, REG_XMM3);							// cvtdq2ps xmm3,xmm3
	if (do_tmu0)
		_mulps_r128_m128abs(REG_XMM7, &ard->f32_256[0]);				// mulps    xmm7,256.0
	_movaps_m128abs_r128(&ard->dargb_dy[0], REG_XMM3);					// movaps   ard->dargb_dy,xmm3
	if (do_tmu0)
		_movaps_m128abs_r128(&ard->start_st01[0], REG_XMM7);			// movaps   ard->start_st01,xmm7

	if (do_tmu0)
	{
		_movd_r128_m32abs(REG_XMM0, &tri_ds0dx);						// movd     xmm0,tri_ds0dx
		_movd_r128_m32abs(REG_XMM1, &tri_dt0dx);						// movd     xmm1,tri_dt0dx
		_movd_r128_m32abs(REG_XMM2, &tri_ds1dx);						// movd     xmm2,tri_ds1dx
		_movd_r128_m32abs(REG_XMM3, &tri_dt1dx);						// movd     xmm3,tri_dt1dx
		_movd_r128_m32abs(REG_XMM4, &tri_ds0dy);						// movd     xmm4,tri_ds0dy
		_movd_r128_m32abs(REG_XMM5, &tri_dt0dy);						// movd     xmm5,tri_dt0dy
		_movd_r128_m32abs(REG_XMM6, &tri_ds1dy);						// movd     xmm6,tri_ds1dy
		_movd_r128_m32abs(REG_XMM7, &tri_dt1dy);						// movd     xmm7,tri_dt1dy
		_punpckldq_r128_r128(REG_XMM1, REG_XMM0);						// punpckldq xmm1,xmm0
		_punpckldq_r128_r128(REG_XMM5, REG_XMM4);						// punpckldq xmm5,xmm4
		_punpckldq_r128_r128(REG_XMM3, REG_XMM2);						// punpckldq xmm3,xmm2
		_punpckldq_r128_r128(REG_XMM7, REG_XMM6);						// punpckldq xmm7,xmm6
		_punpcklqdq_r128_r128(REG_XMM3, REG_XMM1);						// punpcklqdq xmm3,xmm1
		_punpcklqdq_r128_r128(REG_XMM7, REG_XMM5);						// punpcklqdq xmm7,xmm5
		_mulps_r128_m128abs(REG_XMM3, &ard->f32_256[0]);				// mulps    xmm3,256.0
		_mulps_r128_m128abs(REG_XMM7, &ard->f32_256[0]);				// mulps    xmm7,256.0
		_movaps_m128abs_r128(&ard->dst01_dx[0], REG_XMM3);				// movaps   ard->dst01_dx,xmm3
		_movaps_m128abs_r128(&ard->dst01_dy[0], REG_XMM7);				// movaps   ard->dst01_dy,xmm7

		_movd_r128_m32abs(REG_XMM0, &tri_startw0);						// movd     xmm0,tri_startw0
		_movd_r128_m32abs(REG_XMM1, &tri_startw1);						// movd     xmm1,tri_startw1
		_movd_r128_m32abs(REG_XMM2, &tri_dw0dx);						// movd     xmm2,tri_dw0dx
		_movd_r128_m32abs(REG_XMM3, &tri_dw1dx);						// movd     xmm3,tri_dw1dx
		_movd_r128_m32abs(REG_XMM4, &tri_dw0dy);						// movd     xmm4,tri_dw0dy
		_movd_r128_m32abs(REG_XMM5, &tri_dw1dy);						// movd     xmm5,tri_dw1dy
		_punpckldq_r128_r128(REG_XMM0, REG_XMM0);						// punpckldq xmm0,xmm0
		_punpckldq_r128_r128(REG_XMM1, REG_XMM1);						// punpckldq xmm1,xmm1
		_punpckldq_r128_r128(REG_XMM2, REG_XMM2);						// punpckldq xmm2,xmm2
		_punpckldq_r128_r128(REG_XMM3, REG_XMM3);						// punpckldq xmm3,xmm3
		_punpckldq_r128_r128(REG_XMM4, REG_XMM4);						// punpckldq xmm4,xmm4
		_punpckldq_r128_r128(REG_XMM5, REG_XMM5);						// punpckldq xmm5,xmm5
		_punpcklqdq_r128_r128(REG_XMM1, REG_XMM0);						// punpcklqdq xmm1,xmm0
		_punpcklqdq_r128_r128(REG_XMM3, REG_XMM2);						// punpcklqdq xmm3,xmm2
		_punpcklqdq_r128_r128(REG_XMM5, REG_XMM4);						// punpcklqdq xmm5,xmm4
		_movdqa_m128abs_r128(&ard->start_w01[0], REG_XMM1);				// movdqa   ard->start_w01,xmm1
		_movdqa_m128abs_r128(&ard->dw01_dx[0], REG_XMM3);				// movdqa   ard->dw01_dx,xmm3
		_movdqa_m128abs_r128(&ard->dw01_dy[0], REG_XMM5);				// movdqa   ard->dw01_dy,xmm5
	}

	_movss_r128_m32abs(REG_XMM0, &tri_startw);							// movss    xmm0,tri_startw
	_cvtsi2ss_r128_m32abs(REG_XMM1, &tri_startz);						// cvtsi2ss xmm1,tri_startz
	_movss_r128_m32abs(REG_XMM2, &tri_dwdx);							// movss    xmm2,tri_dwdx
	_cvtsi2ss_r128_m32abs(REG_XMM3, &tri_dzdx);							// cvtsi2ss xmm3,tri_dzdx
	_movss_r128_m32abs(REG_XMM4, &tri_dwdy);							// movss    xmm4,tri_dwdy
	_cvtsi2ss_r128_m32abs(REG_XMM5, &tri_dzdy);							// cvtsi2ss xmm5,tri_dzdy
	if (needs_w_depth)
	{
		/* pack with W on the bottom */
		_unpcklps_r128_r128(REG_XMM0, REG_XMM1);						// unpcklps xmm0,xmm1
		_unpcklps_r128_r128(REG_XMM2, REG_XMM3);						// unpcklps xmm2,xmm3
		_unpcklps_r128_r128(REG_XMM4, REG_XMM5);						// unpcklps xmm4,xmm5
		_mulps_r128_m128abs(REG_XMM0, &ard->wz_scale[0]);				// mulps    xmm0,ard->wz_scale
		_mulps_r128_m128abs(REG_XMM2, &ard->wz_scale[0]);				// mulps    xmm2,ard->wz_scale
		_mulps_r128_m128abs(REG_XMM4, &ard->wz_scale[0]);				// mulps    xmm4,ard->wz_scale
		_movaps_m128abs_r128(&ard->start_wz[0], REG_XMM0);				// movaps   ard->start_wz,xmm0
		_movaps_m128abs_r128(&ard->dwz_dx[0], REG_XMM2);				// movaps   ard->dwz_dx,xmm2
		_movaps_m128abs_r128(&ard->dwz_dy[0], REG_XMM4);				// movaps   ard->dwz_dy,xmm4
	}
	else
	{
		/* pack with Z on the bottom */
		_unpcklps_r128_r128(REG_XMM1, REG_XMM0);						// unpcklps xmm1,xmm0
		_unpcklps_r128_r128(REG_XMM3, REG_XMM2);						// unpcklps xmm3,xmm2
		_unpcklps_r128_r128(REG_XMM5, REG_XMM4);						// unpcklps xmm5,xmm4
		if (!needs_z_float_depth)
		{
			_mulps_r128_m128abs(REG_XMM1, &ard->zw_scale[0]);			// mulps    xmm1,ard->zw_scale
			_mulps_r128_m128abs(REG_XMM3, &ard->zw_scale[0]);			// mulps    xmm3,ard->zw_scale
			_mulps_r128_m128abs(REG_XMM5, &ard->zw_scale[0]);			// mulps    xmm5,ard->zw_scale
		}
		else
		{
			_mulps_r128_m128abs(REG_XMM1, &ard->zfw_scale[0]);			// mulps    xmm1,ard->zfw_scale
			_mulps_r128_m128abs(REG_XMM3, &ard->zfw_scale[0]);			// mulps    xmm3,ard->zfw_scale
			_mulps_r128_m128abs(REG_XMM5, &ard->zfw_scale[0]);			// mulps    xmm5,ard->zfw_scale
		}
		_movaps_m128abs_r128(&ard->start_wz[0], REG_XMM1);				// movaps   ard->start_wz,xmm1
		_movaps_m128abs_r128(&ard->dwz_dx[0], REG_XMM3);				// movaps   ard->dwz_dx,xmm3
		_movaps_m128abs_r128(&ard->dwz_dy[0], REG_XMM5);				// movaps   ard->dwz_dy,xmm5
	}

	/* top of the Y loop */
	ylooptop = drc->cache_top;											// yloop:
		_mov_r32_m32abs(REG_EAX, &_curry);									// mov      eax,_curry
		_mov_r32_r32(REG_EBX, REG_EAX);										// mov      ebx,eax
		if (FBZMODE_BITS(17,1))
		{
			_neg_r32(REG_EAX);												// neg      eax
			_add_r32_m32abs(REG_EAX, &inverted_yorigin);					// add      eax,inverted_yorigin
		}
		_cmp_r32_imm(REG_EAX, FRAMEBUF_HEIGHT);								// cmp      eax,FRAMEBUF_HEIGHT
		_jcc_near_link(COND_AE, &yloopend_link1);							// jae      yloopend

		/* if we're allowed to cheat, skip some scanlines */
		/* EAX = effy, EBX = y */
		if (RESOLUTION_DIVIDE_SHIFT != 0)
		{
			_test_m8abs_imm(&cheating_allowed, 0xff);						// test     cheating_allowed,0xff
			_jcc_short_link(COND_Z, &link1);								// jz       skip
			_test_r32_imm(REG_EAX, (1 << RESOLUTION_DIVIDE_SHIFT) - 1);		// test     eax,(1 << RESOLUTION_DIVIDE_SHIFT) - 1
			_jcc_near_link(COND_NZ, &yloopend_link2);						// jnz      yloopend
		}

		/* get pointers to the destination and depth buffers */
		/* EAX = effy, EBX = y */
		_shl_r32_imm(REG_EAX, 10);											// shl      eax,10
		_mov_r32_m32abs(REG_EDI, fbz_draw_buffer);							// mov      edi,fbz_draw_buffer
		_lea_r32_m32isd(REG_ESI, REG_EAX, 2, depthbuf);						// lea      esi,[eax*2+depthbuf]
		_lea_r32_m32bisd(REG_EDI, REG_EDI, REG_EAX, 2, 0);					// lea      edi,[edi+eax*2]

		/* compute X endpoints */
		/* EBX = y, ESI -> depth, EDI -> draw */
		_cvtsi2ss_r128_r32(REG_XMM0, REG_EBX);								// cvtsi2ss xmm0,ebx
		_mov_r32_m32abs(REG_EBX, &_vmid);									// mov      ebx,vmid
		_addss_r128_m32abs(REG_XMM0, &ard->f32_0p5[0]);						// addss    xmm0,0.5
		_mov_r32_m32abs(REG_EAX, &_vmin);									// mov      eax,vmin
		_mov_r32_imm(REG_ECX, &_dxdy_midmax);								// mov      ecx,&dxdy_midmax
		_mov_r32_imm(REG_EDX, &_dxdy_minmid);								// mov      edx,&dxdy_minmid
		_comiss_r128_m32bd(REG_XMM0, REG_EBX, offsetof(struct tri_vertex, y));// comiss xmm0,ebx->y
		_movss_r128_r128(REG_XMM1, REG_XMM0);								// movss    xmm1,xmm0
		_movss_r128_r128(REG_XMM2, REG_XMM0);								// movss    xmm2,xmm0
		_cmov_r32_r32(COND_B, REG_EBX, REG_EAX);							// cmovb    ebx,eax
		_subss_r128_m32bd(REG_XMM1, REG_EAX, offsetof(struct tri_vertex, y));// subss   xmm1,eax->y
		_cmov_r32_r32(COND_B, REG_ECX, REG_EDX);							// cmovb    ecx,edx
		_subss_r128_m32bd(REG_XMM2, REG_EBX, offsetof(struct tri_vertex, y));// subss   xmm2,ebx->y
		_mulss_r128_m32abs(REG_XMM1, &_dxdy_minmax);						// mulss    xmm1,dxdy_minmax
		_mulss_r128_m32bd(REG_XMM2, REG_ECX, 0);							// mulss    xmm2,[ecx]
		_addss_r128_m32bd(REG_XMM1, REG_EAX, offsetof(struct tri_vertex, x));// addss   xmm1,eax->x
		_addss_r128_m32bd(REG_XMM2, REG_EBX, offsetof(struct tri_vertex, x));// addss   xmm2,ebx->x
		_addss_r128_m32abs(REG_XMM1, &ard->f32_0p5[0]);						// addss    xmm1,0.5
		_addss_r128_m32abs(REG_XMM2, &ard->f32_0p5[0]);						// addss    xmm2,0.5
		_cvttss2si_r32_r128(REG_EAX, REG_XMM1);								// cvttss2si eax,xmm1
		_cvttss2si_r32_r128(REG_EBX, REG_XMM2);								// cvttss2si ebx,xmm2

		_cmp_r32_r32(REG_EAX, REG_EBX);										// cmp      eax,ebx
		_cmov_r32_r32(COND_G, REG_ECX, REG_EAX);							// cmovg    ecx,eax
		_cmov_r32_r32(COND_G, REG_EAX, REG_EBX);							// cmovg    eax,ebx
		_cmov_r32_r32(COND_G, REG_EBX, REG_ECX);							// cmovg    ebx,ecx
		_cmp_r32_m32abs(REG_EAX, &fbz_cliprect->min_x);						// cmp      eax,fbz_cliprect->min_x
		_cmov_r32_m32abs(COND_L, REG_EAX, &fbz_cliprect->min_x);			// cmovl    eax,fbz_cliprect->min_x
		_cmp_r32_m32abs(REG_EBX, &fbz_cliprect->max_x);						// cmp      ebx,fbz_cliprect->max_x
		_cmov_r32_m32abs(COND_G, REG_EBX, &fbz_cliprect->max_x);			// cmovg    ebx,fbz_cliprect->max_x
		_cmp_r32_r32(REG_EAX, REG_EBX);										// cmp      eax,ebx
		_mov_r32_r32(REG_EBP, REG_EAX);										// mov      ebp,eax
		_mov_m32abs_r32(&_stopx, REG_EBX);									// mov      _stopx,ebx
		_jcc_near_link(COND_GE, &yloopend_link3);							// jge      yloopend

		/* compute parameters */
		/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = (float)y + 0.5 */
		_cvtsi2ss_r128_r32(REG_XMM1, REG_EAX);								// cvtsi2ss xmm1,eax
		_addss_r128_m32abs(REG_XMM1, &ard->f32_0p5[0]);						// addss    xmm1,0.5
		_subss_r128_m32abs(REG_XMM0, &tri_va.y);							// subss    xmm0,tri_va.y
		_subss_r128_m32abs(REG_XMM1, &tri_va.x);							// subss    xmm1,tri_va.x
		_shufps_r128_r128(REG_XMM0, REG_XMM0, 0);							// shufps   xmm0,xmm0,0
		_shufps_r128_r128(REG_XMM1, REG_XMM1, 0);							// shufps   xmm1,xmm1,0

		/* compute starting a,r,g,b as integers in XMM7 */
		/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = dy, XMM1 = dx, XMM7 = argb */
		_movaps_r128_m128abs(REG_XMM2, &ard->dargb_dx[0]);					// movaps   xmm2,ard->dargb_dx
		_movaps_r128_m128abs(REG_XMM3, &ard->dargb_dy[0]);					// movaps   xmm3,ard->dargb_dy
		_movaps_r128_m128abs(REG_XMM7, &ard->start_argb[0]);				// movaps   xmm7,ard->start_argb
		_mulps_r128_r128(REG_XMM2, REG_XMM1);								// mulps    xmm2,xmm1
		_mulps_r128_r128(REG_XMM3, REG_XMM0);								// mulps    xmm3,xmm0
		_addps_r128_r128(REG_XMM7, REG_XMM2);								// addps    xmm7,xmm2
		_addps_r128_r128(REG_XMM7, REG_XMM3);								// addps    xmm7,xmm3
		_cvttps2dq_r128_r128(REG_XMM7, REG_XMM7);							// cvttps2dq xmm7,xmm7

		if (do_tmu0)
		{
			/* compute starting s0,t0,s1,t1 in XMM6 */
			/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = dy, XMM1 = dx, XMM6 = s0t0s1t1, XMM7 = argb */
			_movaps_r128_m128abs(REG_XMM2, &ard->dst01_dx[0]);				// movaps   xmm2,ard->dst01_dx
			_movaps_r128_m128abs(REG_XMM3, &ard->dst01_dy[0]);				// movaps   xmm3,ard->dst01_dy
			_movaps_r128_m128abs(REG_XMM6, &ard->start_st01[0]);			// movaps   xmm6,ard->start_st01
			_mulps_r128_r128(REG_XMM2, REG_XMM1);							// mulps    xmm2,xmm1
			_mulps_r128_r128(REG_XMM3, REG_XMM0);							// mulps    xmm3,xmm0
			_addps_r128_r128(REG_XMM6, REG_XMM2);							// addps    xmm6,xmm2
			_addps_r128_r128(REG_XMM6, REG_XMM3);							// addps    xmm6,xmm3

			/* compute starting w0,w1 in XMM5 */
			/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = dy, XMM1 = dx, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			_movaps_r128_m128abs(REG_XMM2, &ard->dw01_dx[0]);				// movaps   xmm2,ard->dw01_dx
			_movaps_r128_m128abs(REG_XMM3, &ard->dw01_dy[0]);				// movaps   xmm3,ard->dw01_dy
			_movaps_r128_m128abs(REG_XMM5, &ard->start_w01[0]);				// movaps   xmm5,ard->start_w01
			_mulps_r128_r128(REG_XMM2, REG_XMM1);							// mulps    xmm2,xmm1
			_mulps_r128_r128(REG_XMM3, REG_XMM0);							// mulps    xmm3,xmm0
			_addps_r128_r128(REG_XMM5, REG_XMM2);							// addps    xmm5,xmm2
			_addps_r128_r128(REG_XMM5, REG_XMM3);							// addps    xmm5,xmm3
		}

		/* compute starting w,z in XMM4 */
		/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = dy, XMM1 = dx, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
		_movaps_r128_m128abs(REG_XMM2, &ard->dwz_dx[0]);					// movaps   xmm2,ard->dwz_dx
		_movaps_r128_m128abs(REG_XMM3, &ard->dwz_dy[0]);					// movaps   xmm3,ard->dwz_dy
		_movaps_r128_m128abs(REG_XMM4, &ard->start_wz[0]);					// movaps   xmm4,ard->start_wz
		_mulps_r128_r128(REG_XMM2, REG_XMM1);								// mulps    xmm2,xmm1
		_mulps_r128_r128(REG_XMM3, REG_XMM0);								// mulps    xmm3,xmm0
		_addps_r128_r128(REG_XMM4, REG_XMM2);								// addps    xmm4,xmm2
		_addps_r128_r128(REG_XMM4, REG_XMM3);								// addps    xmm4,xmm3

		/* loop over X */
		/* EAX = startx, EBX = stopx, ESI -> depth, EDI -> draw, EBP = x, XMM0 = dy, XMM1 = dx, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
		_sub_r32_r32(REG_EBX, REG_EAX);										// sub      ebx,eax
		_add_m32abs_r32(&voodoo_regs[fbiPixelsIn], REG_EBX);				// add      fbiPixelsIn,ebx
		_add_m32abs_r32(&voodoo_regs[pixelcount], REG_EBX);					// add      pixelcount,ebx

		/* top of the X loop */
		xlooptop = drc->cache_top;											// xloop:

			if (RESOLUTION_DIVIDE_SHIFT != 0)
			{
				_test_m8abs_imm(&cheating_allowed, 0xff);					// test     cheating_allowed,0xff
				_jcc_short_link(COND_Z, &link1);							// jz       skip
				_test_r32_imm(REG_EBP, (1 << RESOLUTION_DIVIDE_SHIFT) - 1);	// test     ebp,(1 << RESOLUTION_DIVIDE_SHIFT) - 1
				_jcc_near_link(COND_NZ, &nodraw_link1);						// jnz      nodraw
			}

			/* handle stippling */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(2,1))
			{
				/* rotate mode */
				if (!FBZMODE_BITS(12,1))
				{
					_rol_m32abs_imm(&voodoo_regs[stipple], 1);				// rol      stipple,1
					_jcc_near_link(COND_NS, &nodraw_link2);					// jns      nodraw
				}

				/* pattern mode */
				else
				{
					_mov_r32_r32(REG_EAX, REG_EBP);							// mov      eax,ebp
					_mov_r32_m32abs(REG_EBX, &_curry);						// mov      ebx,_curry
					_not_r32(REG_EAX);										// not      eax
					_and_r32_imm(REG_EBX, 3);								// and      ebx,3
					_and_r32_imm(REG_EAX, 7);								// and      eax,7
					_bt_m32bd_r32(REG_EBX, &voodoo_regs[stipple], REG_EAX);	// bt       [stipple+ebx],eax
					_jcc_near_link(COND_NC, &nodraw_link2);					// jnc      nodraw
				}
			}

			/* compute the floating-point W value if we need it */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (needs_w_depth || needs_w_fog)
			{
				if (needs_w_depth)
				{
					_comiss_r128_m32abs(REG_XMM4, &ard->wz_scale[0]);		// comiss   xmm4,1.0
					_mov_r32_imm(REG_EAX, 0);								// mov      eax,0
					_cvttss2si_r32_r128(REG_EBX, REG_XMM4);					// cvttss2si ebx,xmm4
					_jcc_short_link(COND_AE, &link1);						// jae      skip
					_comiss_r128_m32abs(REG_XMM4, &ard->f32_0[0]);			// comiss   xmm4,0.0
				}
				else
				{
					_pshufd_r128_r128(REG_XMM0, REG_XMM4, 0x55);			// pshufd   xmm0,xmm4,0x55
					_comiss_r128_m32abs(REG_XMM0, &ard->wz_scale[1]);		// comiss   xmm0,1.0
					_mov_r32_imm(REG_EAX, 0);								// mov      eax,0
					_cvttss2si_r32_r128(REG_EBX, REG_XMM0);					// cvttss2si ebx,xmm0
					_jcc_short_link(COND_AE, &link1);						// jae      skip
					_comiss_r128_m32abs(REG_XMM0, &ard->f32_0[0]);			// comiss   xmm0,0.0
				}
				_mov_r32_imm(REG_EAX, 0xffff);								// mov      eax,0xffff
				_jcc_short_link(COND_B, &link2);							// jb       skip
				_bsr_r32_r32(REG_ECX, REG_EBX);								// bsr      edx,ebx
				_mov_r32_imm(REG_EAX, 0xfffe);								// mov      eax,0xfffe
				_jcc_short_link(COND_Z, &link3);							// jz       skip
				_xor_r32_imm(REG_ECX, 31);									// xor      ecx,31
				_cmp_r32_imm(REG_ECX, 19);									// cmp      ecx,19
				_mov_r32_imm(REG_EDX, 19);									// mov      edx,19
				_cmov_r32_r32(COND_A, REG_ECX, REG_EDX);					// cmova    ecx,edx
				_shl_r32_cl(REG_EBX);										// shl      ebx,cl
				_lea_r32_m32bd(REG_EAX, REG_ECX, -4);						// lea      eax,[ecx-4]
				_shr_r32_imm(REG_EBX, 19);									// shr      ebx,19
				_shl_r32_imm(REG_EAX, 12);									// shl      eax,12
				_and_r32_imm(REG_EBX, 0xfff);								// and      ebx,0xfff
				_or_r32_r32(REG_EAX, REG_EBX);								// or       eax,ebx
				_xor_r32_imm(REG_EAX, 0xfff);								// xor      eax,0xfff
				_resolve_link(&link1);										// skip:
				_resolve_link(&link2);										// skip:
				_resolve_link(&link3);										// skip:
				if (needs_w_fog)
					_mov_m32abs_r32(&_wfloat, REG_EAX);						// mov      _wfloat,eax
			}

			/* compute depth value (W or Z) for this pixel */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (!FBZMODE_BITS(3,1))
			{
				_cvttss2si_r32_r128(REG_EAX, REG_XMM4);						// cvttss2si eax,xmm4
				_cmp_r32_imm(REG_EAX, 0);									// cmp      eax,0
				_mov_r32_imm(REG_EBX, 0);									// mov      ebx,0
				_cmov_r32_r32(COND_L, REG_EAX, REG_EBX);					// cmovl    eax,ebx
				_cmp_r32_imm(REG_EAX, 0xffff);								// cmp      eax,0xffff
				_mov_r32_imm(REG_EBX, 0xffff);								// mov      ebx,0xffff
				_cmov_r32_r32(COND_G, REG_EAX, REG_EBX);					// cmovg    eax,ebx
			}
			else if (!FBZMODE_BITS(21,1))
			{
				/* W value already computed above */
			}
			else
			{
				_comiss_r128_m32abs(REG_XMM4, &ard->zfw_scale[0]);			// comiss   xmm4,1.0
				_mov_r32_imm(REG_EAX, 0);									// mov      eax,0
				_cvttss2si_r32_r128(REG_EBX, REG_XMM4);						// cvttss2si ebx,xmm4
				_jcc_short_link(COND_AE, &link1);							// jae      skip
				_comiss_r128_m32abs(REG_XMM4, &ard->f32_0[0]);				// comiss   xmm4,0.0
				_mov_r32_imm(REG_EAX, 0xffff);								// mov      eax,0xffff
				_jcc_short_link(COND_B, &link2);							// jb       skip
				_bsr_r32_r32(REG_ECX, REG_EBX);								// bsr      ecx,ebx
				_mov_r32_imm(REG_EAX, 0xfffe);								// mov      eax,0xfffe
				_jcc_short_link(COND_Z, &link3);							// jz       skip
				_cmp_r32_imm(REG_ECX, 19);									// cmp      ecx,19
				_mov_r32_imm(REG_EDX, 19);									// mov      edx,19
				_cmov_r32_r32(COND_A, REG_ECX, REG_EDX);					// cmova    ecx,edx
				_shl_r32_cl(REG_EBX);										// shl      ebx,cl
				_lea_r32_m32bd(REG_EAX, REG_ECX, -4);						// lea      eax,[ecx-4]
				_shr_r32_imm(REG_EBX, 19);									// shr      ebx,19
				_shl_r32_imm(REG_EAX, 12);									// shl      eax,12
				_and_r32_imm(REG_EBX, 0xfff);								// and      ebx,0xfff
				_or_r32_r32(REG_EAX, REG_EBX);								// or       eax,ebx
				_xor_r32_imm(REG_EAX, 0xfff);								// xor      eax,0xfff
				_resolve_link(&link1);										// skip:
				_resolve_link(&link2);										// skip:
				_resolve_link(&link3);										// skip:
			}

			/* if we're storing the depth, save it for later */
			/* EAX = depthval, ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(10,1))
				_mov_m32abs_r32(&_depthval, REG_EAX);						// mov      depthval,eax

			/* handle depth buffer testing */
			/* EAX = depthval, ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(4,1) && FBZMODE_BITS(5,3) != 7)
			{
				if (FBZMODE_BITS(5,3) == 0)
					_jmp_near_link(&nodraw_link3);							// jmp      nodraw

				/* the source depth is either the iterated W/Z+bias or a constant value */
				if (!FBZMODE_BITS(20,1))
				{
					/* add the bias */
					if (FBZMODE_BITS(16,1))
					{
						_movsx_r32_m16abs(REG_EBX, &voodoo_regs[zaColor]);	// movsx    ebx,zaColor
						_add_r32_r32(REG_EAX, REG_EBX);						// add      eax,ebx
						_cmp_r32_imm(REG_EAX, 0xffff);						// cmp      eax,0xffff
						_mov_r32_imm(REG_EBX, 0xffff);						// mov      ebx,0xffff
						_cmov_r32_r32(COND_G, REG_EAX, REG_EBX);			// cmovg    eax,ebx
						_cmp_r32_imm(REG_EAX, 0);							// cmp      eax,0
						_mov_r32_imm(REG_EBX, 0);							// mov      ebx,0
						_cmov_r32_r32(COND_L, REG_EAX, REG_EBX);			// cmovl    eax,ebx
					}
				}
				else
					_movzx_r32_m16abs(REG_EAX, &voodoo_regs[zaColor]);		// movzx    eax,zaColor

				/* test against the depth buffer */
				_cmp_r16_m16bisd(REG_AX, REG_ESI, REG_EBP, 2, 0);			// cmp      ax,[esi+ebp*2]
				switch (FBZMODE_BITS(5,3))
				{
					case 1:
						_jcc_near_link(COND_AE, &nodraw_link3);				// jae      nodraw
						break;
					case 2:
						_jcc_near_link(COND_NE, &nodraw_link3);				// jne      nodraw
						break;
					case 3:
						_jcc_near_link(COND_A, &nodraw_link3);				// ja       nodraw
						break;
					case 4:
						_jcc_near_link(COND_BE, &nodraw_link3);				// jbe      nodraw
						break;
					case 5:
						_jcc_near_link(COND_E, &nodraw_link3);				// je       nodraw
						break;
					case 6:
						_jcc_near_link(COND_B, &nodraw_link3);				// jb       nodraw
						break;
				}
			}

			/* load the texel if necessary */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (do_tmu0)
			{
				int persp0 = TEXTUREMODE0_BITS(0,1);
				int persp1 = (do_tmu1) ? TEXTUREMODE1_BITS(0,1) : persp0;

				/* perspective correct both TMUs up front in XMM2, and compute lodresult */
				if (persp0 != persp1)
				{
//                  osd_die("Unimplemented: one TMU perspective corrected, one not!");
					persp1 = persp0 = 1;
				}
				if (persp0 && persp1)
				{
					_movaps_r128_m128abs(REG_XMM2, &ard->f32_1[0]);				// movaps   xmm2,1.0
					_movaps_r128_m128abs(REG_XMM0, &ard->lodbase[0]);			// movaps   xmm0,ard->lodbase
					_divps_r128_r128(REG_XMM2, REG_XMM5);						// divps    xmm2,xmm5
					_mulps_r128_r128(REG_XMM0, REG_XMM2);						// mulps    xmm0,xmm2
					_mulps_r128_r128(REG_XMM0, REG_XMM2);						// mulps    xmm0,xmm2
					_mulps_r128_r128(REG_XMM2, REG_XMM6);						// mulps    xmm2,xmm6
					_movaps_m128abs_r128(&ard->lodresult[0], REG_XMM0);			// movaps   ard->lodresult,xmm0
					_cvttps2dq_r128_r128(REG_XMM2, REG_XMM2);					// cvttps2dq xmm2,xmm2
				}
				else if (!persp0 && !persp1)
				{
					_movaps_r128_m128abs(REG_XMM0, &ard->lodbase[0]);			// movaps   xmm0,ard->lodbase
					_cvttps2dq_r128_r128(REG_XMM2, REG_XMM6);					// cvttps2dq xmm2,xmm6
					_movaps_m128abs_r128(&ard->lodresult[0], REG_XMM0);			// movaps   ard->lodresult,xmm0
				}

				/* start with a 0 c_other value */
				_pxor_r128_r128(REG_XMM3, REG_XMM3);						// pxor     xmm3,xmm3

				/* if we use two TMUs, load texel from TMU1 in place of c_other */
				if (do_tmu1)
				{
					_movdqa_m128abs_r128(&ard->uvsave, REG_XMM2);			// movdqa   ard->uvsave,xmm2
					generate_load_texel(drc, 1, lookup1);					// <load texel into xmm3>
					_movdqa_r128_m128abs(REG_XMM2, &ard->uvsave);			// movdqa   xmm2,ard->uvsave
				}

				/* now load texel from TMU0 */
				generate_load_texel(drc, 0, lookup0);						// <load texel into xmm3>

				/* handle chroma key */
				if (FBZMODE_BITS(1,1))
				{
					/* note: relies on texture code leaving packed value in XMM1 */
					_movd_r32_r128(REG_EAX, REG_XMM1);						// movd     eax,xmm1
					_xor_r32_m32abs(REG_EAX, &voodoo_regs[chromaKey]);		// xor      eax,[chromaKey]
					_and_r32_imm(REG_EAX, 0xffffff);						// and      eax,0xffffff
					_jcc_near_link(COND_Z, &nodraw_link4);					// jz       nodraw
				}
			}

			/* compute c_local (XMM2) */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (!FBZCOLORPATH_BITS(7,1))
			{
				if (!FBZCOLORPATH_BITS(4,1))			/* cc_localselect mux == iterated RGB */
				{
					_movdqa_r128_r128(REG_XMM2, REG_XMM7);					// movdqa   xmm2,xmm7
					_psrld_r128_imm(REG_XMM2, 16);							// psrld    xmm2,16
					_pand_r128_m128abs(REG_XMM2, &ard->i32_0xff[0]);		// pand     xmm2,0x000000ff x 4
					_packssdw_r128_r128(REG_XMM2, REG_XMM2);				// packssdw xmm2,xmm2
				}
				else									/* cc_localselect mux == color0 RGB */
				{
					_movd_r128_m32abs(REG_XMM2, &voodoo_regs[color0]);		// movd     xmm2,[color0]
					_pxor_r128_r128(REG_XMM1, REG_XMM1);					// pxor     xmm1,xmm1
					_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);				// punpcklbw xmm2,xmm1
				}
			}
			else
			{
				_pextrw_r32_r128(REG_EAX, REG_XMM3, 3);						// pextrw   eax,xmm3,3
				_and_r32_imm(REG_EAX, 0x0080);								// and      eax,0x80
				_jcc_short_link(COND_NZ, &link1);							// jnz      skip
				_movdqa_r128_r128(REG_XMM2, REG_XMM7);						// movdqa   xmm2,xmm7
				_psrld_r128_imm(REG_XMM2, 16);								// psrld    xmm2,16
				_pand_r128_m128abs(REG_XMM2, &ard->i32_0xff[0]);			// pand     xmm2,0x000000ff x 4
				_packssdw_r128_r128(REG_XMM2, REG_XMM2);					// packssdw xmm2,xmm2
				_jmp_short_link(&link2);									// jmp      skip2
				_resolve_link(&link1);										// skip:
				_movd_r128_m32abs(REG_XMM2, &voodoo_regs[color0]);			// movd     xmm2,[color0]
				_pxor_r128_r128(REG_XMM1, REG_XMM1);						// pxor     xmm1,xmm1
				_punpcklbw_r128_r128(REG_XMM2, REG_XMM1);					// punpcklbw xmm2,xmm1
				_resolve_link(&link2);										// skip2:
			}

			/* compute a_local */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM2 = c_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(5,2) == 0)		/* cca_localselect mux == iterated alpha */
				_pextrw_r32_r128(REG_EAX, REG_XMM7, 7);						// pextrw   eax,xmm7,7
			else if (FBZCOLORPATH_BITS(5,2) == 1)	/* cca_localselect mux == color0 alpha */
				_movzx_r32_m8abs(REG_EAX, &((UINT8 *)&voodoo_regs[color0])[3]);// movzx eax,[color0][3]
			else if (FBZCOLORPATH_BITS(5,2) == 2)	/* cca_localselect mux == iterated Z */
			{
				if (needs_z_depth)
				{
					_cvttss2si_r32_r128(REG_EAX, REG_XMM4);					// cvttss2si eax,xmm4
					_shr_r32_imm(REG_EAX, 12);								// shr      eax,12
				}
				else if (needs_z_float_depth)
				{
					_cvttss2si_r32_r128(REG_EAX, REG_XMM4);					// cvttss2si eax,xmm4
					_shr_r32_imm(REG_EAX, 24);								// shr      eax,24
				}
				else
				{
					_pshufd_r128_r128(REG_XMM0, REG_XMM4, 0x55);			// pshufd   xmm0,xmm4,0x55
					_cvttss2si_r32_r128(REG_EAX, REG_XMM0);					// cvttss2si eax,xmm0
					_shr_r32_imm(REG_EAX, 12);								// shr      eax,12
				}
			}
			_pinsrw_r128_r32(REG_XMM2, REG_EAX, 3);							// pinsrw   xmm2,eax,3

			/* determine the RGB values (XMM1) */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (!FBZCOLORPATH_BITS(8,1))				/* cc_zero_other */
			{
				if (FBZCOLORPATH_BITS(0,2) == 0)		/* cc_rgbselect mux == iterated RGB */
				{
					_movdqa_r128_r128(REG_XMM1, REG_XMM7);					// movdqa   xmm1,xmm7
					_psrld_r128_imm(REG_XMM1, 16);							// psrld    xmm1,16
					_pand_r128_m128abs(REG_XMM1, &ard->i32_0xff[0]);		// pand     xmm1,0x000000ff x 4
					_packssdw_r128_r128(REG_XMM1, REG_XMM1);				// packssdw xmm1,xmm1
				}
				else if (FBZCOLORPATH_BITS(0,2) == 1)	/* cc_rgbselect mux == texture RGB */
					_movdqa_r128_r128(REG_XMM1, REG_XMM3);					// movdqa   xmm1,xmm3

				else if (FBZCOLORPATH_BITS(0,2) == 2)	/* cc_rgbselect mux == color1 RGB */
				{
					_movd_r128_m32abs(REG_XMM1, &voodoo_regs[color1]);		// movd     xmm1,[color1]
					_pxor_r128_r128(REG_XMM0, REG_XMM0);					// pxor     xmm0,xmm0
					_punpcklbw_r128_r128(REG_XMM1, REG_XMM0);				// punpcklbw xmm1,xmm0
				}
			}
			else
				_pxor_r128_r128(REG_XMM1, REG_XMM1);						// pxor     xmm1,xmm1

			/* subtract local color */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(9,1))					/* cc_sub_clocal */
				_psubw_r128_r128(REG_XMM1, REG_XMM2);						// psubw    xmm1,xmm2

			/* scale RGB */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(10,3) != 0)			/* cc_mselect mux */
			{
				switch (FBZCOLORPATH_BITS(10,3))
				{
					case 1:		/* cc_mselect mux == c_local */
						_movdqa_r128_r128(REG_XMM0, REG_XMM2);				// movdqa   xmm0,xmm2
						break;

					case 2:		/* cc_mselect mux == a_other */
						if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
						{
							_pshufhw_r128_r128(REG_XMM0, REG_XMM7, 0x55);	// psuhfhw  xmm0,xmm7,0x55
							_psrldq_r128_imm(REG_XMM0, 8);					// psrldq   xmm0,8
						}
						else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
							_pshuflw_r128_r128(REG_XMM0, REG_XMM3, 0xff);	// pshuflw  xmm0,xmm3,0xff
						else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
						{
							_movd_r128_m32abs(REG_XMM0, &voodoo_regs[color1]);// movd   xmm0,color1
							_psrldq_r128_imm(REG_XMM0, 3);					// psrldq   xmm0,3
							_pshuflw_r128_r128(REG_XMM0, REG_XMM0, 0x00);	// pshuflw  xmm0,xmm0,0x00
						}
						else
							_pxor_r128_r128(REG_XMM0, REG_XMM0);
						break;

					case 3:		/* cc_mselect mux == a_local */
						_pshuflw_r128_r128(REG_XMM0, REG_XMM2, 0xff);		// pshuflw  xmm0,xmm2,0xff
						break;

					case 4:		/* cc_mselect mux == texture alpha */
						_pshuflw_r128_r128(REG_XMM0, REG_XMM3, 0xff);		// pshuflw  xmm0,xmm3,0xff
						break;

					default:
						_pxor_r128_r128(REG_XMM0, REG_XMM0);
						break;
				}

				if (!FBZCOLORPATH_BITS(13,1))			/* cc_reverse_blend */
					_pxor_r128_m128abs(REG_XMM0, &ard->i16_0xff[0]);		// pxor     xmm0,0x00ff x 4

				_paddw_r128_m128abs(REG_XMM0, &ard->i16_1[0]);				// paddw    xmm0,0x0001 x 4
				_pmullw_r128_r128(REG_XMM1, REG_XMM0);						// pmullw   xmm1,xmm0
				_psrlw_r128_imm(REG_XMM1, 8);								// psrlw    xmm1,8
			}

			/* add local color */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(14,1))				/* cc_add_clocal */
				_paddw_r128_r128(REG_XMM1, REG_XMM2);						// paddw    xmm1,xmm3

			/* add local alpha */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			else if (FBZCOLORPATH_BITS(15,1))			/* cc_add_alocal */
			{
				_pshuflw_r128_r128(REG_XMM0, REG_XMM2, 0xff);				// pshuflw  xmm0,xmm2,0xff
				_paddw_r128_r128(REG_XMM1, REG_XMM0);						// paddw    xmm1,xmm0
			}

			/* determine the A value */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (!FBZCOLORPATH_BITS(17,1))				/* cca_zero_other */
			{
				if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
					_pextrw_r32_r128(REG_EDX, REG_XMM7, 7);					// pextrw   edx,xmm7,7
				else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
					_pextrw_r32_r128(REG_EDX, REG_XMM3, 3);					// pextrw   edx,xmm3,3
				else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
					_movzx_r32_m8abs(REG_EDX, (UINT8 *)&voodoo_regs[color1] + 3);// movzx edx,color1.alpha
			}
			else
				_mov_r32_imm(REG_EDX, 0);									// mov      edx,0

			/* subtract local alpha */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(18,1))				/* cca_sub_clocal */
			{
				_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);						// pextrw   ebx,xmm2,3
				_sub_r32_r32(REG_EDX, REG_EBX);								// sub      edx,ebx
			}

			/* scale alpha */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM3 = ac_other, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(19,3) != 0)			/* cca_mselect mux */
			{
				switch (FBZCOLORPATH_BITS(19,3))
				{
					case 1:		/* cca_mselect mux == a_local */
					case 3:		/* cca_mselect mux == a_local */
						_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);				// pextrw   ebx,xmm2,3
						break;

					case 2:		/* cca_mselect mux == a_other */
						if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
							_pextrw_r32_r128(REG_EBX, REG_XMM7, 7);			// pextrw   ebx,xmm7,7
						else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
							_pextrw_r32_r128(REG_EBX, REG_XMM3, 3);			// pextrw   ebx,xmm3,3
						else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
							_movzx_r32_m8abs(REG_EBX, (UINT8 *)&voodoo_regs[color1] + 3);// movzx ebx,color1.alpha
						else
							_mov_r32_imm(REG_EBX, 0);						// mov      ebx,0
						break;

					case 4:		/* cca_mselect mux == texture alpha */
						_pextrw_r32_r128(REG_EBX, REG_XMM3, 3);				// pextrw   ebx,xmm3,3
						break;

					default:
						_mov_r32_imm(REG_EBX, 0);							// mov      ebx,0
						break;
				}

				if (!FBZCOLORPATH_BITS(22,1))			/* cca_reverse_blend */
					_xor_r32_imm(REG_EBX, 0xff);							// xor      ebx,0xff
				_add_r32_imm(REG_EBX, 1);									// add      ebx,1
				_imul_r32_r32(REG_EDX, REG_EBX);							// imul     edx,ebx
				_shr_r32_imm(REG_EDX, 8);									// shr      edx,8
			}

			/* add local alpha */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM2 = ac_local, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(23,2))				/* cca_add_clocal */
			{
				_pextrw_r32_r128(REG_EBX, REG_XMM2, 3);						// pextrw   ebx,xmm2,3
				_add_r32_r32(REG_EDX, REG_EBX);								// add      edx,ebx
			}

			/* handle alpha masking */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(13,1))
			{
				_test_r32_imm(REG_EDX, 1);									// test     edx,1
				_jcc_near_link(COND_Z, &nodraw_link5);						// jz       nodraw
			}

			/* apply alpha function */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (ALPHAMODE_BITS(0,4))
			{
				_cmp_r32_imm(REG_EDX, ALPHAMODE_BITS(24,8));				// cmp      edx,alphaval
				switch (ALPHAMODE_BITS(1,3))
				{
					case 1:
						_jcc_near_link(COND_AE, &nodraw_link6);				// jae      nodraw
						break;
					case 2:
						_jcc_near_link(COND_NE, &nodraw_link6);				// jne      nodraw
						break;
					case 3:
						_jcc_near_link(COND_A, &nodraw_link6);				// ja       nodraw
						break;
					case 4:
						_jcc_near_link(COND_BE, &nodraw_link6);				// jbe      nodraw
						break;
					case 5:
						_jcc_near_link(COND_E, &nodraw_link6);				// je       nodraw
						break;
					case 6:
						_jcc_near_link(COND_B, &nodraw_link6);				// jb       nodraw
						break;
				}
			}

			/* invert */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZCOLORPATH_BITS(16,1))				/* cc_invert_output */
				_pxor_r128_m128abs(REG_XMM1, &ard->i16_0xff[0]);			// pxor     xmm1,0x00ff x8
			if (FBZCOLORPATH_BITS(25,1))				/* cca_invert_output */
				_xor_r32_imm(REG_EDX, 0xff);								// xor      edx,0xff

			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FOGGING && FOGMODE_BITS(0,1))
			{
				if (FOGMODE_BITS(5,1))					/* fogconstant */
				{
					_movd_r128_m32abs(REG_XMM3, &voodoo_regs[fogColor]);	// movd     xmm3,[fogColor]
					_pxor_r128_r128(REG_XMM0, REG_XMM0);					// pxor     xmm0,xmm0
					_punpcklbw_r128_r128(REG_XMM3, REG_XMM0);				// punpcklbw xmm3,xmm0
					_paddw_r128_r128(REG_XMM1, REG_XMM3);					// paddw    xmm1,xmm3
				}
				else
				{
					if (FOGMODE_BITS(4,1))				/* fogz */
					{
						if (needs_z_depth)
						{
							_cvttss2si_r32_r128(REG_EAX, REG_XMM4);			// cvttss2si eax,xmm4
							_shr_r32_imm(REG_EAX, 12);						// shr      eax,12
						}
						else if (needs_z_float_depth)
						{
							_cvttss2si_r32_r128(REG_EAX, REG_XMM4);			// cvttss2si eax,xmm4
							_shr_r32_imm(REG_EAX, 24);						// shr      eax,24
						}
						else
						{
							_pshufd_r128_r128(REG_XMM0, REG_XMM4, 0x55);	// pshufd   xmm0,xmm4,0x55
							_cvttss2si_r32_r128(REG_EAX, REG_XMM0);			// cvttss2si eax,xmm0
							_shr_r32_imm(REG_EAX, 12);						// shr      eax,12
						}
					}
					else if (FOGMODE_BITS(3,1))			/* fogalpha */
						_pextrw_r32_r128(REG_EAX, REG_XMM7, 7);				// pextrw   eax,xmm7,7
					else
					{
						_mov_r32_m32abs(REG_EBX, &_wfloat);					// mov      ebx,_wfloat
						_mov_r32_r32(REG_ECX, REG_EBX);						// mov      ecx,ebx
						_shr_r32_imm(REG_EBX, 10);							// shr      ebx,10
						_shr_r32_imm(REG_ECX, 2);							// shr      ecx,2
						_movzx_r32_m8bd(REG_EAX, REG_EBX, &fog_blend[0]);	// movzx    eax,[ebx + fog_blend]
						_and_r32_imm(REG_ECX, 0xff);						// and      ecx,0xff
						_movzx_r32_m8bd(REG_EBX, REG_EBX, &fog_delta[0]);	// movzx    ebx,[ebx + fog_delta]
						_imul_r32_r32(REG_EBX, REG_ECX);					// imul     ebx,ecx
						_shr_r32_imm(REG_EBX, 10);							// shr      ebx,10
						_add_r32_r32(REG_EAX, REG_EBX);						// add      eax,ebx
					}

					if (!FOGMODE_BITS(2,1))				/* fogmult */
					{
						_mov_r32_imm(REG_EBX, 0x100);						// mov      ebx,0x100
						_sub_r32_r32(REG_EBX, REG_EAX);						// sub      ebx,eax
						_movd_r128_r32(REG_XMM0, REG_EBX);					// movd     xmm0,ebx
						_pshuflw_r128_r128(REG_XMM0, REG_XMM0, 0);			// pshuflw  xmm0,xmm0,0
						_pmullw_r128_r128(REG_XMM1, REG_XMM0);				// pmullw   xmm1,xmm0
						_psrlw_r128_imm(REG_XMM1, 8);						// psrlw    xmm1,8
					}
					else
						_pxor_r128_r128(REG_XMM1, REG_XMM1);				// pxor     xmm1,xmm1

					if (!FOGMODE_BITS(1,1))				/* fogadd */
					{
						_movd_r128_m32abs(REG_XMM3, &voodoo_regs[fogColor]);// movd     xmm3,[fogColor]
						_pxor_r128_r128(REG_XMM0, REG_XMM0);				// pxor     xmm0,xmm0
						_punpcklbw_r128_r128(REG_XMM3, REG_XMM0);			// punpcklbw xmm3,xmm0
						_movd_r128_r32(REG_XMM0, REG_EAX);					// movd     xmm0,eax
						_pshuflw_r128_r128(REG_XMM0, REG_XMM0, 0);			// pshuflw  xmm0,xmm0,0
						_pmullw_r128_r128(REG_XMM3, REG_XMM0);				// pmullw   xmm3,xmm0
						_psrlw_r128_imm(REG_XMM3, 8);						// psrlw    xmm3,8
						_paddw_r128_r128(REG_XMM1, REG_XMM3);				// paddw    xmm1,xmm3
					}
				}
			}

			/* apply alpha blending */
			/* EDX = a, ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (ALPHA_BLENDING && ALPHAMODE_BITS(4,1))
			{
				_movzx_r32_m16bisd(REG_EAX, REG_EDI, REG_EBP, 2, 0);		// movzx    eax,[edi+ebp*2]
				_movdqa_r128_r128(REG_XMM0, REG_XMM1);						// movdqa   xmm0,xmm1

				/* compute source portion */
				switch (ALPHAMODE_BITS(8,4))
				{
					case 0:		// AZERO
					case 7:		// AOMDST_ALPHA
// fix me -- we don't support alpha layer on the dest
						_pxor_r128_r128(REG_XMM1, REG_XMM1);				// pxor     xmm1,xmm1
						break;
					case 1:		// ASRC_ALPHA
						_lea_r32_m32bd(REG_ECX, REG_EDX, 1);				// lea      ecx,[edx+1]
						_movd_r128_r32(REG_XMM1, REG_ECX);					// movd     xmm1,ecx
						_pshuflw_r128_r128(REG_XMM1, REG_XMM1, 0x00);		// pshuflw  xmm1,xmm1,0x00
						_pmullw_r128_r128(REG_XMM1, REG_XMM0);				// pmullw   xmm1,xmm0
						_psrlw_r128_imm(REG_XMM1, 8);						// psrlw    xmm1,8
						break;
					case 2:		// A_COLOR
						_movq_r128_m64isd(REG_XMM1, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm1,uncolor_table[eax*8]
						_paddw_r128_m128abs(REG_XMM1, &ard->i16_1[0]);		// paddw    xmm1,0x0001 x 8
						_pmullw_r128_r128(REG_XMM1, REG_XMM0);				// pmullw   xmm1,xmm0
						_psrlw_r128_imm(REG_XMM1, 8);						// psrlw    xmm1,8
						break;
					case 3:		// ADST_ALPHA
// fix me -- we don't support alpha layer on the dest
					case 4:		// AONE
						break;
					case 5:		// AOMSRC_ALPHA
						_mov_r32_imm(REG_ECX, 0x100);						// mov      ecx,0x100
						_sub_r32_r32(REG_ECX, REG_EDX);						// sub      ecx,edx
						_movd_r128_r32(REG_XMM1, REG_ECX);					// movd     xmm1,ecx
						_pshuflw_r128_r128(REG_XMM1, REG_XMM1, 0x00);		// pshuflw  xmm1,xmm1,0x00
						_pmullw_r128_r128(REG_XMM1, REG_XMM0);				// pmullw   xmm1,xmm0
						_psrlw_r128_imm(REG_XMM1, 8);						// psrlw    xmm1,8
						break;
					case 6:		// AOM_COLOR
						_movq_r128_m64isd(REG_XMM1, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm0,uncolor_table[eax*8]
						_pxor_r128_m128abs(REG_XMM1, &ard->i16_0xff[0]);	// pxor     xmm1,0x00ff x 8
						_paddw_r128_m128abs(REG_XMM1, &ard->i16_1[0]);		// paddw    xmm1,0x0001 x 8
						_pmullw_r128_r128(REG_XMM1, REG_XMM0);				// pmullw   xmm1,xmm0
						_psrlw_r128_imm(REG_XMM1, 8);						// psrlw    xmm1,8
						break;
				}

				/* add in dest portion */
				switch (ALPHAMODE_BITS(12,4))
				{
					case 0:		// AZERO
					case 7:		// AOMDST_ALPHA
// fix me -- we don't support alpha layer on the dest
						break;
					case 1:		// ASRC_ALPHA
						_lea_r32_m32bd(REG_ECX, REG_EDX, 1);				// lea      ecx,[edx+1]
						_movd_r128_r32(REG_XMM2, REG_ECX);					// movd     xmm2,ecx
						_movq_r128_m64isd(REG_XMM3, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm3,uncolor_table[eax*8]
						_pshuflw_r128_r128(REG_XMM2, REG_XMM2, 0x00);		// pshuflw  xmm2,xmm2,0x00
						_pmullw_r128_r128(REG_XMM2, REG_XMM3);				// pmullw   xmm2,xmm3
						_psrlw_r128_imm(REG_XMM2, 8);						// psrlw    xmm2,8
						_paddw_r128_r128(REG_XMM1, REG_XMM2);				// paddw    xmm1,xmm2
						break;
					case 2:		// A_COLOR
						_movq_r128_m64isd(REG_XMM3, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm3,uncolor_table[eax*8]
						_paddw_r128_m128abs(REG_XMM0, &ard->i16_1[0]);		// paddw    xmm0,0x0001 x 8
						_pmullw_r128_r128(REG_XMM0, REG_XMM3);				// pmullw   xmm0,xmm3
						_psrlw_r128_imm(REG_XMM0, 8);						// psrlw    xmm0,8
						_paddw_r128_r128(REG_XMM1, REG_XMM0);				// paddw    xmm1,xmm0
						break;
					case 3:		// ADST_ALPHA
// fix me -- we don't support alpha layer on the dest
					case 4:		// AONE
						_movq_r128_m64isd(REG_XMM3, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm3,uncolor_table[eax*8]
						_paddw_r128_r128(REG_XMM1, REG_XMM3);				// paddw    xmm1,xmm3
						break;
					case 5:		// AOMSRC_ALPHA
						_mov_r32_imm(REG_ECX, 0x100);						// mov      ecx,0x100
						_sub_r32_r32(REG_ECX, REG_EDX);						// sub      ecx,edx
						_movd_r128_r32(REG_XMM2, REG_ECX);					// movd     xmm2,ecx
						_movq_r128_m64isd(REG_XMM3, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm3,uncolor_table[eax*8]
						_pshuflw_r128_r128(REG_XMM2, REG_XMM2, 0x00);		// pshuflw  xmm2,xmm2,0x00
						_pmullw_r128_r128(REG_XMM2, REG_XMM3);				// pmullw   xmm2,xmm3
						_psrlw_r128_imm(REG_XMM2, 8);						// psrlw    xmm2,8
						_paddw_r128_r128(REG_XMM1, REG_XMM2);				// paddw    xmm1,xmm2
						break;
					case 6:		// AOM_COLOR
						_movq_r128_m64isd(REG_XMM3, REG_EAX, 8, &ard->uncolor_table[0]);// movq xmm3,uncolor_table[eax*8]
						_pxor_r128_m128abs(REG_XMM0, &ard->i16_0xff[0]);	// pxor     xmm0,0x00ff x 8
						_paddw_r128_m128abs(REG_XMM0, &ard->i16_1[0]);		// paddw    xmm0,0x0001 x 8
						_pmullw_r128_r128(REG_XMM0, REG_XMM3);				// pmullw   xmm0,xmm3
						_psrlw_r128_imm(REG_XMM0, 8);						// psrlw    xmm0,8
						_paddw_r128_r128(REG_XMM1, REG_XMM0);				// paddw    xmm1,xmm0
						break;
				}
			}

			/* apply dithering */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(8,1))
			{
				_packuswb_r128_r128(REG_XMM1, REG_XMM1);					// packuswb xmm1,xmm1
				_pxor_r128_r128(REG_XMM0, REG_XMM0);						// pxor     xmm0,xmm0
				_mov_r32_m32abs(REG_EAX, &_curry);							// mov      eax,_curry
				_movdqa_r128_r128(REG_XMM2, REG_XMM1);						// movdqa   xmm2,xmm1
				_mov_r32_r32(REG_EBX, REG_EBP);								// mov      ebx,ebp
				_punpcklbw_r128_r128(REG_XMM1, REG_XMM0);					// punpcklbw xmm1,xmm0
				_and_r32_imm(REG_EAX, 3);									// and      eax,3
				_punpcklbw_r128_r128(REG_XMM2, REG_XMM0);					// punpcklbw xmm2,xmm0
				_and_r32_imm(REG_EBX, 3);									// and      ebx,3
				_psllw_r128_imm(REG_XMM1, 1);								// psllw    xmm1,1
				_shl_r32_imm(REG_EAX, 6);									// shl      eax,5
				_psrlw_r128_imm(REG_XMM2, 4);								// psrlw    xmm2,4
				_shl_r32_imm(REG_EBX, 4);									// shl      ebx,3
				_psubw_r128_r128(REG_XMM1, REG_XMM2);						// psubw    xmm1,xmm2
				if (!FBZMODE_BITS(11,1))
					_paddw_r128_m128bisd(REG_XMM1, REG_EAX, REG_EBX, 1, &ard->dither_4x4_expand[0]);// paddw xmm1,[eax+ebx+dither_4x4_expand]
				else
					_paddw_r128_m128bisd(REG_XMM1, REG_EAX, REG_EBX, 1, &ard->dither_2x2_expand[0]);// paddw xmm1,[eax+ebx+dither_2x2_expand]
				_psrlw_r128_imm(REG_XMM1, 1);								// psrlw    xmm1,1
			}

			/* clamp */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			_pinsrw_r128_r32(REG_XMM1, REG_EDX, 3);							// pinsrw   xmm1,edx,3
			_packuswb_r128_r128(REG_XMM1, REG_XMM1);						// packuswb xmm1,xmm1
			_movd_r32_r128(REG_EAX, REG_XMM1);								// movd     eax,xmm1

			/* write the pixel data */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM1 = rgb, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(9,1))
			{
				_mov_r32_r32(REG_EBX, REG_EAX);								// mov      ebx,eax
				_mov_r32_r32(REG_ECX, REG_EAX);								// mov      ecx,eax
				_shr_r32_imm(REG_EAX, 8);									// shr      eax,8
				_shr_r32_imm(REG_EBX, 5);									// shr      ebx,5
				_shr_r32_imm(REG_ECX, 3);									// shr      ecx,3
				_and_r32_imm(REG_EAX, 0xf800);								// and      eax,0xf800
				_and_r32_imm(REG_EBX, 0x07e0);								// and      ebx,0x07e0
				_and_r32_imm(REG_ECX, 0x001f);								// and      ecx,0x001f
				_or_r32_r32(REG_EAX, REG_EBX);								// or       eax,ebx
				_or_r32_r32(REG_EAX, REG_ECX);								// or       eax,ecx
				_mov_m16bisd_r16(REG_EDI, REG_EBP, 2, 0, REG_AX);			// mov      [edi+ebp*2],ax
			}

			/* write the depth data */
			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			if (FBZMODE_BITS(10,1))
			{
				_mov_r16_m16abs(REG_AX, &_depthval);						// mov      ax,depthval
				_mov_m16bisd_r16(REG_ESI, REG_EBP, 2, 0, REG_AX);			// mov      [esi+ebp*2],ax
			}

			if (nodraw_link1.target != NULL) _resolve_link(&nodraw_link1);
			if (nodraw_link2.target != NULL) _resolve_link(&nodraw_link2);
			if (nodraw_link3.target != NULL) _resolve_link(&nodraw_link3);
			if (nodraw_link4.target != NULL) _resolve_link(&nodraw_link4);
			if (nodraw_link5.target != NULL) _resolve_link(&nodraw_link5);
			if (nodraw_link6.target != NULL) _resolve_link(&nodraw_link6);

			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			_addps_r128_m128abs(REG_XMM4, &ard->dwz_dx[0]);					// addps    xmm4,ard->dwz_dx
			if (do_tmu0)
			{
				_addps_r128_m128abs(REG_XMM5, &ard->dw01_dx[0]);			// addps    xmm5,ard->dw01_dx
				_addps_r128_m128abs(REG_XMM6, &ard->dst01_dx[0]);			// addps    xmm6,ard->dst01_dx
			}
			_paddd_r128_m128abs(REG_XMM7, &ard->dargb_idx[0]);				// paddd    xmm7,ard->dargb_idx

			/* ESI -> depth, EDI -> draw, EBP = x, XMM4 = 00wz, XMM5 = w0w0w1w1, XMM6 = s0t0s1t1, XMM7 = argb */
			_add_r32_imm(REG_EBP, 1);										// add      ebp,1
			_cmp_r32_m32abs(REG_EBP, &_stopx);								// cmp      ebp,_stopx
			_jcc(COND_B, xlooptop);											// jb       xlooptop

		if (yloopend_link1.target != NULL) _resolve_link(&yloopend_link1);
		if (yloopend_link2.target != NULL) _resolve_link(&yloopend_link2);
		if (yloopend_link3.target != NULL) _resolve_link(&yloopend_link3);

		_mov_r32_m32abs(REG_EAX, &_curry);									// mov      eax,_curry
		_add_r32_imm(REG_EAX, 1);											// add      eax,1
		_cmp_r32_m32abs(REG_EAX, &_stopy);									// cmp      eax,_stopy
		_mov_m32abs_r32(&_curry, REG_EAX);									// mov      _curry,eax
		_jcc(COND_B, ylooptop);												// jb       ylooptop

	if (funcend_link1.target != NULL) _resolve_link(&funcend_link1);

	_and_m32abs_imm(&voodoo_regs[fbiPixelsIn], 0xffffff);					// and      fbiPixelsIn,0xffffff
	_pop_r32(REG_EBP);														// pop      ebp
	_pop_r32(REG_EDI);														// pop      edi
	_pop_r32(REG_ESI);														// pop      esi
	_pop_r32(REG_EBX);														// pop      ebx
	_ret();																	// ret

#if (LOG_CODE)
{
	FILE *f = fopen("code.bin", "wb");
	fwrite(drc->cache_base, 1, drc->cache_top - drc->cache_base, f);
	fclose(f);
}
#endif

	return add_rasterizer(start);
}

#undef BITFIELD
#undef FBZCOLORPATH_BITS
#undef FOGMODE_BITS
#undef ALPHAMODE_BITS
#undef FBZMODE_BITS
#undef TEXTUREMODE_BITS
#undef TEXTUREMODE0_BITS
#undef TEXTUREMODE1_BITS
