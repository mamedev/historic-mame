#ifndef NUM_TMUS
#error need to define the number of TMUs
#endif


/*
    Things yet to verify:

    * LOD calculations
    * LOD dither

    // use texture that is all FF in RGB

    // combine with to get LOD fraction in low bits
    grTexCombine(GR_TMU0,
                 GR_COMBINE_FUNCTION_BLEND_LOCAL, // (1-f)*local
                 GR_COMBINE_FACTOR_LOD_FRACTION,
                 GR_COMBINE_FUNCTION_BLEND_LOCAL, // (1-f)*local
                 GR_COMBINE_FACTOR_DETAIL_FACTOR,
                 FXTRUE,                          // invert to give f*local
                 FXFALSE);

    // combine with to get different LOD value (8-LOD)<<4
    grTexDetailControl(GR_TMU0, 8, 4, 1.0);
    grTexCombine(GR_TMU0,
                 GR_COMBINE_FUNCTION_BLEND_LOCAL, // (1-f)*local
                 GR_COMBINE_FACTOR_DETAIL_FACTOR,
                 GR_COMBINE_FUNCTION_BLEND_LOCAL, // (1-f)*local
                 GR_COMBINE_FACTOR_DETAIL_FACTOR,
                 FXTRUE,                          // invert to give f*local
                 FXFALSE);

*/


/*
lambda = log base 2 of rho(x,y).

rho(x,y) = max { sqrt( (du/dx)^2 + (dv/dx)^2 ), sqrt( (du/dy)^2 + (dv/dy)^2 ) }.

From the results at iXBT, it looks like ATI uses a linear approximation to log base 2. (natural log of x = ln x = 1 + x + x^2 / 2 + x^3 / 3 + ..., so 1 + x would be a linear approximation to ln x. For log base 2, just take ln x and divide by ln 2).


A correct formula is:
ln (1+x) = x - x^2 / 2 + x^3 / 3 - x^4 / 4 + ...

So, to first order, one can approximate the log by simply using ln(1+x) = x, or ln(x) = (x-1)/x.

As a side note, if you better-approximate the log, it can become easier to properly do the calculation:
sqrt(x^2 + y^2) that is necessary for LOD calculations, as the square root becomes trivial (a simple divide by two after the log).



lod = log2(max(sqrt((dsdx*w)^2 + (dtdx*w)^2), sqrt((dsdy*w)^2 + (dtdy*w)^2)))
lod = log2(sqrt(w^2 * max(dsdx^2 + dtdx^2, dsdy^2 + dvdy^2)))
lod = log2(w * sqrt(max(dsdx^2 + dtdx^2, dsdy^2 + dvdy^2)))
lod = low2(w) + 0.5 * log2(w^2 * max(dsdx^2 + dtdx^2, dsdy^2 + dvdy^2))


*/

#ifndef ONESHOTS
#define ONESHOTS

/*************************************
 *
 *  float to custom 16-bit float converter
 *
 *************************************/

#define float_to_depth(wval,result) 							\
do {															\
	if ((wval) >= 1.0)											\
		(result) = 0x0000;										\
	else if ((wval) < 0.0)										\
		(result) = 0xffff;										\
	else														\
	{															\
		INT32 ival = (INT32)((wval) * (float)(1 << 28));		\
		if ((ival & 0xffff000) == 0)							\
			(result) = 0xffff;									\
		else													\
		{														\
			INT32 ex = 0;										\
			while (!(ival & 0x8000000)) ival <<= 1, ex++;		\
			(result) = ((ex << 12) | ((~ival >> 15) & 0xfff)) + 1;\
		}														\
	}															\
} while (0)														\

#define BITFIELD(fix,fixmask,var,shift,mask) \
	(((((fixmask) >> (shift)) & (mask)) == (mask)) ? (((fix) >> (shift)) & (mask)) : (((var) >> (shift)) & (mask)))
#define FBZCOLORPATH_BITS(start,len) \
	BITFIELD(FBZCOLORPATH, FBZCOLORPATH_MASK, voodoo_regs[fbzColorPath], start, (1 << (len)) - 1)
#define FOGMODE_BITS(start,len) \
	((voodoo_regs[fogMode] >> (start)) & ((1 << (len)) - 1))
#define ALPHAMODE_BITS(start,len) \
	BITFIELD(ALPHAMODE, ALPHAMODE_MASK, voodoo_regs[alphaMode], start, (1 << (len)) - 1)
#define FBZMODE_BITS(start,len) \
	BITFIELD(FBZMODE, FBZMODE_MASK, voodoo_regs[fbzMode], start, (1 << (len)) - 1)
#define TEXTUREMODE0_BITS(start,len) \
	BITFIELD(TEXTUREMODE0, TEXTUREMODE0_MASK, voodoo_regs[0x100 + textureMode], start, (1 << (len)) - 1)
#define TEXTUREMODE1_BITS(start,len) \
	BITFIELD(TEXTUREMODE1, TEXTUREMODE1_MASK, voodoo_regs[0x200 + textureMode], start, (1 << (len)) - 1)

#define NEEDS_TEX1		(NUM_TMUS > 1) /* && (TEXTUREMODE0_BITS(12,1) == 0 || TEXTUREMODE0_BITS(21,1) == 0)) */

#if 0


#define TEXTURE_PIPELINE()															\
do {																				\
	INT32 fs = curs1;
	INT32 ft = curt1;
	INT32 lod = lodbase1;

	/* perspective correct */
	if (TEXTUREMODE1_BITS(0,1))
	{
		float invw1 = 1.0f / curw1;
		fs *= invw1;
		ft *= invw1;
		lod += lod_lookup[f2u(invw1) >> 16];
	}

	/* clamp LOD */
	lod += trex_lodbias[1];
	if (TEXTUREMODE1_BITS(4,1))
		lod += lod_dither_matrix[(x & 3) | ((y & 3) << 2)];
	if (lod < trex_lodmin[1]) lod = trex_lodmin[1];
	if (lod > trex_lodmax[1]) lod = trex_lodmax[1];

	/* compute texture base */
	texturebase = trex_lod_start[1][lod >> 8];
	lodshift = trex_lod_width_shift[1][lod >> 8];

	/* point-sampled filter */
	if ((TEXTUREMODE1_BITS(1,2) == 0) ||
		(lod == trex_lodmin[1] && !TEXTUREMODE1_BITS(2,1)) ||
		(lod != trex_lodmin[1] && !TEXTUREMODE1_BITS(1,1)))
	{
		/* convert to int */
		INT32 s = TRUNC_TO_INT(fs);
		INT32 t = TRUNC_TO_INT(ft);
		int ilod = lod >> 8;

		/* clamp W */
		if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
			s = t = 0;

		/* clamp S */
		if (TEXTUREMODE1_BITS(6,1))
		{
			if (s < 0) s = 0;
			else if (s >= trex_width[1]) s = trex_width[1] - 1;
		}
		else
			s &= trex_width[1] - 1;
		s >>= ilod;

		/* clamp T */
		if (TEXTUREMODE1_BITS(7,1))
		{
			if (t < 0) t = 0;
			else if (t >= trex_height[1]) t = trex_height[1] - 1;
		}
		else
			t &= trex_height[1] - 1;
		t >>= ilod;

		/* fetch raw texel data */
		if (!TEXTUREMODE1_BITS(11,1))
			texel = *((UINT8 *)texturebase + (t << lodshift) + s);
		else
			texel = *((UINT16 *)texturebase + (t << lodshift) + s);

		/* convert to ARGB */
		c_local = lookup1[texel];
	}

	/* bilinear filter */
	else
	{
		INT32 ts0, tt0, ts1, tt1;
		UINT32 factor, factorsum, ag, rb;

		/* convert to int */
		INT32 s, t;
		int ilod = lod >> 8;
		s = TRUNC_TO_INT(fs * 256.0f) - (128 << ilod);
		t = TRUNC_TO_INT(ft * 256.0f) - (128 << ilod);

		/* clamp W */
		if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
			s = t = 0;

		/* clamp S0 */
		ts0 = s >> 8;
		if (TEXTUREMODE1_BITS(6,1))
		{
			if (ts0 < 0) ts0 = 0;
			else if (ts0 >= trex_width[1]) ts0 = trex_width[1] - 1;
		}
		else
			ts0 &= trex_width[1] - 1;
		ts0 >>= ilod;

		/* clamp S1 */
		ts1 = (s >> 8) + (1 << ilod);
		if (TEXTUREMODE1_BITS(6,1))
		{
			if (ts1 < 0) ts1 = 0;
			else if (ts1 >= trex_width[1]) ts1 = trex_width[1] - 1;
		}
		else
			ts1 &= trex_width[1] - 1;
		ts1 >>= ilod;

		/* clamp T0 */
		tt0 = t >> 8;
		if (TEXTUREMODE1_BITS(7,1))
		{
			if (tt0 < 0) tt0 = 0;
			else if (tt0 >= trex_height[1]) tt0 = trex_height[1] - 1;
		}
		else
			tt0 &= trex_height[1] - 1;
		tt0 >>= ilod;

		/* clamp T1 */
		tt1 = (t >> 8) + (1 << ilod);
		if (TEXTUREMODE1_BITS(7,1))
		{
			if (tt1 < 0) tt1 = 0;
			else if (tt1 >= trex_height[1]) tt1 = trex_height[1] - 1;
		}
		else
			tt1 &= trex_height[1] - 1;
		tt1 >>= ilod;

		s >>= ilod;
		t >>= ilod;

		/* texel 0 */
		factorsum = factor = ((0x100 - (s & 0xff)) * (0x100 - (t & 0xff))) >> 8;

		/* fetch raw texel data */
		if (!TEXTUREMODE1_BITS(11,1))
			texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts0);
		else
			texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts0);

		/* convert to ARGB */
		texel = lookup1[texel];
		ag = ((texel >> 8) & 0x00ff00ff) * factor;
		rb = (texel & 0x00ff00ff) * factor;

		/* texel 1 */
		factorsum += factor = ((s & 0xff) * (0x100 - (t & 0xff))) >> 8;
		if (factor)
		{
			/* fetch raw texel data */
			if (!TEXTUREMODE1_BITS(11,1))
				texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts1);
			else
				texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts1);

			/* convert to ARGB */
			texel = lookup1[texel];
			ag += ((texel >> 8) & 0x00ff00ff) * factor;
			rb += (texel & 0x00ff00ff) * factor;
		}

		/* texel 2 */
		factorsum += factor = ((0x100 - (s & 0xff)) * (t & 0xff)) >> 8;
		if (factor)
		{
			/* fetch raw texel data */
			if (!TEXTUREMODE1_BITS(11,1))
				texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts0);
			else
				texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts0);

			/* convert to ARGB */
			texel = lookup1[texel];
			ag += ((texel >> 8) & 0x00ff00ff) * factor;
			rb += (texel & 0x00ff00ff) * factor;
		}

		/* texel 3 */
		factor = 0x100 - factorsum;
		if (factor)
		{
			/* fetch raw texel data */
			if (!TEXTUREMODE1_BITS(11,1))
				texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts1);
			else
				texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts1);

			/* convert to ARGB */
			texel = lookup1[texel];
			ag += ((texel >> 8) & 0x00ff00ff) * factor;
			rb += (texel & 0x00ff00ff) * factor;
		}

		/* this becomes the local color */
		c_local = (ag & 0xff00ff00) | ((rb >> 8) & 0x00ff00ff);
	}

	/* zero/other selection */
	if (!TEXTUREMODE1_BITS(12,1))				/* tc_zero_other */
	{
		tr = (c_other >> 16) & 0xff;
		tg = (c_other >> 8) & 0xff;
		tb = (c_other >> 0) & 0xff;
	}
	else
		tr = tg = tb = 0;

	/* subtract local color */
	if (TEXTUREMODE1_BITS(13,1))				/* tc_sub_clocal */
	{
		tr -= (c_local >> 16) & 0xff;
		tg -= (c_local >> 8) & 0xff;
		tb -= (c_local >> 0) & 0xff;
	}

	/* scale RGB */
	if (TEXTUREMODE1_BITS(14,3) != 0)			/* tc_mselect mux */
	{
		INT32 rm = 0, gm = 0, bm = 0;

		switch (TEXTUREMODE1_BITS(14,3))
		{
			case 1:		/* tc_mselect mux == c_local */
				rm = (c_local >> 16) & 0xff;
				gm = (c_local >> 8) & 0xff;
				bm = (c_local >> 0) & 0xff;
				break;
			case 2:		/* tc_mselect mux == a_other */
				rm = gm = bm = c_other >> 24;
				break;
			case 3:		/* tc_mselect mux == a_local */
				rm = gm = bm = c_local >> 24;
				break;
			case 4:		/* tc_mselect mux == LOD */
				printf("Detail tex\n");
				rm = gm = bm = (UINT8)(lod >> 3);
				break;
			case 5:		/* tc_mselect mux == LOD frac */
				printf("Trilinear tex\n");
				rm = gm = bm = (UINT8)lod;
				break;
		}

		if (TEXTUREMODE1_BITS(17,1))			/* tc_reverse_blend */
		{
			tr = (tr * (rm + 1)) >> 8;
			tg = (tg * (gm + 1)) >> 8;
			tb = (tb * (bm + 1)) >> 8;
		}
		else
		{
			tr = (tr * ((rm ^ 0xff) + 1)) >> 8;
			tg = (tg * ((gm ^ 0xff) + 1)) >> 8;
			tb = (tb * ((bm ^ 0xff) + 1)) >> 8;
		}
	}

	/* add local color */
	if (TEXTUREMODE1_BITS(18,1))				/* tc_add_clocal */
	{
		tr += (c_local >> 16) & 0xff;
		tg += (c_local >> 8) & 0xff;
		tb += (c_local >> 0) & 0xff;
	}

	/* add local alpha */
	else if (TEXTUREMODE1_BITS(19,1))			/* tc_add_alocal */
	{
		tr += c_local >> 24;
		tg += c_local >> 24;
		tb += c_local >> 24;
	}

	/* zero/other selection */
	if (!TEXTUREMODE1_BITS(21,1))				/* tca_zero_other */
		ta = (c_other >> 24) & 0xff;
	else
		ta = 0;

	/* subtract local alpha */
	if (TEXTUREMODE1_BITS(22,1))				/* tca_sub_clocal */
		ta -= (c_local >> 24) & 0xff;

	/* scale alpha */
	if (TEXTUREMODE1_BITS(23,3) != 0)			/* tca_mselect mux */
	{
		INT32 am = 0;

		switch (TEXTUREMODE1_BITS(23,3))
		{
			case 1:		/* tca_mselect mux == c_local */
				am = (c_local >> 24) & 0xff;
				break;
			case 2:		/* tca_mselect mux == a_other */
				am = c_other >> 24;
				break;
			case 3:		/* tca_mselect mux == a_local */
				am = c_local >> 24;
				break;
			case 4:		/* tca_mselect mux == LOD */
				am = (UINT8)(lod >> 3);
				break;
			case 5:		/* tca_mselect mux == LOD frac */
				am = (UINT8)lod;
				break;
		}

		if (TEXTUREMODE1_BITS(26,1))			/* tca_reverse_blend */
			ta = (ta * (am + 1)) >> 8;
		else
			ta = (ta * ((am ^ 0xff) + 1)) >> 8;
	}

	/* add local color */
	if (TEXTUREMODE1_BITS(27,1) ||
		TEXTUREMODE1_BITS(28,1))				/* tca_add_clocal/tca_add_alocal */
		ta += (c_local >> 24) & 0xff;

	/* clamp */
	if (tr < 0) tr = 0;
	else if (tr > 255) tr = 255;
	if (tg < 0) tg = 0;
	else if (tg > 255) tg = 255;
	if (tb < 0) tb = 0;
	else if (tb > 255) tb = 255;
	if (ta < 0) ta = 0;
	else if (ta > 255) ta = 255;
	c_other = (ta << 24) | (tr << 16) | (tg << 8) | tb;

	/* invert */
	if (TEXTUREMODE1_BITS(20,1))
		c_other ^= 0x00ffffff;
	if (TEXTUREMODE1_BITS(29,1))
		c_other ^= 0xff000000;
} while (0)
#endif

#endif


void RENDERFUNC(void)
{
#if (PER_PIXEL_LOD)
	float tex0x = tri_ds0dx * tri_ds0dx + tri_dt0dx * tri_dt0dx;
	float tex0y = tri_ds0dy * tri_ds0dy + tri_dt0dy * tri_dt0dy;
	INT16 lodbase0 = ((tex0x > tex0y) ? lod_lookup[f2u(tex0x) >> 16] : lod_lookup[f2u(tex0y) >> 16]) / 2;
#if (NUM_TMUS > 1)
	float tex1x = tri_ds1dx * tri_ds1dx + tri_dt1dx * tri_dt1dx;
	float tex1y = tri_ds1dy * tri_ds1dy + tri_dt1dy * tri_dt1dy;
	INT16 lodbase1 = ((tex1x > tex1y) ? lod_lookup[f2u(tex1x) >> 16] : lod_lookup[f2u(tex1y) >> 16]) / 2;
#endif
#endif

	UINT16 *buffer = *fbz_draw_buffer;
	const UINT32 *lookup0 = NULL;
#if (NUM_TMUS > 1)
	const UINT32 *lookup1 = NULL;
#endif
	int x, y;
	struct tri_vertex *vmin, *vmid, *vmax;
	float dxdy_minmid, dxdy_minmax, dxdy_midmax;
	int starty, stopy;
	float fptemp;
#if DEBUG_LOD
	int lodhit = 0;
#endif

#if (0)
	if (FBZMODE_BITS(4,1) || FBZMODE_BITS(10,1))
	{
		static const char *funcs[] = { "never", "lt", "eq", "le", "gt", "ne", "ge", "always" };
		if (!FBZMODE_BITS(20,1))
		{
			if (!FBZMODE_BITS(3,1))
				logerror("Depth Z: %c%c %s %08X,%08X,%08X -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startz,
					tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx),
					tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx),
					(UINT16)(tri_startz >> 12),
					(UINT16)(tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx)) >> 12,
					(UINT16)(tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx)) >> 12);
			else if (!FBZMODE_BITS(21,1))
				logerror("Depth Wf: %c%c %s %f,%f,%f -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startw,
					tri_startw + (INT32)((tri_vb.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dwdx),
					tri_startw + (INT32)((tri_vc.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dwdx),
					float_to_depth(tri_startw),
					float_to_depth(tri_startw + (INT32)((tri_vb.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dwdx)),
					float_to_depth(tri_startw + (INT32)((tri_vc.y - tri_va.y) * tri_dwdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dwdx)));
			else
				logerror("Depth Wz: %c%c %s %08X,%08X,%08X -> %04X,%04X,%04X", FBZMODE_BITS(4,1) ? 'T' : ' ', FBZMODE_BITS(10,1) ? 'W' : ' ', funcs[FBZMODE_BITS(5,3)],
					tri_startz,
					tri_startz + (INT32)((tri_vb.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * (float)tri_dzdx),
					tri_startz + (INT32)((tri_vc.y - tri_va.y) * (float)tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * (float)tri_dzdx),
					float_to_depth((float)(tri_startz) * (1.0 / 4096.0)),
					float_to_depth((float)(tri_startz + (INT32)((tri_vb.y - tri_va.y) * tri_dzdy) + (INT32)((tri_vb.x - tri_va.x) * tri_dzdx)) * (1.0 / 4096.0)),
					float_to_depth((float)(tri_startz + (INT32)((tri_vc.y - tri_va.y) * tri_dzdy) + (INT32)((tri_vc.x - tri_va.x) * tri_dzdx)) * (1.0 / 4096.0)));

			if (FBZMODE_BITS(16,1))
				logerror(" + %04X\n", (UINT16)voodoo_regs[zaColor]);
		}
		else
			logerror("Depth const: %04X\n", (UINT16)voodoo_regs[zaColor]);
	}
#endif

	/* sort the verticies */
	vmin = &tri_va;
	vmid = &tri_vb;
	vmax = &tri_vc;
	if (vmid->y < vmin->y) { struct tri_vertex *temp = vmid; vmid = vmin; vmin = temp; }
	if (vmax->y < vmin->y) { struct tri_vertex *temp = vmax; vmax = vmin; vmin = temp; }
	if (vmax->y < vmid->y) { struct tri_vertex *temp = vmax; vmax = vmid; vmid = temp; }

	/* compute the clipped start/end y */
	starty = TRUNC_TO_INT(vmin->y + 0.5f);
	stopy = TRUNC_TO_INT(vmax->y + 0.5f);
	if (starty < fbz_cliprect->min_y)
		starty = fbz_cliprect->min_y;
	if (stopy > fbz_cliprect->max_y)
		stopy = fbz_cliprect->max_y;
	if (starty >= stopy)
		return;

	/* compute the slopes */
	fptemp = vmid->y - vmin->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_minmid = (vmid->x - vmin->x) / fptemp;
	fptemp = vmax->y - vmin->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_minmax = (vmax->x - vmin->x) / fptemp;
	fptemp = vmax->y - vmid->y;
	if (fptemp == 0.0f) fptemp = 1.0f;
	dxdy_midmax = (vmax->x - vmid->x) / fptemp;

	/* setup texture */
	if (FBZCOLORPATH_BITS(27,1))
	{
		int t;

		/* determine the lookup */
		t = TEXTUREMODE0_BITS(8,4);
		if ((t & 7) == 1 && TEXTUREMODE0_BITS(5,1))
			t += 6;

		/* handle dirty tables */
		if (texel_lookup_dirty[0][t])
		{
			(*update_texel_lookup[t])(0);
			texel_lookup_dirty[0][t] = 0;
		}
		lookup0 = &texel_lookup[0][t][0];

#if (NUM_TMUS > 1)
		/* determine the lookup */
		if (NEEDS_TEX1 && tmus > 1)
		{
			t = TEXTUREMODE1_BITS(8,4);
			if ((t & 7) == 1 && TEXTUREMODE1_BITS(5,1))
				t += 6;

			/* handle dirty tables */
			if (texel_lookup_dirty[1][t])
			{
				(*update_texel_lookup[t])(1);
				texel_lookup_dirty[1][t] = 0;
			}
			lookup1 = &texel_lookup[1][t][0];
		}
#endif
	}

	/* do the render */
	for (y = starty; y < stopy; y++)
	{
		int effy = FBZMODE_BITS(17,1) ? (inverted_yorigin - y) : y;
		if (effy >= 0 && effy < FRAMEBUF_HEIGHT)
		{
			const UINT8 *dither_matrix = (!FBZMODE_BITS(11,1)) ? &dither_matrix_4x4[(effy & 3) << 2] : &dither_matrix_2x2[(effy & 3) << 2];
			UINT16 *dest = &buffer[effy * FRAMEBUF_WIDTH];
			UINT16 *depth = &depthbuf[effy * FRAMEBUF_WIDTH];
			INT32 dx, dy;
			float fdx, fdy;
			INT32 curr, curg, curb, cura, curz;
			float curw;
			float curs0, curt0, curw0, invw0;
#if (NUM_TMUS > 1)
			float curs1, curt1, curw1, invw1;
#endif
			int startx, stopx;
			float fpy;

#if (OPTIMIZATIONS_ENABLED)
			if (cheating_allowed && (effy & resolution_mask))
				continue;
#endif

			/* compute X endpoints */
			fpy = (float)y + 0.5f;
			startx = TRUNC_TO_INT((fpy - vmin->y) * dxdy_minmax + vmin->x + 0.5f);
			if (fpy < vmid->y)
				stopx = TRUNC_TO_INT((fpy - vmin->y) * dxdy_minmid + vmin->x + 0.5f);
			else
				stopx = TRUNC_TO_INT((fpy - vmid->y) * dxdy_midmax + vmid->x + 0.5f);
			if (startx > stopx) { int temp = startx; startx = stopx; stopx = temp; }
			if (startx < fbz_cliprect->min_x) startx = fbz_cliprect->min_x;
			if (stopx > fbz_cliprect->max_x) stopx = fbz_cliprect->max_x;
			if (startx >= stopx)
				continue;

			/* compute parameters */
			fdx = (float)startx + 0.5f - tri_va.x;
			fdy = (float)y + 0.5f - tri_va.y;
			dx = TRUNC_TO_INT(fdx * 16.0f + 0.5f);
			dy = TRUNC_TO_INT(fdy * 16.0f + 0.5f);
			curr = tri_startr + ((dy * tri_drdy + dx * tri_drdx) >> 4);
			curg = tri_startg + ((dy * tri_dgdy + dx * tri_dgdx) >> 4);
			curb = tri_startb + ((dy * tri_dbdy + dx * tri_dbdx) >> 4);
			cura = tri_starta + ((dy * tri_dady + dx * tri_dadx) >> 4);
			curz = tri_startz + ((dy * tri_dzdy + dx * tri_dzdx) >> 4);
			curw = tri_startw + fdy * tri_dwdy + fdx * tri_dwdx;
			curs0 = tri_starts0 + fdy * tri_ds0dy + fdx * tri_ds0dx;
			curt0 = tri_startt0 + fdy * tri_dt0dy + fdx * tri_dt0dx;
			curw0 = tri_startw0 + fdy * tri_dw0dy + fdx * tri_dw0dx;
#if (NUM_TMUS > 1)
			curs1 = tri_starts1 + fdy * tri_ds1dy + fdx * tri_ds1dx;
			curt1 = tri_startt1 + fdy * tri_dt1dy + fdx * tri_dt1dx;
			curw1 = tri_startw1 + fdy * tri_dw1dy + fdx * tri_dw1dx;
#endif

			/* loop over X */
			voodoo_regs[fbiPixelsIn] += stopx - startx;
			ADD_TO_PIXEL_COUNT(stopx - startx);
			for (x = startx; x < stopx; x++)
			{
				INT32 r = 0, g = 0, b = 0, a = 0, depthval;
				UINT32 texel = 0, c_local = 0;

#if (OPTIMIZATIONS_ENABLED)
				if (cheating_allowed && (x & resolution_mask))
					goto skipdrawdepth;
#endif
				/* rotate stipple pattern */
				if (!FBZMODE_BITS(12,1))
					voodoo_regs[stipple] = (voodoo_regs[stipple] << 1) | (voodoo_regs[stipple] >> 31);

				/* handle stippling */
				if (FBZMODE_BITS(2,1))
				{
					/* rotate mode */
					if (!FBZMODE_BITS(12,1))
					{
						if ((voodoo_regs[stipple] & 0x80000000) == 0)
							goto skipdrawdepth;
					}

					/* pattern mode */
					else
					{
						int stipple_index = ((y & 3) << 3) | (~x & 7);
						if ((voodoo_regs[stipple] & (1 << stipple_index)) == 0)
							goto skipdrawdepth;
					}
				}

				/* compute depth value (W or Z) for this pixel */
				if (!FBZMODE_BITS(3,1))
				{
					depthval = curz >> 12;
					if (depthval >= 0xffff)
						depthval = 0xffff;
					else if (depthval < 0)
						depthval = 0;
				}
				else if (!FBZMODE_BITS(21,1))
					float_to_depth(curw, depthval);
				else
					float_to_depth((float)curz * (1.0 / 4096.0), depthval);

				/* handle depth buffer testing */
				if (FBZMODE_BITS(4,1))
				{
					INT32 depthsource;

					/* the source depth is either the iterated W/Z+bias or a constant value */
					if (!FBZMODE_BITS(20,1))
					{
						depthsource = depthval;

						/* add the bias */
						if (FBZMODE_BITS(16,1))
						{
							depthsource += (INT16)voodoo_regs[zaColor];

							if (depthsource >= 0xffff)
								depthsource = 0xffff;
							else if (depthsource < 0)
								depthsource = 0;
						}
					}
					else
						depthsource = voodoo_regs[zaColor] & 0xffff;

					/* test against the depth buffer */
					switch (FBZMODE_BITS(5,3))
					{
						case 0:
							goto skipdrawdepth;
						case 1:
							if (depthsource >= depth[x])
								goto skipdrawdepth;
							break;
						case 2:
							if (depthsource != depth[x])
								goto skipdrawdepth;
							break;
						case 3:
							if (depthsource > depth[x])
								goto skipdrawdepth;
							break;
						case 4:
							if (depthsource <= depth[x])
								goto skipdrawdepth;
							break;
						case 5:
							if (depthsource == depth[x])
								goto skipdrawdepth;
							break;
						case 6:
							if (depthsource < depth[x])
								goto skipdrawdepth;
							break;
						case 7:
							break;
					}
				}

				/* load the texel if necessary */
				if (FBZCOLORPATH_BITS(0,2) == 1 || FBZCOLORPATH_BITS(2,2) == 1)
				{
					UINT32 c_other = 0;
					INT32 tr, tg, tb, ta;
					UINT8 *texturebase;
					float fs, ft;
					UINT8 lodshift;
					INT32 lod;

#if (NUM_TMUS > 1)
invw1 = 0;
					/* import from TMU 1 if needed */
					if (NEEDS_TEX1 && tmus > 1)
					{
						/* perspective correct */
						fs = curs1;
						ft = curt1;
						lod = lodbase1;
						if (TEXTUREMODE1_BITS(0,1))
						{
							invw1 = 1.0f / curw1;
							fs *= invw1;
							ft *= invw1;
#if (PER_PIXEL_LOD)
							lod += lod_lookup[f2u(invw1) >> 16];
#endif
						}
#if (PER_PIXEL_LOD)
						/* clamp LOD */
						lod += trex_lodbias[1];
						if (TEXTUREMODE1_BITS(4,1))
							lod += lod_dither_matrix[(x & 3) | ((y & 3) << 2)];
						if (lod < trex_lodmin[1]) lod = trex_lodmin[1];
						if (lod > trex_lodmax[1]) lod = trex_lodmax[1];
#else
						lod = trex_lodmin[1];
#endif
						/* compute texture base */
						texturebase = trex_lod_start[1][lod >> 8];
						lodshift = trex_lod_width_shift[1][lod >> 8];

						/* point-sampled filter */
#if (BILINEAR_FILTER)
						if ((TEXTUREMODE1_BITS(1,2) == 0) ||
							(lod == trex_lodmin[1] && !TEXTUREMODE1_BITS(2,1)) ||
							(lod != trex_lodmin[1] && !TEXTUREMODE1_BITS(1,1)))
#endif
						{
							/* convert to int */
							INT32 s = TRUNC_TO_INT(fs);
							INT32 t = TRUNC_TO_INT(ft);
							int ilod = lod >> 8;

							/* clamp W */
							if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
								s = t = 0;

							/* clamp S */
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (s < 0) s = 0;
								else if (s >= trex_width[1]) s = trex_width[1] - 1;
							}
							else
								s &= trex_width[1] - 1;
							s >>= ilod;

							/* clamp T */
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (t < 0) t = 0;
								else if (t >= trex_height[1]) t = trex_height[1] - 1;
							}
							else
								t &= trex_height[1] - 1;
							t >>= ilod;

							/* fetch raw texel data */
							if (!TEXTUREMODE1_BITS(11,1))
								texel = *((UINT8 *)texturebase + (t << lodshift) + s);
							else
								texel = *((UINT16 *)texturebase + (t << lodshift) + s);

							/* convert to ARGB */
							c_local = lookup1[texel];
						}

#if (BILINEAR_FILTER)
						/* bilinear filter */
						else
						{
							INT32 ts0, tt0, ts1, tt1;
							UINT32 factor, factorsum, ag, rb;

							/* convert to int */
							INT32 s, t;
							int ilod = lod >> 8;
							s = TRUNC_TO_INT(fs * 256.0f) - (128 << ilod);
							t = TRUNC_TO_INT(ft * 256.0f) - (128 << ilod);

							/* clamp W */
							if (TEXTUREMODE1_BITS(3,1) && curw1 < 0.0f)
								s = t = 0;

							/* clamp S0 */
							ts0 = s >> 8;
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (ts0 < 0) ts0 = 0;
								else if (ts0 >= trex_width[1]) ts0 = trex_width[1] - 1;
							}
							else
								ts0 &= trex_width[1] - 1;
							ts0 >>= ilod;

							/* clamp S1 */
							ts1 = (s >> 8) + (1 << ilod);
							if (TEXTUREMODE1_BITS(6,1))
							{
								if (ts1 < 0) ts1 = 0;
								else if (ts1 >= trex_width[1]) ts1 = trex_width[1] - 1;
							}
							else
								ts1 &= trex_width[1] - 1;
							ts1 >>= ilod;

							/* clamp T0 */
							tt0 = t >> 8;
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (tt0 < 0) tt0 = 0;
								else if (tt0 >= trex_height[1]) tt0 = trex_height[1] - 1;
							}
							else
								tt0 &= trex_height[1] - 1;
							tt0 >>= ilod;

							/* clamp T1 */
							tt1 = (t >> 8) + (1 << ilod);
							if (TEXTUREMODE1_BITS(7,1))
							{
								if (tt1 < 0) tt1 = 0;
								else if (tt1 >= trex_height[1]) tt1 = trex_height[1] - 1;
							}
							else
								tt1 &= trex_height[1] - 1;
							tt1 >>= ilod;

							s >>= ilod;
							t >>= ilod;

							/* texel 0 */
							factorsum = factor = ((0x100 - (s & 0xff)) * (0x100 - (t & 0xff))) >> 8;

							/* fetch raw texel data */
							if (!TEXTUREMODE1_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts0);
							else
								texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts0);

							/* convert to ARGB */
							texel = lookup1[texel];
							ag = ((texel >> 8) & 0x00ff00ff) * factor;
							rb = (texel & 0x00ff00ff) * factor;

							/* texel 1 */
							factorsum += factor = ((s & 0xff) * (0x100 - (t & 0xff))) >> 8;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts1);
								else
									texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts1);

								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}

							/* texel 2 */
							factorsum += factor = ((0x100 - (s & 0xff)) * (t & 0xff)) >> 8;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts0);
								else
									texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts0);

								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}

							/* texel 3 */
							factor = 0x100 - factorsum;
							if (factor)
							{
								/* fetch raw texel data */
								if (!TEXTUREMODE1_BITS(11,1))
									texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts1);
								else
									texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts1);

								/* convert to ARGB */
								texel = lookup1[texel];
								ag += ((texel >> 8) & 0x00ff00ff) * factor;
								rb += (texel & 0x00ff00ff) * factor;
							}

							/* this becomes the local color */
							c_local = (ag & 0xff00ff00) | ((rb >> 8) & 0x00ff00ff);
						}
#endif
						/* zero/other selection */
						if (!TEXTUREMODE1_BITS(12,1))				/* tc_zero_other */
						{
							tr = (c_other >> 16) & 0xff;
							tg = (c_other >> 8) & 0xff;
							tb = (c_other >> 0) & 0xff;
						}
						else
							tr = tg = tb = 0;

						/* subtract local color */
						if (TEXTUREMODE1_BITS(13,1))				/* tc_sub_clocal */
						{
							tr -= (c_local >> 16) & 0xff;
							tg -= (c_local >> 8) & 0xff;
							tb -= (c_local >> 0) & 0xff;
						}

						/* scale RGB */
						if (TEXTUREMODE1_BITS(14,3) != 0)			/* tc_mselect mux */
						{
							INT32 rm = 0, gm = 0, bm = 0;

							switch (TEXTUREMODE1_BITS(14,3))
							{
								case 1:		/* tc_mselect mux == c_local */
									rm = (c_local >> 16) & 0xff;
									gm = (c_local >> 8) & 0xff;
									bm = (c_local >> 0) & 0xff;
									break;
								case 2:		/* tc_mselect mux == a_other */
									rm = gm = bm = c_other >> 24;
									break;
								case 3:		/* tc_mselect mux == a_local */
									rm = gm = bm = c_local >> 24;
									break;
								case 4:		/* tc_mselect mux == LOD */
									printf("Detail tex\n");
									rm = gm = bm = (UINT8)(lod >> 3);
									break;
								case 5:		/* tc_mselect mux == LOD frac */
									printf("Trilinear tex\n");
									rm = gm = bm = (UINT8)lod;
									break;
							}

							if (TEXTUREMODE1_BITS(17,1))			/* tc_reverse_blend */
							{
								tr = (tr * (rm + 1)) >> 8;
								tg = (tg * (gm + 1)) >> 8;
								tb = (tb * (bm + 1)) >> 8;
							}
							else
							{
								tr = (tr * ((rm ^ 0xff) + 1)) >> 8;
								tg = (tg * ((gm ^ 0xff) + 1)) >> 8;
								tb = (tb * ((bm ^ 0xff) + 1)) >> 8;
							}
						}

						/* add local color */
						if (TEXTUREMODE1_BITS(18,1))				/* tc_add_clocal */
						{
							tr += (c_local >> 16) & 0xff;
							tg += (c_local >> 8) & 0xff;
							tb += (c_local >> 0) & 0xff;
						}

						/* add local alpha */
						else if (TEXTUREMODE1_BITS(19,1))			/* tc_add_alocal */
						{
							tr += c_local >> 24;
							tg += c_local >> 24;
							tb += c_local >> 24;
						}

						/* zero/other selection */
						if (!TEXTUREMODE1_BITS(21,1))				/* tca_zero_other */
							ta = (c_other >> 24) & 0xff;
						else
							ta = 0;

						/* subtract local alpha */
						if (TEXTUREMODE1_BITS(22,1))				/* tca_sub_clocal */
							ta -= (c_local >> 24) & 0xff;

						/* scale alpha */
						if (TEXTUREMODE1_BITS(23,3) != 0)			/* tca_mselect mux */
						{
							INT32 am = 0;

							switch (TEXTUREMODE1_BITS(23,3))
							{
								case 1:		/* tca_mselect mux == c_local */
									am = (c_local >> 24) & 0xff;
									break;
								case 2:		/* tca_mselect mux == a_other */
									am = c_other >> 24;
									break;
								case 3:		/* tca_mselect mux == a_local */
									am = c_local >> 24;
									break;
								case 4:		/* tca_mselect mux == LOD */
									am = (UINT8)(lod >> 3);
									break;
								case 5:		/* tca_mselect mux == LOD frac */
									am = (UINT8)lod;
									break;
							}

							if (TEXTUREMODE1_BITS(26,1))			/* tca_reverse_blend */
								ta = (ta * (am + 1)) >> 8;
							else
								ta = (ta * ((am ^ 0xff) + 1)) >> 8;
						}

						/* add local color */
						if (TEXTUREMODE1_BITS(27,1) ||
							TEXTUREMODE1_BITS(28,1))				/* tca_add_clocal/tca_add_alocal */
							ta += (c_local >> 24) & 0xff;

						/* clamp */
						if (tr < 0) tr = 0;
						else if (tr > 255) tr = 255;
						if (tg < 0) tg = 0;
						else if (tg > 255) tg = 255;
						if (tb < 0) tb = 0;
						else if (tb > 255) tb = 255;
						if (ta < 0) ta = 0;
						else if (ta > 255) ta = 255;
						c_other = (ta << 24) | (tr << 16) | (tg << 8) | tb;

						/* invert */
						if (TEXTUREMODE1_BITS(20,1))
							c_other ^= 0x00ffffff;
						if (TEXTUREMODE1_BITS(29,1))
							c_other ^= 0xff000000;
					}
#endif
					/* perspective correct */
					fs = curs0;
					ft = curt0;
					lod = lodbase0;
					if (TEXTUREMODE0_BITS(0,1))
					{
						invw0 = 1.0f / curw0;
						fs *= invw0;
						ft *= invw0;
#if (PER_PIXEL_LOD)
						lod += lod_lookup[f2u(invw0) >> 16];
#endif
					}
#if (PER_PIXEL_LOD)
					/* clamp LOD */
					lod += trex_lodbias[0];
					if (TEXTUREMODE0_BITS(4,1))
						lod += lod_dither_matrix[(x & 3) | ((y & 3) << 2)];
					if (lod < trex_lodmin[0]) lod = trex_lodmin[0];
					if (lod > trex_lodmax[0]) lod = trex_lodmax[0];
#if DEBUG_LOD
					lodhit = (lod >> 8);
#endif
#else
					lod = trex_lodmin[0];
#endif
					/* compute texture base */
					texturebase = trex_lod_start[0][lod >> 8];
					lodshift = trex_lod_width_shift[0][lod >> 8];

#if (BILINEAR_FILTER)
					/* point-sampled filter */
					if ((TEXTUREMODE0_BITS(1,2) == 0) ||
						(lod == trex_lodmin[0] && !TEXTUREMODE0_BITS(2,1)) ||
						(lod != trex_lodmin[0] && !TEXTUREMODE0_BITS(1,1)))
#endif
					{
						/* convert to int */
						INT32 s = TRUNC_TO_INT(fs);
						INT32 t = TRUNC_TO_INT(ft);
						int ilod = lod >> 8;

						/* clamp W */
						if (TEXTUREMODE0_BITS(3,1) && curw0 < 0.0f)
							s = t = 0;

						/* clamp S */
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (s < 0) s = 0;
							else if (s >= trex_width[0]) s = trex_width[0] - 1;
						}
						else
							s &= trex_width[0] - 1;
						s >>= ilod;

						/* clamp T */
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (t < 0) t = 0;
							else if (t >= trex_height[0]) t = trex_height[0] - 1;
						}
						else
							t &= trex_height[0] - 1;
						t >>= ilod;

						/* fetch raw texel data */
						if (!TEXTUREMODE0_BITS(11,1))
							texel = *((UINT8 *)texturebase + (t << lodshift) + s);
						else
							texel = *((UINT16 *)texturebase + (t << lodshift) + s);

						/* convert to ARGB */
						c_local = lookup0[texel];
					}

#if (BILINEAR_FILTER)
					/* bilinear filter */
					else
					{
						INT32 ts0, tt0, ts1, tt1;
						UINT32 factor, factorsum, ag, rb;

						/* convert to int */
						INT32 s, t;
						int ilod = lod >> 8;
						s = TRUNC_TO_INT(fs * 256.0f) - (128 << ilod);
						t = TRUNC_TO_INT(ft * 256.0f) - (128 << ilod);

						/* clamp W */
						if (TEXTUREMODE0_BITS(3,1) && curw0 < 0.0f)
							s = t = 0;

						/* clamp S0 */
						ts0 = s >> 8;
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (ts0 < 0) ts0 = 0;
							else if (ts0 >= trex_width[0]) ts0 = trex_width[0] - 1;
						}
						else
							ts0 &= trex_width[0] - 1;
						ts0 >>= ilod;

						/* clamp S1 */
						ts1 = (s >> 8) + (1 << ilod);
						if (TEXTUREMODE0_BITS(6,1))
						{
							if (ts1 < 0) ts1 = 0;
							else if (ts1 >= trex_width[0]) ts1 = trex_width[0] - 1;
						}
						else
							ts1 &= trex_width[0] - 1;
						ts1 >>= ilod;

						/* clamp T0 */
						tt0 = t >> 8;
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (tt0 < 0) tt0 = 0;
							else if (tt0 >= trex_height[0]) tt0 = trex_height[0] - 1;
						}
						else
							tt0 &= trex_height[0] - 1;
						tt0 >>= ilod;

						/* clamp T1 */
						tt1 = (t >> 8) + (1 << ilod);
						if (TEXTUREMODE0_BITS(7,1))
						{
							if (tt1 < 0) tt1 = 0;
							else if (tt1 >= trex_height[0]) tt1 = trex_height[0] - 1;
						}
						else
							tt1 &= trex_height[0] - 1;
						tt1 >>= ilod;

						s >>= ilod;
						t >>= ilod;

						/* texel 0 */
						factorsum = factor = ((0x100 - (s & 0xff)) * (0x100 - (t & 0xff))) >> 8;

						/* fetch raw texel data */
						if (!TEXTUREMODE0_BITS(11,1))
							texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts0);
						else
							texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts0);

						/* convert to ARGB */
						texel = lookup0[texel];
						ag = ((texel >> 8) & 0x00ff00ff) * factor;
						rb = (texel & 0x00ff00ff) * factor;

						/* texel 1 */
						factorsum += factor = ((s & 0xff) * (0x100 - (t & 0xff))) >> 8;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt0 << lodshift) + ts1);
							else
								texel = *((UINT16 *)texturebase + (tt0 << lodshift) + ts1);

							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}

						/* texel 2 */
						factorsum += factor = ((0x100 - (s & 0xff)) * (t & 0xff)) >> 8;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts0);
							else
								texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts0);

							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}

						/* texel 3 */
						factor = 0x100 - factorsum;
						if (factor)
						{
							/* fetch raw texel data */
							if (!TEXTUREMODE0_BITS(11,1))
								texel = *((UINT8 *)texturebase + (tt1 << lodshift) + ts1);
							else
								texel = *((UINT16 *)texturebase + (tt1 << lodshift) + ts1);

							/* convert to ARGB */
							texel = lookup0[texel];
							ag += ((texel >> 8) & 0x00ff00ff) * factor;
							rb += (texel & 0x00ff00ff) * factor;
						}

						/* this becomes the local color */
						c_local = (ag & 0xff00ff00) | ((rb >> 8) & 0x00ff00ff);
					}
#endif
					/* zero/other selection */
					if (!TEXTUREMODE0_BITS(12,1))				/* tc_zero_other */
					{
						tr = (c_other >> 16) & 0xff;
						tg = (c_other >> 8) & 0xff;
						tb = (c_other >> 0) & 0xff;
					}
					else
						tr = tg = tb = 0;

					/* subtract local color */
					if (TEXTUREMODE0_BITS(13,1))				/* tc_sub_clocal */
					{
						tr -= (c_local >> 16) & 0xff;
						tg -= (c_local >> 8) & 0xff;
						tb -= (c_local >> 0) & 0xff;
					}

					/* scale RGB */
					if (TEXTUREMODE0_BITS(14,3) != 0)			/* tc_mselect mux */
					{
						INT32 rm = 0, gm = 0, bm = 0;

						switch (TEXTUREMODE0_BITS(14,3))
						{
							case 1:		/* tc_mselect mux == c_local */
								rm = (c_local >> 16) & 0xff;
								gm = (c_local >> 8) & 0xff;
								bm = (c_local >> 0) & 0xff;
								break;
							case 2:		/* tc_mselect mux == a_other */
								rm = gm = bm = c_other >> 24;
								break;
							case 3:		/* tc_mselect mux == a_local */
								rm = gm = bm = c_local >> 24;
								break;
							case 4:		/* tc_mselect mux == LOD */
								printf("Detail tex\n");
								rm = gm = bm = (UINT8)(lod >> 3);
								break;
							case 5:		/* tc_mselect mux == LOD frac */
								printf("Trilinear tex\n");
								rm = gm = bm = (UINT8)lod;
								break;
						}

						if (TEXTUREMODE0_BITS(17,1))			/* tc_reverse_blend */
						{
							tr = (tr * (rm + 1)) >> 8;
							tg = (tg * (gm + 1)) >> 8;
							tb = (tb * (bm + 1)) >> 8;
						}
						else
						{
							tr = (tr * ((rm ^ 0xff) + 1)) >> 8;
							tg = (tg * ((gm ^ 0xff) + 1)) >> 8;
							tb = (tb * ((bm ^ 0xff) + 1)) >> 8;
						}
					}

					/* add local color */
					if (TEXTUREMODE0_BITS(18,1))				/* tc_add_clocal */
					{
						tr += (c_local >> 16) & 0xff;
						tg += (c_local >> 8) & 0xff;
						tb += (c_local >> 0) & 0xff;
					}

					/* add local alpha */
					else if (TEXTUREMODE0_BITS(19,1))			/* tc_add_alocal */
					{
						tr += c_local >> 24;
						tg += c_local >> 24;
						tb += c_local >> 24;
					}

					/* zero/other selection */
					if (!TEXTUREMODE0_BITS(21,1))				/* tca_zero_other */
						ta = (c_other >> 24) & 0xff;
					else
						ta = 0;

					/* subtract local alpha */
					if (TEXTUREMODE0_BITS(22,1))				/* tca_sub_clocal */
						ta -= (c_local >> 24) & 0xff;

					/* scale alpha */
					if (TEXTUREMODE0_BITS(23,3) != 0)			/* tca_mselect mux */
					{
						INT32 am = 0;

						switch (TEXTUREMODE0_BITS(23,3))
						{
							case 1:		/* tca_mselect mux == c_local */
								am = (c_local >> 24) & 0xff;
								break;
							case 2:		/* tca_mselect mux == a_other */
								am = c_other >> 24;
								break;
							case 3:		/* tca_mselect mux == a_local */
								am = c_local >> 24;
								break;
							case 4:		/* tca_mselect mux == LOD */
								am = (UINT8)(lod >> 3);
								break;
							case 5:		/* tca_mselect mux == LOD frac */
								am = (UINT8)lod;
								break;
						}

						if (TEXTUREMODE0_BITS(26,1))			/* tca_reverse_blend */
							ta = (ta * (am + 1)) >> 8;
						else
							ta = (ta * ((am ^ 0xff) + 1)) >> 8;
					}

					/* add local color */
					/* how do you add c_local to the alpha???? */
					/* CalSpeed does this in its FMV */
					if (TEXTUREMODE0_BITS(27,1))				/* tca_add_clocal */
						ta += (c_local >> 24) & 0xff;
					if (TEXTUREMODE0_BITS(28,1))				/* tca_add_alocal */
						ta += (c_local >> 24) & 0xff;

					/* clamp */
					if (tr < 0) tr = 0;
					else if (tr > 255) tr = 255;
					if (tg < 0) tg = 0;
					else if (tg > 255) tg = 255;
					if (tb < 0) tb = 0;
					else if (tb > 255) tb = 255;
					if (ta < 0) ta = 0;
					else if (ta > 255) ta = 255;
					texel = (ta << 24) | (tr << 16) | (tg << 8) | tb;

					/* invert */
					if (TEXTUREMODE0_BITS(20,1))
						texel ^= 0x00ffffff;
					if (TEXTUREMODE0_BITS(29,1))
						texel ^= 0xff000000;
				}

				/* handle chroma key */
				if (FBZMODE_BITS(1,1) && ((texel ^ voodoo_regs[chromaKey]) & 0xffffff) == 0)
					goto skipdrawdepth;

				/* compute c_local */
				if (!FBZCOLORPATH_BITS(7,1))
				{
					if (!FBZCOLORPATH_BITS(4,1))			/* cc_localselect mux == iterated RGB */
						c_local = (curr & 0xff0000) | ((curg >> 8) & 0xff00) | ((curb >> 16) & 0xff);
					else									/* cc_localselect mux == color0 RGB */
						c_local = voodoo_regs[color0] & 0xffffff;
				}
				else
				{
					if (!(texel & 0x80000000))				/* cc_localselect mux == iterated RGB */
						c_local = (curr & 0xff0000) | ((curg >> 8) & 0xff00) | ((curb >> 16) & 0xff);
					else									/* cc_localselect mux == color0 RGB */
						c_local = voodoo_regs[color0] & 0xffffff;
				}

				/* compute a_local */
				if (FBZCOLORPATH_BITS(5,2) == 0)		/* cca_localselect mux == iterated alpha */
					c_local |= (cura << 8) & 0xff000000;
				else if (FBZCOLORPATH_BITS(5,2) == 1)	/* cca_localselect mux == color0 alpha */
					c_local |= voodoo_regs[color0] & 0xff000000;
				else if (FBZCOLORPATH_BITS(5,2) == 2)	/* cca_localselect mux == iterated Z */
					c_local |= (curz << 4) & 0xff000000;

				/* determine the RGB values */
				if (!FBZCOLORPATH_BITS(8,1))				/* cc_zero_other */
				{
					if (FBZCOLORPATH_BITS(0,2) == 0)		/* cc_rgbselect mux == iterated RGB */
					{
						r = curr >> 16;
						g = curg >> 16;
						b = curb >> 16;
					}
					else if (FBZCOLORPATH_BITS(0,2) == 1)	/* cc_rgbselect mux == texture RGB */
					{
						r = (texel >> 16) & 0xff;
						g = (texel >> 8) & 0xff;
						b = (texel >> 0) & 0xff;
					}
					else if (FBZCOLORPATH_BITS(0,2) == 2)	/* cc_rgbselect mux == color1 RGB */
					{
						r = (voodoo_regs[color1] >> 16) & 0xff;
						g = (voodoo_regs[color1] >> 8) & 0xff;
						b = (voodoo_regs[color1] >> 0) & 0xff;
					}
				}
				else
					r = g = b = 0;

				/* subtract local color */
				if (FBZCOLORPATH_BITS(9,1))					/* cc_sub_clocal */
				{
					r -= (c_local >> 16) & 0xff;
					g -= (c_local >> 8) & 0xff;
					b -= (c_local >> 0) & 0xff;
				}

				/* scale RGB */
				if (FBZCOLORPATH_BITS(10,3) != 0)			/* cc_mselect mux */
				{
					INT32 rm, gm, bm;

					switch (FBZCOLORPATH_BITS(10,3))
					{
						case 1:		/* cc_mselect mux == c_local */
							rm = (c_local >> 16) & 0xff;
							gm = (c_local >> 8) & 0xff;
							bm = (c_local >> 0) & 0xff;
							break;

						case 2:		/* cc_mselect mux == a_other */
							if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
								rm = gm = bm = cura >> 16;
							else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
								rm = gm = bm = (texel >> 24) & 0xff;
							else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
								rm = gm = bm = (voodoo_regs[color1] >> 24) & 0xff;
							else
								rm = gm = bm = 0;
							break;

						case 3:		/* cc_mselect mux == a_local */
							rm = gm = bm = c_local >> 24;
							break;

						case 4:		/* cc_mselect mux == texture alpha */
							rm = gm = bm = texel >> 24;
							break;

						case 5:		/* cc_mselect mux == texture RGB (voodoo 2 only) */
							rm = (texel >> 16) & 0xff;
							gm = (texel >> 8) & 0xff;
							bm = (texel >> 0) & 0xff;
							break;

						default:
							rm = gm = bm = 0;
							break;
					}

					if (FBZCOLORPATH_BITS(13,1))			/* cc_reverse_blend */
					{
						r = (r * (rm + 1)) >> 8;
						g = (g * (gm + 1)) >> 8;
						b = (b * (bm + 1)) >> 8;
					}
					else
					{
						r = (r * ((rm ^ 0xff) + 1)) >> 8;
						g = (g * ((gm ^ 0xff) + 1)) >> 8;
						b = (b * ((bm ^ 0xff) + 1)) >> 8;
					}
				}

				/* add local color */
				if (FBZCOLORPATH_BITS(14,1))				/* cc_add_clocal */
				{
					r += (c_local >> 16) & 0xff;
					g += (c_local >> 8) & 0xff;
					b += (c_local >> 0) & 0xff;
				}

				/* add local alpha */
				else if (FBZCOLORPATH_BITS(15,1))			/* cc_add_alocal */
				{
					r += c_local >> 24;
					g += c_local >> 24;
					b += c_local >> 24;
				}

				/* determine the A value */
				if (!FBZCOLORPATH_BITS(17,1))				/* cca_zero_other */
				{
					if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
						a = cura >> 16;
					else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
						a = (texel >> 24) & 0xff;
					else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
						a = (voodoo_regs[color1] >> 24) & 0xff;
				}
				else
					a = 0;

				/* subtract local alpha */
				if (FBZCOLORPATH_BITS(18,1))				/* cca_sub_clocal */
					a -= c_local >> 24;

				/* scale alpha */
				if (FBZCOLORPATH_BITS(19,3) != 0)			/* cca_mselect mux */
				{
					INT32 am;

					switch (FBZCOLORPATH_BITS(19,3))
					{
						case 1:		/* cca_mselect mux == a_local */
						case 3:		/* cca_mselect mux == a_local */
							am = c_local >> 24;
							break;

						case 2:		/* cca_mselect mux == a_other */
							if (FBZCOLORPATH_BITS(2,2) == 0)			/* cca_localselect mux == iterated alpha */
								am = cura >> 16;
							else if (FBZCOLORPATH_BITS(2,2) == 1)		/* cca_localselect mux == texture alpha */
								am = (texel >> 24) & 0xff;
							else if (FBZCOLORPATH_BITS(2,2) == 2)		/* cca_localselect mux == color1 alpha */
								am = (voodoo_regs[color1] >> 24) & 0xff;
							else
								am = 0;
							break;

						case 4:		/* cca_mselect mux == texture alpha */
							am = texel >> 24;
							break;

						default:
							am = 0;
							break;
					}

					if (FBZCOLORPATH_BITS(22,1))			/* cca_reverse_blend */
						a = (a * (am + 1)) >> 8;
					else
						a = (a * ((am ^ 0xff) + 1)) >> 8;
				}

				/* add local alpha */
				if (FBZCOLORPATH_BITS(23,2))				/* cca_add_clocal */
					a += c_local >> 24;

				/* handle alpha masking */
				if (FBZMODE_BITS(13,1) && (a & 1) == 0)
					goto skipdrawdepth;

				/* apply alpha function */
				if (ALPHAMODE_BITS(0,4))
				{
					switch (ALPHAMODE_BITS(1,3))
					{
						case 1:
							if (a >= ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 2:
							if (a != ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 3:
							if (a > ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 4:
							if (a <= ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 5:
							if (a == ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
						case 6:
							if (a < ALPHAMODE_BITS(24,8))
								goto skipdrawdepth;
							break;
					}
				}

				/* invert */
				if (FBZCOLORPATH_BITS(16,1))				/* cc_invert_output */
				{
					r ^= 0xff;
					g ^= 0xff;
					b ^= 0xff;
				}
				if (FBZCOLORPATH_BITS(25,1))				/* cca_invert_output */
					a ^= 0xff;

				/* fog */
#if (FOGGING)
				if (FOGMODE_BITS(0,1))
				{
					if (FOGMODE_BITS(5,1))					/* fogconstant */
					{
						r += (voodoo_regs[fogColor] >> 16) & 0xff;
						g += (voodoo_regs[fogColor] >> 8) & 0xff;
						b += (voodoo_regs[fogColor] >> 0) & 0xff;
					}
					else
					{
						INT32 fogalpha;

						if (FOGMODE_BITS(4,1))				/* fogz */
							fogalpha = (curz >> 20) & 0xff;
						else if (FOGMODE_BITS(3,1))			/* fogalpha */
							fogalpha = (cura >> 16) & 0xff;
						else
						{
							INT32 wval;
							float_to_depth(curw, wval);
							fogalpha = fog_blend[wval >> 10];
							fogalpha += (fog_delta[wval >> 10] * ((wval >> 2) & 0xff)) >> 10;
						}

						if (!FOGMODE_BITS(2,1))				/* fogmult */
						{
							if (fogalpha)
							{
								r = (r * (0x100 - fogalpha)) >> 8;
								g = (g * (0x100 - fogalpha)) >> 8;
								b = (b * (0x100 - fogalpha)) >> 8;
							}
						}
						else
							r = g = b = 0;

						if (!FOGMODE_BITS(1,1) && fogalpha)	/* fogadd */
						{
							r += (((voodoo_regs[fogColor] >> 16) & 0xff) * fogalpha) >> 8;
							g += (((voodoo_regs[fogColor] >> 8) & 0xff) * fogalpha) >> 8;
							b += (((voodoo_regs[fogColor] >> 0) & 0xff) * fogalpha) >> 8;
						}
					}
				}
#endif
				/* apply alpha blending */
#if (ALPHA_BLENDING)
				if (ALPHAMODE_BITS(4,1))
				{
					/* quick out for standard alpha blend with opaque pixel */
					if (ALPHAMODE_BITS(8,8) != 0x51 || a < 0xff)
					{
						int dpix = dest[x];
						int dr = (dpix >> 8) & 0xf8;
						int dg = (dpix >> 3) & 0xfc;
						int db = (dpix << 3) & 0xf8;
// fix me -- we don't support alpha layer on the dest
						int da = 0xff;
						int sr = r;
						int sg = g;
						int sb = b;
						int sa = a;
						int ta;

// fix me -- add dither subtraction here
//                      if (FBZMODE_BITS(19,1))

						/* compute source portion */
						switch (ALPHAMODE_BITS(8,4))
						{
							case 0:		// AZERO
								r = g = b = 0;
								break;
							case 1:		// ASRC_ALPHA
								r = (sr * (sa + 1)) >> 8;
								g = (sg * (sa + 1)) >> 8;
								b = (sb * (sa + 1)) >> 8;
								break;
							case 2:		// A_COLOR
								r = (sr * (dr + 1)) >> 8;
								g = (sg * (dg + 1)) >> 8;
								b = (sb * (db + 1)) >> 8;
								break;
							case 3:		// ADST_ALPHA
								r = (sr * (da + 1)) >> 8;
								g = (sg * (da + 1)) >> 8;
								b = (sb * (da + 1)) >> 8;
								break;
							case 4:		// AONE
								break;
							case 5:		// AOMSRC_ALPHA
								r = (sr * (0x100 - sa)) >> 8;
								g = (sg * (0x100 - sa)) >> 8;
								b = (sb * (0x100 - sa)) >> 8;
								break;
							case 6:		// AOM_COLOR
								r = (sr * (0x100 - dr)) >> 8;
								g = (sg * (0x100 - dg)) >> 8;
								b = (sb * (0x100 - db)) >> 8;
								break;
							case 7:		// AOMDST_ALPHA
								r = (sr * (0x100 - da)) >> 8;
								g = (sg * (0x100 - da)) >> 8;
								b = (sb * (0x100 - da)) >> 8;
								break;
							case 15:	// ASATURATE
								ta = (sa < 0x100 - da) ? sa : 0x100 - da;
								r = (sr * (0x100 - ta)) >> 8;
								g = (sg * (0x100 - ta)) >> 8;
								b = (sb * (0x100 - ta)) >> 8;
								break;
						}

						/* add in dest portion */
						switch (ALPHAMODE_BITS(12,4))
						{
							case 0:		// AZERO
								break;
							case 1:		// ASRC_ALPHA
								r += (dr * (sa + 1)) >> 8;
								g += (dg * (sa + 1)) >> 8;
								b += (db * (sa + 1)) >> 8;
								break;
							case 2:		// A_COLOR
								r += (dr * (sr + 1)) >> 8;
								g += (dg * (sg + 1)) >> 8;
								b += (db * (sb + 1)) >> 8;
								break;
							case 3:		// ADST_ALPHA
								r += (dr * (da + 1)) >> 8;
								g += (dg * (da + 1)) >> 8;
								b += (db * (da + 1)) >> 8;
								break;
							case 4:		// AONE
								r += dr;
								g += dg;
								b += db;
								break;
							case 5:		// AOMSRC_ALPHA
								r += (dr * (0x100 - sa)) >> 8;
								g += (dg * (0x100 - sa)) >> 8;
								b += (db * (0x100 - sa)) >> 8;
								break;
							case 6:		// AOM_COLOR
								r += (dr * (0x100 - sr)) >> 8;
								g += (dg * (0x100 - sg)) >> 8;
								b += (db * (0x100 - sb)) >> 8;
								break;
							case 7:		// AOMDST_ALPHA
								r += (dr * (0x100 - da)) >> 8;
								g += (dg * (0x100 - da)) >> 8;
								b += (db * (0x100 - da)) >> 8;
								break;
							case 15:	// A_COLORBEFOREFOG
								osd_die("Unimplemented voodoo feature: A_COLORBEFOREFOG\n");
								break;
						}
					}

					// fix me -- alpha blending on the alpha channel not implemented
				}
#endif
				/* clamp */
				if (r < 0) r = 0;
				else if (r > 255) r = 255;
				if (g < 0) g = 0;
				else if (g > 255) g = 255;
				if (b < 0) b = 0;
				else if (b > 255) b = 255;
//              if (a < 0) a = 0;
//              else if (a > 255) a = 255;

				/* apply dithering */
				if (FBZMODE_BITS(8,1))
				{
					UINT8 dith = dither_matrix[x & 3];
					r = DITHER_RB(r, dith);
					g = DITHER_G (g, dith);
					b = DITHER_RB(b, dith);
				}

#if DEBUG_LOD
{
	static const UINT8 rgblod[] =
	{
		0x00,0x00,0x00,0x00,	/* black -- not used */
		0x00,0x00,0xff,0x00,	/* blue = LOD1 */
		0x00,0xff,0x00,0x00,	/* green = LOD2 */
		0x00,0xff,0xff,0x00,	/* cyan = LOD3 */
		0xff,0x00,0x00,0x00,	/* red = LOD4 */
		0xff,0x00,0xff,0x00,	/* purple = LOD5 */
		0xff,0xff,0x00,0x00,	/* yellow = LOD6 */
		0xff,0xff,0xff,0x00,	/* white = LOD7 */
		0x00,0x00,0x00,0x00		/* black = LOD8 */
	};
	if (loglod && lodhit)
	{
		r = rgblod[lodhit*4+0];
		g = rgblod[lodhit*4+1];
		b = rgblod[lodhit*4+2];
	}
}
#endif

				/* stuff */
				if (FBZMODE_BITS(9,1))
					dest[x] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
				if (FBZMODE_BITS(10,1))
					depth[x] = depthval;

		skipdrawdepth:
				/* advance */
				curr += tri_drdx;
				curg += tri_dgdx;
				curb += tri_dbdx;
				cura += tri_dadx;
				curz += tri_dzdx;
				curw += tri_dwdx;
				curs0 += tri_ds0dx;
				curt0 += tri_dt0dx;
				curw0 += tri_dw0dx;
#if (NUM_TMUS > 1)
				curs1 += tri_ds1dx;
				curt1 += tri_dt1dx;
				curw1 += tri_dw1dx;
#endif
			}
		}
	}

	voodoo_regs[fbiPixelsIn] &= 0xffffff;
}
