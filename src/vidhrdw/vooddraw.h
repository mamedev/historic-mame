/*************************************************************************

    3dfx Voodoo rasterization

**************************************************************************/


static void generic_render_1tmu(void);
static void generic_render_2tmu(void);

static struct rasterizer_info *generate_rasterizer(void);


/*************************************
 *
 *  Dither helper
 *
 *************************************/

INLINE void dither_to_matrix(UINT32 color, UINT16 *matrix)
{
	UINT32 rawr = (color >> 16) & 0xff;
	UINT32 rawg = (color >> 8) & 0xff;
	UINT32 rawb = color & 0xff;
	int i;

	for (i = 0; i < 16; i++)
	{
		UINT8 dither = fbz_dither_matrix[i];
		UINT32 newr = DITHER_VAL(rawr,dither);
		UINT32 newg = DITHER_VAL(rawg,dither);
		UINT32 newb = DITHER_VAL(rawb,dither);
		matrix[i] = ((newr >> 4) << 11) | ((newg >> 3) << 5) | (newb >> 4);
	}
}



/*************************************
 *
 *  FASTFILL handler
 *
 *************************************/

static void fastfill(void)
{
	int sx = (voodoo_regs[clipLeftRight] >> 16) & 0x3ff;
	int ex = (voodoo_regs[clipLeftRight] >> 0) & 0x3ff;
	int sy = (voodoo_regs[clipLowYHighY] >> 16) & 0x3ff;
	int ey = (voodoo_regs[clipLowYHighY] >> 0) & 0x3ff;
	UINT16 *buffer = *fbz_draw_buffer;
	UINT16 dither[16];
	int x, y;

	/* frame buffer clear? */
	if (fbz_rgb_write)
	{
		if (LOG_COMMANDS)
			logerror("FASTFILL RGB = %06X\n", voodoo_regs[color1] & 0xffffff);

		/* determine dithering */
		if (!fbz_dithering)
			dither[0] = (((voodoo_regs[color1] >> 19) & 0x1f) << 11) | (((voodoo_regs[color1] >> 10) & 0x3f) << 5) | (((voodoo_regs[color1] >> 3) & 0x1f) << 0);
		else
			dither_to_matrix(voodoo_regs[color1], dither);

		/* loop over y */
#if (OPTIMIZATIONS_ENABLED)
		if (cheating_allowed)
		{
			for (y = sy; y < ey; y++)
			{
				int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
				UINT16 *dest = &buffer[effy * FRAMEBUF_WIDTH];

				if (effy & resolution_mask)
					continue;

				/* if not dithered, it's easy */
				if (!fbz_dithering)
				{
					UINT16 color = dither[0];
					for (x = sx & ~resolution_mask; x < ex; x += resolution_mask + 1)
						dest[x] = color;
				}

				/* dithered is a little trickier */
				else
				{
					UINT16 *dbase = dither + 4 * (y & 3);
					for (x = sx & ~resolution_mask; x < ex; x += resolution_mask + 1)
						dest[x] = dbase[x & 3];
				}
			}
		}
		else
#endif
		for (y = sy; y < ey; y++)
		{
			int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
			UINT16 *dest = &buffer[effy * FRAMEBUF_WIDTH + sx];

			/* if not dithered, it's easy */
			if (!fbz_dithering)
			{
				UINT16 color = dither[0];
				for (x = sx; x < ex; x++)
					*dest++ = color;
			}

			/* dithered is a little trickier */
			else
			{
				UINT16 *dbase = dither + 4 * (y & 3);
				for (x = sx; x < ex; x++)
					*dest++ = dbase[x & 3];
			}
		}
	}

	/* depth buffer clear? */
	if (fbz_depth_write)
	{
		UINT16 color = voodoo_regs[zaColor];
		if (LOG_COMMANDS)
			logerror("FASTFILL depth = %04X\n", color);

		/* loop over y */
#if (OPTIMIZATIONS_ENABLED)
		if (cheating_allowed)
		{
			for (y = sy; y < ey; y++)
			{
				int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
				UINT16 *dest = &depthbuf[effy * FRAMEBUF_WIDTH];
				if (effy & resolution_mask)
					continue;
				for (x = sx & ~resolution_mask; x < ex; x += resolution_mask + 1)
					dest[x] = color;
			}
		}
		else
#endif
		for (y = sy; y < ey; y++)
		{
			int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
			UINT16 *dest = &depthbuf[effy * FRAMEBUF_WIDTH + sx];
			for (x = sx; x < ex; x++)
				*dest++ = color;
		}
	}
}



/*************************************
 *
 *  TMU helpers
 *
 *************************************/

INLINE int needs_tmu0(void)
{
	return ((voodoo_regs[fbzColorPath] >> 27) & 1);
}


INLINE int needs_tmu1(void)
{
	return tmus > 1 &&
			(((voodoo_regs[0x100 + textureMode] >> 12) & 1) == 0 ||
			 ((voodoo_regs[0x100 + textureMode] >> 14) & 3) == 2 ||
			 ((voodoo_regs[0x100 + textureMode] >> 21) & 1) == 0 ||
			 ((voodoo_regs[0x100 + textureMode] >> 23) & 3) == 2);
}



/*************************************
 *
 *  Rasterizer management
 *
 *************************************/

INLINE UINT32 compute_raster_hash(void)
{
	int needs_texMode0 = needs_tmu0();
	int needs_texMode1 = needs_texMode0 && needs_tmu1();
	UINT32 hash;

	hash = voodoo_regs[fbzColorPath];
	hash = (hash << 1) | (hash >> 31);
	hash ^= voodoo_regs[alphaMode];
	hash = (hash << 1) | (hash >> 31);
	hash ^= voodoo_regs[fogMode];
	hash = (hash << 1) | (hash >> 31);
	hash ^= voodoo_regs[fbzMode];
	if (needs_texMode0)
	{
		hash = (hash << 1) | (hash >> 31);
		hash ^= voodoo_regs[0x100 + textureMode];
		hash = (hash << 1) | (hash >> 31);
		hash ^= voodoo_regs[0x100 + tLOD];
	}
	if (needs_texMode1)
	{
		hash = (hash << 1) | (hash >> 31);
		hash ^= voodoo_regs[0x200 + textureMode];
		hash = (hash << 1) | (hash >> 31);
		hash ^= voodoo_regs[0x200 + tLOD];
	}
	return hash % RASTER_HASH_SIZE;
}


static struct rasterizer_info *add_rasterizer(genf *callback)
{
	struct rasterizer_info *info = auto_malloc(sizeof(*info));
	int needs_texMode0 = needs_tmu0();
	int needs_texMode1 = needs_texMode0 && needs_tmu1();
	int hash = compute_raster_hash();

	/* hook us into the hash table */
	info->next = raster_hash[hash];
	raster_hash[hash] = info;

	/* fill in the data */
	info->is_generic = FALSE;
	info->hits = 0;
	info->polys = 0;
	info->callback = callback;
	info->val_fbzColorPath = voodoo_regs[fbzColorPath];
	info->val_alphaMode = voodoo_regs[alphaMode];
	info->val_fogMode = voodoo_regs[fogMode];
	info->val_fbzMode = voodoo_regs[fbzMode];
	info->val_textureMode0 = needs_texMode0 ? voodoo_regs[0x100 + textureMode] : 0;
	info->val_tlod0 = needs_texMode0 ? voodoo_regs[0x100 + tLOD] : 0;
	info->val_textureMode1 = needs_texMode1 ? voodoo_regs[0x200 + textureMode] : 0;
	info->val_tlod1 = needs_texMode1 ? voodoo_regs[0x200 + tLOD] : 0;

#if (LOG_RASTERIZERS)
	printf("Adding rasterizer @ %08X : %08X %08X %08X %08X %08X %08X %08X %08X (hash=%d)\n",
			(UINT32)callback,
			info->val_fbzColorPath, info->val_alphaMode, info->val_fogMode, info->val_fbzMode,
			needs_texMode0 ? info->val_textureMode0 : 0,
			needs_texMode0 ? info->val_tlod0 : 0,
			needs_texMode1 ? info->val_textureMode1 : 0,
			needs_texMode1 ? info->val_tlod1 : 0,
			hash);
#endif

	return info;
}


static struct rasterizer_info *find_rasterizer(void)
{
	struct rasterizer_info *info, *prev = NULL;
	int needs_texMode0 = needs_tmu0();
	int needs_texMode1 = needs_texMode0 && needs_tmu1();
	int hash = compute_raster_hash();

	/* find the appropriate hash entry */
	for (info = raster_hash[hash]; info; prev = info, info = info->next)
	{
		if (info->val_fbzColorPath != voodoo_regs[fbzColorPath])
			continue;
		if (info->val_alphaMode != voodoo_regs[alphaMode])
			continue;
		if (info->val_fogMode != voodoo_regs[fogMode])
			continue;
		if (info->val_fbzMode != voodoo_regs[fbzMode])
			continue;
		if (needs_texMode0 && info->val_textureMode0 != voodoo_regs[0x100 + textureMode])
			continue;
//      if (needs_texMode0 && info->val_tlod0 != voodoo_regs[0x100 + tLOD])
//          continue;
		if (needs_texMode1 && info->val_textureMode1 != voodoo_regs[0x200 + textureMode])
			continue;
//      if (needs_texMode1 && info->val_tlod1 != voodoo_regs[0x200 + tLOD])
//          continue;

		/* got it, move us to the head of the list */
		if (prev)
		{
			prev->next = info->next;
			info->next = raster_hash[hash];
			raster_hash[hash] = info;
		}

		/* return the result */
		return info;
	}

	/* attempt to generate one */
	return generate_rasterizer();
}



/*************************************
 *
 *  Rasterizers
 *
 *************************************/

static void draw_triangle(void)
{
	int totalpix = voodoo_regs[fbiPixelsIn];
	struct rasterizer_info *info;

	if (cheating_allowed && skip_count != 0)
		return;

	profiler_mark(PROFILER_USER1);

	voodoo_regs[fbiTrianglesOut] = (voodoo_regs[fbiTrianglesOut] + 1) & 0xffffff;

	/* recompute dirty texture params */
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

	/* find a rasterizer */
	info = find_rasterizer();

	SETUP_FPU();
	if (info)
	{
		if (info->is_generic)
			generic_polys++;
		(*info->callback)();

		totalpix = voodoo_regs[fbiPixelsIn] - totalpix;
		if (totalpix < 0) totalpix += 0x1000000;
		info->hits += totalpix;
		info->polys++;
	}
	RESTORE_FPU();

	profiler_mark(PROFILER_END);

	polycount++;
	if (voodoo_regs[fbzMode] & 8)
		wpolycount++;
}



static void setup_and_draw_triangle(void)
{
	float dx1, dy1, dx2, dy2;
	float divisor;

	/* grab the X/Ys at least */
	tri_va.x = setup_verts[0].x;
	tri_va.y = setup_verts[0].y;
	tri_vb.x = setup_verts[1].x;
	tri_vb.y = setup_verts[1].y;
	tri_vc.x = setup_verts[2].x;
	tri_vc.y = setup_verts[2].y;

	/* compute the divisor */
	divisor = 1.0f / ((tri_va.x - tri_vb.x) * (tri_va.y - tri_vc.y) - (tri_va.x - tri_vc.x) * (tri_va.y - tri_vb.y));

	/* backface culling */
	if (voodoo_regs[sSetupMode] & 0x20000)
	{
		int culling_sign = (voodoo_regs[sSetupMode] >> 18) & 1;
		int divisor_sign = (divisor < 0);

		/* if doing strips and ping pong is enabled, apply the ping pong */
		if ((voodoo_regs[sSetupMode] & 0x90000) == 0x00000)
			culling_sign ^= (setup_count - 3) & 1;

		/* if our sign matches the culling sign, we're done for */
		if (divisor_sign == culling_sign)
			return;
	}

	/* compute the dx/dy values */
	dx1 = tri_va.y - tri_vc.y;
	dx2 = tri_va.y - tri_vb.y;
	dy1 = tri_va.x - tri_vb.x;
	dy2 = tri_va.x - tri_vc.x;

	/* set up appropriate bits */
	if (voodoo_regs[sSetupMode] & 0x0001)
	{
		tri_startr = (INT32)(setup_verts[0].r * 65536.0);
		tri_drdx = (INT32)(((setup_verts[0].r - setup_verts[1].r) * dx1 - (setup_verts[0].r - setup_verts[2].r) * dx2) * divisor * 65536.0);
		tri_drdy = (INT32)(((setup_verts[0].r - setup_verts[2].r) * dy1 - (setup_verts[0].r - setup_verts[1].r) * dy2) * divisor * 65536.0);
		tri_startg = (INT32)(setup_verts[0].g * 65536.0);
		tri_dgdx = (INT32)(((setup_verts[0].g - setup_verts[1].g) * dx1 - (setup_verts[0].g - setup_verts[2].g) * dx2) * divisor * 65536.0);
		tri_dgdy = (INT32)(((setup_verts[0].g - setup_verts[2].g) * dy1 - (setup_verts[0].g - setup_verts[1].g) * dy2) * divisor * 65536.0);
		tri_startb = (INT32)(setup_verts[0].b * 65536.0);
		tri_dbdx = (INT32)(((setup_verts[0].b - setup_verts[1].b) * dx1 - (setup_verts[0].b - setup_verts[2].b) * dx2) * divisor * 65536.0);
		tri_dbdy = (INT32)(((setup_verts[0].b - setup_verts[2].b) * dy1 - (setup_verts[0].b - setup_verts[1].b) * dy2) * divisor * 65536.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0002)
	{
		tri_starta = (INT32)(setup_verts[0].a * 65536.0);
		tri_dadx = (INT32)(((setup_verts[0].a - setup_verts[1].a) * dx1 - (setup_verts[0].a - setup_verts[2].a) * dx2) * divisor * 65536.0);
		tri_dady = (INT32)(((setup_verts[0].a - setup_verts[2].a) * dy1 - (setup_verts[0].a - setup_verts[1].a) * dy2) * divisor * 65536.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0004)
	{
		tri_startz = (INT32)(setup_verts[0].z * 4096.0);
		tri_dzdx = (INT32)(((setup_verts[0].z - setup_verts[1].z) * dx1 - (setup_verts[0].z - setup_verts[2].z) * dx2) * divisor * 4096.0);
		tri_dzdy = (INT32)(((setup_verts[0].z - setup_verts[2].z) * dy1 - (setup_verts[0].z - setup_verts[1].z) * dy2) * divisor * 4096.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0008)
	{
		tri_startw = tri_startw0 = tri_startw1 = setup_verts[0].wb;
		tri_dwdx = tri_dw0dx = tri_dw1dx = ((setup_verts[0].wb - setup_verts[1].wb) * dx1 - (setup_verts[0].wb - setup_verts[2].wb) * dx2) * divisor;
		tri_dwdy = tri_dw0dy = tri_dw1dy = ((setup_verts[0].wb - setup_verts[2].wb) * dy1 - (setup_verts[0].wb - setup_verts[1].wb) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0010)
	{
		tri_startw0 = tri_startw1 = setup_verts[0].w0;
		tri_dw0dx = tri_dw1dx = ((setup_verts[0].w0 - setup_verts[1].w0) * dx1 - (setup_verts[0].w0 - setup_verts[2].w0) * dx2) * divisor;
		tri_dw0dy = tri_dw1dy = ((setup_verts[0].w0 - setup_verts[2].w0) * dy1 - (setup_verts[0].w0 - setup_verts[1].w0) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0020)
	{
		tri_starts0 = tri_starts1 = setup_verts[0].s0;
		tri_ds0dx = tri_ds1dx = ((setup_verts[0].s0 - setup_verts[1].s0) * dx1 - (setup_verts[0].s0 - setup_verts[2].s0) * dx2) * divisor;
		tri_ds0dy = tri_ds1dy = ((setup_verts[0].s0 - setup_verts[2].s0) * dy1 - (setup_verts[0].s0 - setup_verts[1].s0) * dy2) * divisor;
		tri_startt0 = tri_startt1 = setup_verts[0].t0;
		tri_dt0dx = tri_dt1dx = ((setup_verts[0].t0 - setup_verts[1].t0) * dx1 - (setup_verts[0].t0 - setup_verts[2].t0) * dx2) * divisor;
		tri_dt0dy = tri_dt1dy = ((setup_verts[0].t0 - setup_verts[2].t0) * dy1 - (setup_verts[0].t0 - setup_verts[1].t0) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0040)
	{
		tri_startw1 = setup_verts[0].w1;
		tri_dw1dx = ((setup_verts[0].w1 - setup_verts[1].w1) * dx1 - (setup_verts[0].w1 - setup_verts[2].w1) * dx2) * divisor;
		tri_dw1dy = ((setup_verts[0].w1 - setup_verts[2].w1) * dy1 - (setup_verts[0].w1 - setup_verts[1].w1) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0080)
	{
		tri_starts1 = setup_verts[0].s1;
		tri_ds1dx = ((setup_verts[0].s1 - setup_verts[1].s1) * dx1 - (setup_verts[0].s1 - setup_verts[2].s1) * dx2) * divisor;
		tri_ds1dy = ((setup_verts[0].s1 - setup_verts[2].s1) * dy1 - (setup_verts[0].s1 - setup_verts[1].s1) * dy2) * divisor;
		tri_startt1 = setup_verts[0].t1;
		tri_dt1dx = ((setup_verts[0].t1 - setup_verts[1].t1) * dx1 - (setup_verts[0].t1 - setup_verts[2].t1) * dx2) * divisor;
		tri_dt1dy = ((setup_verts[0].t1 - setup_verts[2].t1) * dy1 - (setup_verts[0].t1 - setup_verts[1].t1) * dy2) * divisor;
	}

	/* draw the triangle */
	cheating_allowed = 1;
	draw_triangle();
}



/*************************************
 *
 *  Texel lookups
 *
 *************************************/

static void init_texel_0(int which)
{
	/* format 0: 8-bit 3-3-2 */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		r = (i >> 5) & 7;
		g = (i >> 2) & 7;
		b = i & 3;
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 6) | (b << 4) | (b << 2) | b;
		texel_lookup[which][0][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_1(int which)
{
	/* format 1: 8-bit YIQ, NCC table 0 */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		r = g = b = ncc_y[which][0][(i >> 4) & 0x0f];
		r += ncc_ir[which][0][vi] + ncc_qr[which][0][vq];
		g += ncc_ig[which][0][vi] + ncc_qg[which][0][vq];
		b += ncc_ib[which][0][vi] + ncc_qb[which][0][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;

		texel_lookup[which][1][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_2(int which)
{
	/* format 2: 8-bit alpha */
	int i;
	for (i = 0; i < 256; i++)
		texel_lookup[which][2][i] = (i << 24) | (i << 16) | (i << 8) | i;
}


static void init_texel_3(int which)
{
	/* format 3: 8-bit intensity */
	int i;
	for (i = 0; i < 256; i++)
		texel_lookup[which][3][i] = 0xff000000 | (i << 16) | (i << 8) | i;
}


static void init_texel_4(int which)
{
	/* format 4: 8-bit alpha, intensity (4-4) */
	int a, r, i;
	for (i = 0; i < 256; i++)
	{
		a = i >> 4;
		r = i & 15;
		a = (a << 4) | a;
		r = (r << 4) | r;
		texel_lookup[which][4][i] = (a << 24) | (r << 16) | (r << 8) | r;
	}
}


static void init_texel_5(int which)
{
	/* format 5: 8-bit palette -- updated dynamically */
}


static void init_texel_6(int which)
{
	/* format 6: 8-bit palette to RGBA -- updated dynamically */
}


static void init_texel_7(int which)
{
	/* format 7: 8-bit YIQ, NCC table 1 (used internally, not really on the card) */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		r = g = b = ncc_y[which][1][(i >> 4) & 0x0f];
		r += ncc_ir[which][1][vi] + ncc_qr[which][1][vq];
		g += ncc_ig[which][1][vi] + ncc_qg[which][1][vq];
		b += ncc_ib[which][1][vi] + ncc_qb[which][1][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;

		texel_lookup[which][7][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_8(int which)
{
	/* format 8: 16-bit ARGB (8-3-3-2) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		r = (i >> 5) & 7;
		g = (i >> 2) & 7;
		b = i & 3;
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 6) | (b << 4) | (b << 2) | b;
		texel_lookup[which][8][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_9(int which)
{
	/* format 9: 16-bit YIQ, NCC table 0 */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		a = i >> 8;
		r = g = b = ncc_y[which][0][(i >> 4) & 0x0f];
		r += ncc_ir[which][0][vi] + ncc_qr[which][0][vq];
		g += ncc_ig[which][0][vi] + ncc_qg[which][0][vq];
		b += ncc_ib[which][0][vi] + ncc_qb[which][0][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;

		texel_lookup[which][9][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_a(int which)
{
	/* format 10: 16-bit RGB (5-6-5) */
	int r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		r = (i >> 11) & 0x1f;
		g = (i >> 5) & 0x3f;
		b = i & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		texel_lookup[which][10][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_b(int which)
{
	/* format 11: 16-bit ARGB (1-5-5-5) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = ((i >> 15) & 1) ? 0xff : 0x00;
		r = (i >> 10) & 0x1f;
		g = (i >> 5) & 0x1f;
		b = i & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		texel_lookup[which][11][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_c(int which)
{
	/* format 12: 16-bit ARGB (4-4-4-4) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = (i >> 12) & 0x0f;
		r = (i >> 8) & 0x0f;
		g = (i >> 4) & 0x0f;
		b = i & 0x0f;
		a = (a << 4) | a;
		r = (r << 4) | r;
		g = (g << 4) | g;
		b = (b << 4) | b;
		texel_lookup[which][12][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_d(int which)
{
	/* format 13: 16-bit alpha, intensity */
	int a, r, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		r = i & 0xff;
		texel_lookup[which][13][i] = (a << 24) | (r << 16) | (r << 8) | r;
	}
}


static void init_texel_e(int which)
{
	/* format 14: 16-bit alpha, palette */
	int a, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		texel_lookup[which][14][i] = (a << 24) | (texel_lookup[which][5][i & 0xff] & 0x00ffffff);
	}
}


static void init_texel_f(int which)
{
	/* format 9: 16-bit YIQ, NCC table 1 */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		a = i >> 8;
		r = g = b = ncc_y[which][1][(i >> 4) & 0x0f];
		r += ncc_ir[which][1][vi] + ncc_qr[which][1][vq];
		g += ncc_ig[which][1][vi] + ncc_qg[which][1][vq];
		b += ncc_ib[which][1][vi] + ncc_qb[which][1][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;

		texel_lookup[which][15][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void (*update_texel_lookup[16])(int which) =
{
	init_texel_0,	init_texel_1,	init_texel_2,	init_texel_3,
	init_texel_4,	init_texel_5,	init_texel_6,	init_texel_7,
	init_texel_8,	init_texel_9,	init_texel_a,	init_texel_b,
	init_texel_c,	init_texel_d,	init_texel_e,	init_texel_f
};



/*************************************
 *
 *  Generate blitters
 *
 *************************************/

#ifdef VOODOO_DRC
#include "voodrx86.h"
#else
#include "voodrgen.h"
#endif

#undef FBZCOLORPATH_MASK
#undef ALPHAMODE_MASK
#undef FBZMODE_MASK
#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1_MASK

#define FBZCOLORPATH_MASK	0x00000000
#define FBZCOLORPATH		0x00000000
#define ALPHAMODE_MASK		0x00000000
#define ALPHAMODE			0x00000000
#define FBZMODE_MASK		0x00000000
#define FBZMODE				0x00000000
#define TEXTUREMODE0_MASK	0x00000000
#define TEXTUREMODE0		0x00000000
#define TEXTUREMODE1_MASK	0x00000000
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			generic_render_1tmu
#define NUM_TMUS			1

#include "voodrast.h"

#undef NUM_TMUS
#undef RENDERFUNC
#define RENDERFUNC			generic_render_2tmu
#define NUM_TMUS			2

#include "voodrast.h"

#undef NUM_TMUS
#undef RENDERFUNC

#undef TEXTUREMODE0
#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1
#undef TEXTUREMODE1_MASK
#undef FBZMODE
#undef FBZMODE_MASK
#undef ALPHAMODE
#undef ALPHAMODE_MASK
#undef FBZCOLORPATH
#undef FBZCOLORPATH_MASK


