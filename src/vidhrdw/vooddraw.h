/*************************************************************************

	3dfx Voodoo rasterization

**************************************************************************/


static void generic_render_1tmu(void);
static void generic_render_2tmu(void);

static void render_0c000035_00045119_000b4779_0824101f(void);
static void render_0c000035_00045119_000b4779_0824109f(void);
static void render_0c000035_00045119_000b4779_082410df(void);
static void render_0c000035_00045119_000b4779_082418df(void);

static void render_0c600c09_00045119_000b4779_0824100f(void);
static void render_0c600c09_00045119_000b4779_0824180f(void);
static void render_0c600c09_00045119_000b4779_082418cf(void);
static void render_0c480035_00045119_000b4779_082418df(void);
static void render_0c480035_00045119_000b4379_082418df(void);

static void render_0c000035_00040400_000b4739_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c26180f(void);
static void render_0c000035_64040409_000b4739_0c26180f(void);
static void render_0c002c35_64515119_000b4799_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c2618cf(void);
static void render_0c002c35_40515119_000b4739_0c26180f(void);


//static int generate_rasterizer(void);


/*************************************
 *
 *	Dither helper
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
 *	FASTFILL handler
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
#if (RESOLUTION_DIVIDE_SHIFT != 0)
		if (cheating_allowed)
		{
			for (y = sy; y < ey; y++)
			{
				int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
				UINT16 *dest = &buffer[effy * FRAMEBUF_WIDTH];

				if (effy & ((1 << RESOLUTION_DIVIDE_SHIFT) - 1))
					continue;

				/* if not dithered, it's easy */
				if (!fbz_dithering)
				{
					UINT16 color = dither[0];
					for (x = (sx >> RESOLUTION_DIVIDE_SHIFT) << RESOLUTION_DIVIDE_SHIFT; x < ex; x += (1 << RESOLUTION_DIVIDE_SHIFT))
						dest[x] = color;
				}
				
				/* dithered is a little trickier */
				else
				{
					UINT16 *dbase = dither + 4 * (y & 3);
					for (x = (sx >> RESOLUTION_DIVIDE_SHIFT) << RESOLUTION_DIVIDE_SHIFT; x < ex; x += (1 << RESOLUTION_DIVIDE_SHIFT))
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
#if (RESOLUTION_DIVIDE_SHIFT != 0)
		if (cheating_allowed)
		{
			for (y = sy; y < ey; y++)
			{
				int effy = (fbz_invert_y ? (inverted_yorigin - y) : y) & (FRAMEBUF_HEIGHT-1);
				UINT16 *dest = &depthbuf[effy * FRAMEBUF_WIDTH];
				if (effy & ((1 << RESOLUTION_DIVIDE_SHIFT) - 1))
					continue;
				for (x = (sx >> RESOLUTION_DIVIDE_SHIFT) << RESOLUTION_DIVIDE_SHIFT; x < ex; x += (1 << RESOLUTION_DIVIDE_SHIFT))
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
 *	Triangle renderer
 *
 *************************************/

static int add_rasterizer(void *callback)
{
	struct rasterizer_info *info = &raster[num_rasters++];
	if (num_rasters > MAX_RASTERIZERS)
		return 0;
		
	info->next = raster_head;
	raster_head = info;
	
	info->callback = (void (*)(void))callback;
	info->needs_texMode0 = ((voodoo_regs[fbzColorPath] >> 27) & 1);
	info->needs_texMode1 = info->needs_texMode0 &&
				(((voodoo_regs[0x100 + textureMode] >> 12) & 1) == 0 ||
				 ((voodoo_regs[0x100 + textureMode] >> 14) & 3) == 2 ||
				 ((voodoo_regs[0x100 + textureMode] >> 21) & 1) == 0 ||
				 ((voodoo_regs[0x100 + textureMode] >> 23) & 3) == 2);
	info->val_fbzColorPath = voodoo_regs[fbzColorPath];
	info->val_alphaMode = voodoo_regs[alphaMode];
	info->val_fogMode = voodoo_regs[fogMode];
	info->val_fbzMode = voodoo_regs[fbzMode];
	info->val_textureMode0 = voodoo_regs[0x100 + textureMode];
	info->val_tlod0 = voodoo_regs[0x100 + tLOD];
	info->val_textureMode1 = voodoo_regs[0x200 + textureMode];
	info->val_tlod1 = voodoo_regs[0x200 + tLOD];
	
	printf("Adding rasterizer @ %08X : %08X %08X %08X %08X %08X %08X %08X %08X\n",
			(UINT32)callback,
			info->val_fbzColorPath, info->val_alphaMode, info->val_fogMode, info->val_fbzMode, 
			info->needs_texMode0 ? info->val_textureMode0 : 0,
			info->needs_texMode0 ? info->val_tlod0 : 0,
			info->needs_texMode1 ? info->val_textureMode1 : 0,
			info->needs_texMode1 ? info->val_tlod1 : 0);
	
	return 1;
}


static void draw_triangle(void)
{
	int totalpix = voodoo_regs[fbiPixelsIn];
	UINT32 temp;

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

 if (code_pressed(KEYCODE_Y))
 {
 	logerror("fbzColorPath=%08X alphaMode=%08X fogMode=%08X fbzMode=%08X texMode0=%08X tLOD0=%08X texMode1=%08X tLOD1=%08X\n",
 			voodoo_regs[fbzColorPath], voodoo_regs[alphaMode], voodoo_regs[fogMode], voodoo_regs[fbzMode],
 			voodoo_regs[0x100 + textureMode], voodoo_regs[0x100 + tLOD], voodoo_regs[0x200 + textureMode], voodoo_regs[0x200 + tLOD]);
 }

#if 0
{
	struct rasterizer_info *info, *prev = NULL;
	/* find an entry in the rasterizer table */
	for (info = raster_head; info; prev = info, info = info->next)
	{
		if (info->val_fbzColorPath != voodoo_regs[fbzColorPath])
			continue;
		if (info->val_alphaMode != voodoo_regs[alphaMode])
			continue;
		if (info->val_fogMode != voodoo_regs[fogMode])
			continue;
		if (info->val_fbzMode != voodoo_regs[fbzMode])
			continue;
		if (info->needs_texMode0 && info->val_textureMode0 != voodoo_regs[0x100 + textureMode])
			continue;
//		if (info->needs_texMode0 && info->val_tlod0 != voodoo_regs[0x100 + tLOD])
//			continue;
		if (info->needs_texMode1 && info->val_textureMode1 != voodoo_regs[0x200 + textureMode])
			continue;
//		if (info->needs_texMode1 && info->val_tlod1 != voodoo_regs[0x200 + tLOD])
//			continue;
		
		/* call the rasterizer */
		(*info->callback)();
		
		/* if we weren't the head, become the head */
		if (prev)
		{
			prev->next = info->next;
			info->next = raster_head;
			raster_head = info;
		}
		goto done;
	}

	/* didn't find one; better generate one */
	if (generate_rasterizer())
	{
		(*raster_head->callback)();
		goto done;
	}
}
#endif

	SETUP_FPU();
	temp = voodoo_regs[fbzColorPath] & FBZCOLORPATH_MASK;
	if (temp == 0x0c000035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824101f)
					{ render_0c000035_00045119_000b4779_0824101f();	goto done; }	/* wg3dh */
				else if (temp == 0x0824109f)
					{ render_0c000035_00045119_000b4779_0824109f();	goto done; }	/* wg3dh */
				else if (temp == 0x082410df)
					{ render_0c000035_00045119_000b4779_082410df();	goto done; }	/* wg3dh */
				else if (temp == 0x082418df)
					{ render_0c000035_00045119_000b4779_082418df();	goto done; }	/* wg3dh */
			}
		}
		else if (temp == 0x00040400)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c000035_00040400_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
		else if (temp == 0x64040409)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c000035_64040409_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c002c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x64515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4799)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c002c35_64515119_000b4799_0c26180f();	goto done; }	/* blitz99 */
			}
		}
		else if (temp == 0x40515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c002c35_40515119_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c582c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00515110)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c582c35_00515110_000b4739_0c26180f();	goto done; }	/* blitz99 */
				else if (temp == 0x0c2618cf)
					{ render_0c582c35_00515110_000b4739_0c2618cf();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c600c09)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824100f)
					{ render_0c600c09_00045119_000b4779_0824100f();	goto done; }	/* mace */
				else if (temp == 0x0824180f)
					{ render_0c600c09_00045119_000b4779_0824180f();	goto done; }	/* mace */
				else if (temp == 0x082418cf)
					{ render_0c600c09_00045119_000b4779_082418cf();	goto done; }	/* mace */
			}
		}
	}
	else if (temp == 0x0c480035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					{ render_0c480035_00045119_000b4779_082418df();	goto done; }	/* mace */
			}
			else if (temp == 0x000b4379)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					{ render_0c480035_00045119_000b4379_082418df();	goto done; }	/* mace */
			}
		}
	}
	if (tmus == 1)
		generic_render_1tmu();
	else
		generic_render_2tmu();

done:
	RESTORE_FPU();
	totalpix = voodoo_regs[fbiPixelsIn] - totalpix;
	if (totalpix < 0) totalpix += 0x1000000;

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
 *	Texel lookups
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
 *	Generate blitters
 *
 *************************************/

#if 0
#include "voodbx86.h"
#endif

/* 
	WG3dh:
	
    816782: 0C000035 00000000 00045119 000B4779 082410DF
    629976: 0C000035 00000000 00045119 000B4779 0824109F
    497958: 0C000035 00000000 00045119 000B4779 0824101F
    141069: 0C000035 00000000 00045119 000B4779 082418DF
*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824101f
#define TEXTUREMODE0		0x0824101f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824109f
#define TEXTUREMODE0		0x0824109f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082410df
#define TEXTUREMODE0		0x082410df
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082418df
#define TEXTUREMODE0		0x082418df
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*

	mace:
	
000000001173E00C: 0C000035 00000000 00045119 000B4779 082418DF (done)
000000000D3EB6D7: 0C600C09 00000000 00045119 000B4779 0824100F
0000000003E4A1D5: 0C600C09 00000000 00045119 000B4779 0824180F
0000000003AAEA07: 0C600C09 00000000 00045119 000B4779 082418CF
000000000389D5A8: 0C480035 00000000 00045119 000B4779 082418DF
000000000168ED9C: 0C480035 00000000 00045119 000B4379 082418DF
000000000142E146: 08602401 00000000 00045119 000B4779 082418DF

*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c600c09
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824100f
#define TEXTUREMODE0		0x0824100f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824180f
#define TEXTUREMODE0		0x0824180f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_082418cf
#define TEXTUREMODE0		0x082418cf
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZCOLORPATH
#undef FBZMODE
#define FBZCOLORPATH		0x0c480035
#define TEXTUREMODE0		0x082418df

#define FBZMODE				0x000b4779
#define RENDERFUNC			render_0c480035_00045119_000b4779_082418df
#include "voodrast.h"
#undef FBZMODE
#undef RENDERFUNC

#define FBZMODE				0x000b4379
#define RENDERFUNC			render_0c480035_00045119_000b4379_082418df
#include "voodrast.h"
#undef FBZMODE
#undef RENDERFUNC

#undef TEXTUREMODE0
#undef RENDERFUNC
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*
	blitz99:

389BA0CA: 0C000035 00000000 00040400 000B4739 0C26180F 00000000 00000000
0667BB5A: 0C582C35 00000000 00515110 000B4739 0C26180F 00000000 00000000
0661E0A1: 0C000035 00000000 64040409 000B4739 0C26180F 00000000 00000000
048488C4: 0C002C35 00000000 64515119 000B4799 0C26180F 00000000 00000000
044A750D: 0C582C35 00000000 00515110 000B4739 0C2618CF 00000000 00000000
04351781: 0C002C35 00000000 40515119 000B4739 0C26180F 00000000 00000000
02A984D0: 0C002C35 00000000 40515119 000B47F9 0C26180F 00000000 00000000
0121D0A8: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000

0000000039CEEE06: 0C000035 00000001 00040400 000B4739 0C26180F
0000000011F145DD: 0C582C35 00000000 00515110 000B4739 0C2618CF
000000000DEAA542: 0C582C35 00000000 00515110 000B4739 0C26180F
000000000C8D034E: 0C002C35 00000000 00515110 000B47F9 0C26180F
0000000005599D25: 0C000035 00000001 64040409 000B4739 0C26180F
00000000043FA611: 0C000035 00000000 00040400 000B4739 0C26180F
000000000422A43F: 0C002C35 00000001 40515119 000B4739 0C26180F
0000000003959E1E: 0C002C35 00000001 64515119 000B4799 0C26180F
000000000347D228: 0C000035 00000001 00040400 000B47F9 0C26180F
00000000033A5BC8: 0C002C35 00000001 40515119 000B47F9 0C26180F
0000000002F8A88F: 0D422439 00000000 00040400 000B473B 0C2610C9
*/

#define NUM_TMUS			1

#define RENDERFUNC			render_0c000035_00040400_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00040400
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c26180f
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_64040409_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x64040409
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_64515119_000b4799_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x64515119
#define FBZMODE				0x000b4799
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c2618cf
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c2618cf
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_40515119_000b4739_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x40515119
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#undef NUM_TMUS


/*
	Sfrush:

One of these is bad:
       175: 0C000035 00000000 00045119 000B4779 082418DF 00000000 00000000
       175: 0C480035 00000000 00045119 000B4779 082418DF 00000000 00000000
       703: 0C000035 00000000 00045119 000B477B 082410DB 00000000 00000000
       
       108: 0C600C09 00000001 00045119 000B4779 0824101F 0824101F 00000000
       688: 0C600C09 00000001 00045119 000B4779 00000000 00000000 00000000
         1: 0C600C09 00000001 00045119 000B4779 0824101F 082410DF 00000000
        47: 0C600C09 00000001 00045119 000B4779 082410DF 0824101F 00000000
        53: 0C600C09 00000001 00045119 000B4779 082418DF 0824101F 00000000
OK      41: 0C482435 00000001 00045119 000B4379 0824101F 0824101F 00000000
       225: 0C600C09 00000001 00045119 000B4779 0824101F 0824181F 00000000
        18: 0C600C09 00000001 00045119 000B4779 0824181F 0824181F 00000000
        13: 0C600C09 00000001 00045119 000B4779 082418DF 0824181F 00000000
        23: 0C000035 00000001 00045119 000B4779 082418DF 0824181F 00000000
         9: 0C000035 00000001 00045119 000B477B 082410DB 0824181F 00000000
       463: 0C000035 00000001 00045119 000B4779 082410DF 0824181F 00000000
         1: 0C480035 00000001 00045119 000B4779 082418DF 0824181F 00000000
         
       127: 0C000035 00000000 00045119 000B4779 082410DF 0824181F 00000000
        31: 0C480035 00000000 00045119 000B4779 082418DF 0824181F 00000000
       127: 0C000035 00000000 00045119 000B477B 082410DB 0824181F 00000000
       
-----------------------------------------------------------
	
       511: 00000002 00000000 00000000 00000300 00000000
         1: 08000001 00000000 00000000 00000300 00000800
         1: 08000001 00000000 00000000 00000200 08241800
     32353: 0C000035 00000000 00045119 000B4779 082418DF
      5437: 0C480035 00000000 00045119 000B4779 082418DF
     23867: 0C000035 00000000 00045119 000B477B 082410DB
     10655: 0C600C09 00000001 00045119 000B4779 0824101F
     13057: 0C600C09 00000001 00045119 000B4779 00000000
       949: 0C600C09 00000001 00045119 000B4779 082410DF
      2723: 0C600C09 00000001 00045119 000B4779 082418DF
       240: 0C482435 00000001 00045119 000B4379 0824101F
      4166: 0C600C09 00000001 00045119 000B4779 0824181F
       747: 0C000035 00000001 00045119 000B4779 082418DF
       427: 0C000035 00000001 00045119 000B477B 082410DB
     12063: 0C000035 00000001 00045119 000B4779 082410DF
        93: 0C480035 00000001 00045119 000B4779 082418DF
      3949: 0C000035 00000000 00045119 000B4779 082410DF
    470768: 0C600C09 00000000 00045119 000B4779 00000000
    418032: 0C600C09 00000000 00045119 000B4779 0824101F
    130673: 0C600C09 00000000 00045119 000B4779 0824181F
      1857: 0C480035 00000000 00045119 000B477B 00000000
      1891: 0C480035 00000000 00045119 000B4779 082410DF
     92962: 0C482435 00000000 00045119 000B4379 0824101F
    119123: 0C600C09 00000000 00045119 000B4779 082708DF
     33176: 0C600C09 00000000 00045119 000B4779 082418DF
     44448: 0C600C09 00000000 00045119 000B4779 082700DF
      1937: 0C600C09 00000000 00045119 000B4779 0827001F
     36352: 0C600C09 00000000 00045119 000B4779 082410DF
       328: 0C600C09 00000001 00045119 000B4779 082700DF
       659: 0C600C09 00000001 00045119 000B4779 082708DF
        67: 0C480035 00000000 00045119 000B4779 00000000
        
*/


/*

Carnevil:
0000000002B4E96A: 0C002425 00000000 00045119 00034679 0C26180F 00000000 00000000
0000000002885479: 0C002435 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000001EE2400: 0C480015 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000001B92D31: 0D422439 00000000 00040400 000B4739 243210C9 00000000 00000000
000000000186F400: 0C000035 00000000 0A045119 000346F9 0C2618C9 00000000 00000000
00000000013C93EE: 0C482415 00000000 0A045119 000346F9 0C26180F 00000000 00000000
000000000139CF3C: 0C482415 00000000 40045119 00034679 0C2618C9 00000000 00000000
00000000013697FC: 0C486116 00000000 01045119 00034279 0C26180F 00000000 00000000
0000000000E4DE5A: 0C482415 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000000DF385B: 0C482415 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000000D02EB3: 0D422409 00000000 00045119 00034679 0C26180F 00000000 00000000

00000000004F8328: 0C002435 00000000 40045119 000B4779 0C26180F 00000000 00000000
000000000000EDA8: 0C002435 00000000 40045119 000B43F9 0C26180F 00000000 00000000
000000000005B736: 0C002435 00000000 40045119 000342F9 0C26180F 00000000 00000000
0000000000A7E85E: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000
000000000010DCC4: 0D420039 00000000 00040400 000B473B 0C2610C9 00000000 00000000
00000000006525EC: 0C002435 00000000 08045119 000346F9 0C26180F 00000000 00000000
00000000003EFE00: 0C000035 00000000 08045119 000346F9 0C2618C9 00000000 00000000
0000000000086AE4: 0C482415 00000000 05045119 00034679 0C26180F 00000000 00000000
000000000000B1EE: 0C482405 00000000 00045119 00034679 0C26180F 00000000 00000000
00000000003D9446: 0C482415 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000BD3AB0: 0C480015 00000000 04045119 000346F9 0C2618C9 00000000 00000000
0000000000001027: 0542611A 00000000 00515119 00034679 0C26180F 00000000 00000000
00000000002C1360: 0C000035 00000000 04045119 00034679 0C2618C9 00000000 00000000
000000000007BAB0: 0C002435 00000000 04045119 00034679 0C2618C9 00000000 00000000
0000000000005360: 0C002425 00000000 04045119 00034679 0C26180F 00000000 00000000
00000000000190D8: 0C482415 00000000 10045119 00034679 0C2618C9 00000000 00000000
0000000000470C50: 0C482415 00000000 05045119 00034279 0C2618C9 00000000 00000000
00000000000A2800: 0C000035 00000000 04040409 00034679 0C2618C9 00000000 00000000
000000000001E814: 0C002435 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000146C20: 0C000035 00000000 00045119 00034679 0C2618C9 00000000 00000000
00000000000B6B00: 0C002435 00000000 00045119 00034679 0C2618C9 00000000 00000000
000000000002A05E: 0C482415 00000000 05045119 000346F9 0C26180F 00000000 00000000
00000000001E1C98: 0C002435 00000000 40045119 00034679 0C26180F 00000000 00000000
00000000000198CA: 0C002435 00000000 40045119 000346F9 0C26180F 00000000 00000000

*/

#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1_MASK
#undef FBZMODE_MASK
#undef ALPHAMODE_MASK
#undef FBZCOLORPATH_MASK

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
