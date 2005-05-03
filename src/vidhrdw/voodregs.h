/*************************************************************************

    3dfx Voodoo register handlers and tables

**************************************************************************/


#ifdef __MWERKS__
#pragma mark FIXED POINT WRITERS
#endif

/*************************************
 *
 *  Fixed point writers
 *
 *************************************/

static void vertexAx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_va.x = (float)(INT16)data * (1.0f / 16.0f);
}

static void vertexAy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_va.y = (float)(INT16)data * (1.0f / 16.0f);
}

static void vertexBx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vb.x = (float)(INT16)data * (1.0f / 16.0f);
}

static void vertexBy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vb.y = (float)(INT16)data * (1.0f / 16.0f);
}

static void vertexCx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vc.x = (float)(INT16)data * (1.0f / 16.0f);
}

static void vertexCy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vc.y = (float)(INT16)data * (1.0f / 16.0f);
}

static void startR_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startr = ((INT32)data << 8) >> 4;
}

static void startG_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startg = ((INT32)data << 8) >> 4;
}

static void startB_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startb = ((INT32)data << 8) >> 4;
}

static void startA_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_starta = ((INT32)data << 8) >> 4;
}

static void startZ_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startz = (INT32)data;
}

static void startW_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startw = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 2) tri_startw0 = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 4) tri_startw1 = (float)(INT32)data * (1.0 / (float)(1 << 30));
}

static void startS_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_starts0 = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_starts1 = (float)(INT32)data * (1.0 / (float)(1 << 18));
}

static void startT_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_startt0 = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_startt1 = (float)(INT32)data * (1.0 / (float)(1 << 18));
}

static void dRdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_drdx = ((INT32)data << 8) >> 4;
}

static void dGdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dgdx = ((INT32)data << 8) >> 4;
}

static void dBdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dbdx = ((INT32)data << 8) >> 4;
}

static void dAdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dadx = ((INT32)data << 8) >> 4;
}

static void dZdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dzdx = (INT32)data;
}

static void dWdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dwdx = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 2) tri_dw0dx = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 4) tri_dw1dx = (float)(INT32)data * (1.0 / (float)(1 << 30));
}

static void dSdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_ds0dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_ds1dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
}

static void dTdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_dt0dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_dt1dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
}

static void dRdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_drdy = ((INT32)data << 8) >> 4;
}

static void dGdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dgdy = ((INT32)data << 8) >> 4;
}

static void dBdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dbdy = ((INT32)data << 8) >> 4;
}

static void dAdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dady = ((INT32)data << 8) >> 4;
}

static void dZdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dzdy = (INT32)data;
}

static void dWdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dwdy = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 2) tri_dw0dy = (float)(INT32)data * (1.0 / (float)(1 << 30));
	if (chips & 4) tri_dw1dy = (float)(INT32)data * (1.0 / (float)(1 << 30));
}

static void dSdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_ds0dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_ds1dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
}

static void dTdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_dt0dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
	if (chips & 4) tri_dt1dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark FLOATING POINT WRITERS
#endif

/*************************************
 *
 *  Floating point writers
 *
 *************************************/

static void fvertexAx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_va.x = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fvertexAy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_va.y = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fvertexBx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vb.x = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fvertexBy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vb.y = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fvertexCx_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vc.x = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fvertexCy_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_vc.y = (INT16)TRUNC_TO_INT(u2f(data) * 16. + 0.5) * (1. / 16.);
}

static void fstartR_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startr = (INT32)(u2f(data) * 65536.0);
}

static void fstartG_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startg = (INT32)(u2f(data) * 65536.0);
}

static void fstartB_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startb = (INT32)(u2f(data) * 65536.0);
}

static void fstartA_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_starta = (INT32)(u2f(data) * 65536.0);
}

static void fstartZ_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startz = (INT32)(u2f(data) * 4096.0);
}

static void fstartW_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_startw = u2f(data);
	if (chips & 2) tri_startw0 = u2f(data);
	if (chips & 4) tri_startw1 = u2f(data);
}

static void fstartS_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_starts0 = u2f(data);
	if (chips & 4) tri_starts1 = u2f(data);
}

static void fstartT_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_startt0 = u2f(data);
	if (chips & 4) tri_startt1 = u2f(data);
}

static void fdRdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_drdx = (INT32)(u2f(data) * 65536.0);
}

static void fdGdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dgdx = (INT32)(u2f(data) * 65536.0);
}

static void fdBdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dbdx = (INT32)(u2f(data) * 65536.0);
}

static void fdAdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dadx = (INT32)(u2f(data) * 65536.0);
}

static void fdZdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dzdx = (INT32)(u2f(data) * 4096.0);
}

static void fdWdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dwdx = u2f(data);
	if (chips & 2) tri_dw0dx = u2f(data);
	if (chips & 4) tri_dw1dx = u2f(data);
}

static void fdSdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_ds0dx = u2f(data);
	if (chips & 4) tri_ds1dx = u2f(data);
}

static void fdTdX_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_dt0dx = u2f(data);
	if (chips & 4) tri_dt1dx = u2f(data);
}

static void fdRdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_drdy = (INT32)(u2f(data) * 65536.0);
}

static void fdGdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dgdy = (INT32)(u2f(data) * 65536.0);
}

static void fdBdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dbdy = (INT32)(u2f(data) * 65536.0);
}

static void fdAdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dady = (INT32)(u2f(data) * 65536.0);
}

static void fdZdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dzdy = (INT32)(u2f(data) * 4096.0);
}

static void fdWdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 1) tri_dwdy = u2f(data);
	if (chips & 2) tri_dw0dy = u2f(data);
	if (chips & 4) tri_dw1dy = u2f(data);
}

static void fdSdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_ds0dy = u2f(data);
	if (chips & 4) tri_ds1dy = u2f(data);
}

static void fdTdY_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2) tri_dt0dy = u2f(data);
	if (chips & 4) tri_dt1dy = u2f(data);
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark MISC REGISTER WRITERS
#endif

/*************************************
 *
 *  Misc register handlers
 *
 *************************************/

static void fbzMode_w(int chips, offs_t regnum, data32_t data)
{
	/* bit 0 = enable clipping rectangle (1=enable) */
	/* bit 1 = enable chroma keying (1=enable) */
	/* bit 2 = enable stipple register masking (1=enable) */
	/* bit 3 = W-buffer select (0=use Z, 1=use W) */
	/* bit 4 = enable depth-buffering (1=enable) */
	/* bit 5-7 = depth-buffer function */
	/* bit 8 = enable dithering */
	/* bit 9 = RGB buffer write mask (0=disable writes to RGB buffer) */
	/* bit 10 = depth/alpha buffer write mask (0=disable writes) */
	/* bit 11 = dither algorithm (0=4x4 ordered, 1=2x2 ordered) */
	/* bit 12 = enable stipple pattern masking (1=enable) */
	/* bit 13 = enable alpha channel mask (1=enable) */
	/* bit 14-15 = draw buffer (0=front, 1=back) */
	/* bit 16 = enable depth-biasing (1=enable) */
	/* bit 17 = rendering commands Y origin (0=top of screen, 1=bottom) */
	/* bit 18 = enable alpha planes (1=enable) */
	/* bit 19 = enable alpha-blending dither subtraction (1=enable) */
	/* bit 20 = depth buffer compare select (0=normal, 1=zaColor) */

	/* extract parameters we can handle */
	fbz_cliprect = ((data >> 0) & 1) ? &fbz_clip : &fbz_noclip;
	fbz_dithering = (data >> 8) & 1;
	fbz_rgb_write = (data >> 9) & 1;
	fbz_depth_write = (data >> 10) & 1;
	fbz_dither_matrix = ((data >> 11) & 1) ? dither_matrix_2x2 : dither_matrix_4x4;
	fbz_draw_buffer = buffer_access[(data >> 14) & 3];
	fbz_invert_y = (data >> 17) & 1;
}


static void lfbMode_w(int chips, offs_t regnum, data32_t data)
{
	/* bit 0-3 = write format */
	/* bit 4-5 = write buffer select (0=front, 1=back) */
	/* bit 6-7 = read buffer select (0=front, 1=back, 2=depth/alpha) */
	/* bit 8 = enable pixel pipeline-processed writes */
	/* bit 9-10 = linear frame buffer RGBA lanes */
	/* bit 11 = 16-bit word swap LFB writes */
	/* bit 12 = byte swizzle LFB writes */
	/* bit 13 = LFB access Y origin (0=top is origin, 1=bottom) */
	/* bit 14 = LFB write access W select (0=LFB selected, 1=zacolor[15:0]) */
	/* bit 15 = 16-bit word swap LFB reads */
	/* bit 16 = byte swizzle LFB reads */

	/* extract parameters we can handle */
	lfb_write_format = (data & 0x0f) + ((data >> 5) & 0x30);
	lfb_write_buffer = buffer_access[(data >> 4) & 3];
	lfb_read_buffer = buffer_access[(data >> 6) & 3];
	lfb_flipy = (data >> 13) & 1;
}


static void clipLeftRight_w(int chips, offs_t regnum, data32_t data)
{
	fbz_clip.min_x = ((data >> 16) & 0x3ff) << 4;
	fbz_clip.max_x = ((data & 0x3ff) << 4);
}


static void clipLowYHighY_w(int chips, offs_t regnum, data32_t data)
{
	fbz_clip.min_y = ((data >> 16) & 0x3ff) << 4;
	fbz_clip.max_y = ((data & 0x3ff) << 4);
}


static void videoDimensions_w(int chips, offs_t regnum, data32_t data)
{
	if (data & 0x3ff)
		video_width = data & 0x3ff;
	if (data & 0x3ff0000)
		video_height = (data >> 16) & 0x3ff;
	set_visible_area(0, video_width - 1, 0, video_height - 1);
	timer_adjust(vblank_timer, cpu_getscanlinetime(video_height), 0, TIME_NEVER);
	reset_buffers();
}


static void fbiInit0_w(int chips, offs_t regnum, data32_t data)
{
	/* writes 0x00000006 */
	/* writes 0x00000002 */
	/* writes 0x00000000 */
	/* writes 0x00000411 */

	/* bit 0 = VGA passtrough */
	/* bit 1 = FBI reset (1) */
	/* bit 2 = FBI FIFO reset (1) */
	/* bit 4 = Stall PCI enable for high water mark */
	/* bit 6-10 = PCI FIFO empty entries for low water mark (0x10) */
	update_memory_fifo();
	if ((data >> 2) & 1)
	{
		memory_fifo_count = 0;
		num_pending_swaps = 0;
	}
}


static void fbiInit3_w(int chips, offs_t regnum, data32_t data)
{
	/* writes 0x00114000 */

	/* bit 16-13 = FBI-to-TREX bus clock delay (0xa) */
	/* bit 21-17 = TREX-to-FBI bus FIFO full thresh (0x8) */
	/* bit 31-22 = Y origin swap subtraction value */

	inverted_yorigin = (data >> 22) & 0x3ff;
}


static void clutData_w(int chips, offs_t regnum, data32_t data)
{
	int index = (data >> 24) & 0x3f;

	if (index <= 32)
	{
		clut_table[index] = data & 0xffffff;

		// fix me
		/* recompute the lookup tables */
	}
}


static void dacData_w(int chips, offs_t regnum, data32_t data)
{
	/* bit 0-7 = data to write */
	/* bit 8-10 = register number */
	/* bit 11 = write (0) or read (1) */
	UINT8 reg = (data >> 8) & 7;

	if (data & 0x800)
	{
		dac_read_result = dac_regs[reg];
		if (reg == 5)
		{
			/* this is just to make startup happy */
			switch (dac_regs[7])
			{
				case 0x01:	dac_read_result = 0x55; break;
				case 0x07:	dac_read_result = 0x71; break;
				case 0x0b:	dac_read_result = 0x79; break;
			}
		}
		if (LOG_REGISTERS)
			logerror("-- dacData read reg %d; result = %02X\n", reg, dac_read_result);
	}
	else
	{
		dac_regs[reg] = data;
		if (LOG_REGISTERS)
			logerror("-- dacData write reg %d = %02X\n", reg, data & 0xff);
	}
}


static void texture_changed_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
		trex_dirty[0] = 1;
	if (chips & 4)
		trex_dirty[1] = 1;
}


static void vSync_w(int chips, offs_t regnum, data32_t data)
{
	if (voodoo_regs[hSync] != 0 && voodoo_regs[vSync] != 0)
	{
		float vtotal = (voodoo_regs[vSync] >> 16) + (voodoo_regs[vSync] & 0xffff);
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
}


static void sARGB_w(int chips, offs_t regnum, data32_t data)
{
	voodoo_regs[sAlpha] = data >> 24;
	voodoo_regs[sRed] = (data >> 16) & 0xff;
	voodoo_regs[sGreen] = (data >> 8) & 0xff;
	voodoo_regs[sBlue] = data & 0xff;
}


static void cmdFifoBump_w(int chips, offs_t regnum, data32_t data)
{
	data32_t old_depth = voodoo_regs[cmdFifoDepth];
	voodoo_regs[cmdFifoDepth] += (UINT16)data;
	cmdfifo_process_pending(old_depth);
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark COMMAND WRITERS
#endif

/*************************************
 *
 *  Command writes
 *
 *************************************/

static void triangleCMD_w(int chips, offs_t regnum, data32_t data)
{
	cheating_allowed = (tri_va.x != 0 || tri_va.y != 0 || tri_vb.x > 50 || tri_vb.y != 0 || tri_vc.x != 0 || tri_vc.y > 50);
	draw_triangle();
}


static void ftriangleCMD_w(int chips, offs_t regnum, data32_t data)
{
	cheating_allowed = 1;
	draw_triangle();
}


static void nopCMD_w(int chips, offs_t regnum, data32_t data)
{
	if (LOG_COMMANDS)
		logerror("%06X:NOP command\n", memory_fifo_in_process ? 0 : activecpu_get_pc());
	if (data & 1)
	{
		voodoo_regs[fbiPixelsIn] = 0;
		voodoo_regs[fbiChromaFail] = 0;
		voodoo_regs[fbiZfuncFail] = 0;
		voodoo_regs[fbiAfuncFail] = 0;
		voodoo_regs[fbiPixelsOut] = 0;
	}
	if (voodoo_type >= 2 && (data & 2))
		voodoo_regs[fbiTrianglesOut] = 0;
}


static void fastfillCMD_w(int chips, offs_t regnum, data32_t data)
{
	if (LOG_COMMANDS)
		logerror("%06X:FASTFILL command\n", memory_fifo_in_process ? 0 : activecpu_get_pc());
	fastfill();
}


static void swapbufferCMD_w(int chips, offs_t regnum, data32_t data)
{
	if (LOG_COMMANDS)
		logerror("%06X:SWAPBUFFER command = %08X (pend=%d)\n", memory_fifo_in_process ? 0 : activecpu_get_pc(), data, num_pending_swaps);

	/* extract parameters */
	swap_vblanks = (data >> 1) & 0x7f;
	swap_dont_swap = (voodoo_type >= 2 && (data & 0x200) != 0);

	/* immediate? */
	if (!(data & 1))
		swap_buffers();

	/* deferred */
	else
	{
		blocked_on_swap = 1;

		/* if we're not executing from the FIFO, we need to bump the pending swap count */
		if (!memory_fifo_in_process)
			num_pending_swaps++;
	}
}


static void sBeginTriCMD_w(int chips, offs_t regnum, data32_t data)
{
	if (LOG_COMMANDS)
		logerror("%06X:BEGIN TRIANGLE command\n", memory_fifo_in_process ? 0 : activecpu_get_pc());

	setup_verts[2].x = TRUNC_TO_INT(fvoodoo_regs[sVx] * 16. + 0.5) * (1. / 16.);
	setup_verts[2].y = TRUNC_TO_INT(fvoodoo_regs[sVy] * 16. + 0.5) * (1. / 16.);
	setup_verts[2].wb = fvoodoo_regs[sWb];
	setup_verts[2].w0 = fvoodoo_regs[sWtmu0];
	setup_verts[2].s0 = fvoodoo_regs[sS_W0];
	setup_verts[2].t0 = fvoodoo_regs[sT_W0];
	setup_verts[2].w1 = fvoodoo_regs[sWtmu1];
	setup_verts[2].s1 = fvoodoo_regs[sS_Wtmu1];
	setup_verts[2].t1 = fvoodoo_regs[sT_Wtmu1];
	setup_verts[2].a = fvoodoo_regs[sAlpha];
	setup_verts[2].r = fvoodoo_regs[sRed];
	setup_verts[2].g = fvoodoo_regs[sGreen];
	setup_verts[2].b = fvoodoo_regs[sBlue];

	setup_verts[0] = setup_verts[1] = setup_verts[2];

	setup_count = 1;
}


static void sDrawTriCMD_w(int chips, offs_t regnum, data32_t data)
{
	if (LOG_COMMANDS)
		logerror("%06X:DRAW TRIANGLE command\n", memory_fifo_in_process ? 0 : activecpu_get_pc());

	if (!(voodoo_regs[sSetupMode] & 0x10000))	/* strip mode */
		setup_verts[0] = setup_verts[1];
	setup_verts[1] = setup_verts[2];

	setup_verts[2].x = TRUNC_TO_INT(fvoodoo_regs[sVx] * 16. + 0.5) * (1. / 16.);
	setup_verts[2].y = TRUNC_TO_INT(fvoodoo_regs[sVy] * 16. + 0.5) * (1. / 16.);
	setup_verts[2].wb = fvoodoo_regs[sWb];
	setup_verts[2].w0 = fvoodoo_regs[sWtmu0];
	setup_verts[2].s0 = fvoodoo_regs[sS_W0];
	setup_verts[2].t0 = fvoodoo_regs[sT_W0];
	setup_verts[2].w1 = fvoodoo_regs[sWtmu1];
	setup_verts[2].s1 = fvoodoo_regs[sS_Wtmu1];
	setup_verts[2].t1 = fvoodoo_regs[sT_Wtmu1];
	setup_verts[2].a = fvoodoo_regs[sAlpha];
	setup_verts[2].r = fvoodoo_regs[sRed];
	setup_verts[2].g = fvoodoo_regs[sGreen];
	setup_verts[2].b = fvoodoo_regs[sBlue];

	if (++setup_count >= 3)
		setup_and_draw_triangle();
}


static void blt_start_w(int chips, offs_t regnum, data32_t data)
{
	if (data & 0x80000000)
		fprintf(stderr, "WARNING: blt command %08X\n", voodoo_regs[bltCommand] & ~0x80000000);
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark TABLE WRITERS
#endif

/*************************************
 *
 *  Table writers
 *
 *************************************/

static void nccTable0y_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		int base = 4 * (regnum - (nccTable+0));
		ncc_y[0][0][base+0] = (data >>  0) & 0xff;
		ncc_y[0][0][base+1] = (data >>  8) & 0xff;
		ncc_y[0][0][base+2] = (data >> 16) & 0xff;
		ncc_y[0][0][base+3] = (data >> 24) & 0xff;
		texel_lookup_dirty[0][1] = 1;
		texel_lookup_dirty[0][9] = 1;
	}
	if (chips & 4)
	{
		int base = 4 * (regnum - (nccTable+0));
		ncc_y[1][0][base+0] = (data >>  0) & 0xff;
		ncc_y[1][0][base+1] = (data >>  8) & 0xff;
		ncc_y[1][0][base+2] = (data >> 16) & 0xff;
		ncc_y[1][0][base+3] = (data >> 24) & 0xff;
		texel_lookup_dirty[1][1] = 1;
		texel_lookup_dirty[1][9] = 1;
	}
}


static void nccTable0irgb_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		if (data & 0x80000000)
		{
			texel_lookup[0][5][((data >> 23) & 0xfe) | (~regnum & 1)] = 0xff000000 | data;
			{
				UINT8 a = ((data >> 16) & 0xfc) | ((data >> 22) & 0x03);
				UINT8 r = ((data >> 10) & 0xfc) | ((data >> 16) & 0x03);
				UINT8 g = ((data >>  4) & 0xfc) | ((data >> 10) & 0x03);
				UINT8 b = ((data <<  2) & 0xfc) | ((data >>  4) & 0x03);
				texel_lookup[0][6][((data >> 23) & 0xfe) | (~regnum & 1)] = (a << 24) | (r << 16) | (g << 8) | b;
			}
			texel_lookup_dirty[0][14] = 1;
		}
		else
		{
			int base = regnum - (nccTable+4);
			ncc_ir[0][0][base] = (INT32)(data <<  5) >> 23;
			ncc_ig[0][0][base] = (INT32)(data << 14) >> 23;
			ncc_ib[0][0][base] = (INT32)(data << 23) >> 23;
			texel_lookup_dirty[0][1] = 1;
			texel_lookup_dirty[0][9] = 1;
		}
	}
	if (chips & 4)
	{
		if (data & 0x80000000)
		{
			texel_lookup[1][5][((data >> 23) & 0xfe) | (~regnum & 1)] = 0xff000000 | data;
			texel_lookup_dirty[1][14] = 1;
		}
		else
		{
			int base = regnum - (nccTable+4);
			ncc_ir[1][0][base] = (INT32)(data <<  5) >> 23;
			ncc_ig[1][0][base] = (INT32)(data << 14) >> 23;
			ncc_ib[1][0][base] = (INT32)(data << 23) >> 23;
			texel_lookup_dirty[1][1] = 1;
			texel_lookup_dirty[1][9] = 1;
		}
	}
}


static void nccTable0qrgb_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		if (data & 0x80000000)
		{
			texel_lookup[0][5][((data >> 23) & 0xfe) | (~regnum & 1)] = 0xff000000 | data;
			{
				UINT8 a = ((data >> 16) & 0xfc) | ((data >> 22) & 0x03);
				UINT8 r = ((data >> 10) & 0xfc) | ((data >> 16) & 0x03);
				UINT8 g = ((data >>  4) & 0xfc) | ((data >> 10) & 0x03);
				UINT8 b = ((data <<  2) & 0xfc) | ((data >>  4) & 0x03);
				texel_lookup[0][6][((data >> 23) & 0xfe) | (~regnum & 1)] = (a << 24) | (r << 16) | (g << 8) | b;
			}
			texel_lookup_dirty[0][14] = 1;
		}
		else
		{
			int base = regnum - (nccTable+8);
			ncc_qr[0][0][base] = (INT32)(data <<  5) >> 23;
			ncc_qg[0][0][base] = (INT32)(data << 14) >> 23;
			ncc_qb[0][0][base] = (INT32)(data << 23) >> 23;
			texel_lookup_dirty[0][1] = 1;
			texel_lookup_dirty[0][9] = 1;
		}
	}
	if (chips & 4)
	{
		if (data & 0x80000000)
		{
			texel_lookup[1][5][((data >> 23) & 0xfe) | (~regnum & 1)] = 0xff000000 | data;
			texel_lookup_dirty[1][14] = 1;
		}
		else
		{
			int base = regnum - (nccTable+8);
			ncc_qr[1][0][base] = (INT32)(data <<  5) >> 23;
			ncc_qg[1][0][base] = (INT32)(data << 14) >> 23;
			ncc_qb[1][0][base] = (INT32)(data << 23) >> 23;
			texel_lookup_dirty[1][1] = 1;
			texel_lookup_dirty[1][9] = 1;
		}
	}
}


static void nccTable1y_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		int base = 4 * (regnum - (nccTable+12));
		ncc_y[0][1][base+0] = (data >>  0) & 0xff;
		ncc_y[0][1][base+1] = (data >>  8) & 0xff;
		ncc_y[0][1][base+2] = (data >> 16) & 0xff;
		ncc_y[0][1][base+3] = (data >> 24) & 0xff;
		texel_lookup_dirty[0][7] = 1;
		texel_lookup_dirty[0][15] = 1;
	}
	if (chips & 4)
	{
		int base = 4 * (regnum - (nccTable+12));
		ncc_y[1][1][base+0] = (data >>  0) & 0xff;
		ncc_y[1][1][base+1] = (data >>  8) & 0xff;
		ncc_y[1][1][base+2] = (data >> 16) & 0xff;
		ncc_y[1][1][base+3] = (data >> 24) & 0xff;
		texel_lookup_dirty[1][7] = 1;
		texel_lookup_dirty[1][15] = 1;
	}
}


static void nccTable1irgb_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		int base = regnum - (nccTable+16);
		ncc_ir[0][1][base] = (INT32)(data <<  5) >> 23;
		ncc_ig[0][1][base] = (INT32)(data << 14) >> 23;
		ncc_ib[0][1][base] = (INT32)(data << 23) >> 23;
		texel_lookup_dirty[0][7] = 1;
		texel_lookup_dirty[0][15] = 1;
	}
	if (chips & 4)
	{
		int base = regnum - (nccTable+16);
		ncc_ir[1][1][base] = (INT32)(data <<  5) >> 23;
		ncc_ig[1][1][base] = (INT32)(data << 14) >> 23;
		ncc_ib[1][1][base] = (INT32)(data << 23) >> 23;
		texel_lookup_dirty[1][7] = 1;
		texel_lookup_dirty[1][15] = 1;
	}
}


static void nccTable1qrgb_w(int chips, offs_t regnum, data32_t data)
{
	if (chips & 2)
	{
		int base = regnum - (nccTable+20);
		ncc_qr[0][1][base] = (INT32)(data <<  5) >> 23;
		ncc_qg[0][1][base] = (INT32)(data << 14) >> 23;
		ncc_qb[0][1][base] = (INT32)(data << 23) >> 23;
		texel_lookup_dirty[0][7] = 1;
		texel_lookup_dirty[0][15] = 1;
	}
	if (chips & 4)
	{
		int base = regnum - (nccTable+20);
		ncc_qr[1][1][base] = (INT32)(data <<  5) >> 23;
		ncc_qg[1][1][base] = (INT32)(data << 14) >> 23;
		ncc_qb[1][1][base] = (INT32)(data << 23) >> 23;
		texel_lookup_dirty[1][7] = 1;
		texel_lookup_dirty[1][15] = 1;
	}
}


static void fogTable_w(int chips, offs_t regnum, data32_t data)
{
	int base = (regnum - fogTable) * 2;
	fog_delta[base + 0] = (data >> 0) & 0xff;
	fog_blend[base + 0] = (data >> 8) & 0xff;
	fog_delta[base + 1] = (data >> 16) & 0xff;
	fog_blend[base + 1] = (data >> 24) & 0xff;
}



#ifdef __MWERKS__
#pragma mark -
#pragma mark VOODOO 1 TABLES
#endif

/*************************************
 *
 *  Voodoo 1 tables
 *
 *************************************/

static void (*voodoo_handler_w[0x100])(int chips, offs_t offset, data32_t data) =
{
	/* 0x000 */
	NULL,				NULL,				vertexAx_w,			vertexAy_w,
	vertexBx_w,			vertexBy_w,			vertexCx_w,			vertexCy_w,
	startR_w,			startG_w,			startB_w,			startZ_w,
	startA_w,			startS_w,			startT_w,			startW_w,
	/* 0x040 */
	dRdX_w,				dGdX_w,				dBdX_w,				dZdX_w,
	dAdX_w,				dSdX_w,				dTdX_w,				dWdX_w,
	dRdY_w,				dGdY_w,				dBdY_w,				dZdY_w,
	dAdY_w,				dSdY_w,				dTdY_w,				dWdY_w,
	/* 0x080 */
	triangleCMD_w,		NULL,				fvertexAx_w,		fvertexAy_w,
	fvertexBx_w,		fvertexBy_w,		fvertexCx_w,		fvertexCy_w,
	fstartR_w,			fstartG_w,			fstartB_w,			fstartZ_w,
	fstartA_w,			fstartS_w,			fstartT_w,			fstartW_w,
	/* 0x0c0 */
	fdRdX_w,			fdGdX_w,			fdBdX_w,			fdZdX_w,
	fdAdX_w,			fdSdX_w,			fdTdX_w,			fdWdX_w,
	fdRdY_w,			fdGdY_w,			fdBdY_w,			fdZdY_w,
	fdAdY_w,			fdSdY_w,			fdTdY_w,			fdWdY_w,
	/* 0x100 */
	ftriangleCMD_w,		NULL,				NULL,				NULL,
	fbzMode_w,			lfbMode_w,			clipLeftRight_w,	clipLowYHighY_w,
	nopCMD_w,			fastfillCMD_w,		swapbufferCMD_w,	NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x140 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	/* 0x180 */
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	/* 0x1c0 */
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x200 */
	NULL,				NULL,				NULL,				videoDimensions_w,
	fbiInit0_w,			NULL,				NULL,				fbiInit3_w,
	NULL,				vSync_w,			NULL,				dacData_w,
	NULL,				NULL,				NULL,				NULL,
	/* 0x240 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x280 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x2c0 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x300 */
	texture_changed_w,	texture_changed_w,	texture_changed_w,	texture_changed_w,
	texture_changed_w,	texture_changed_w,	texture_changed_w,	NULL,
	NULL,				nccTable0y_w,		nccTable0y_w,		nccTable0y_w,
	nccTable0y_w,		nccTable0irgb_w,	nccTable0irgb_w,	nccTable0irgb_w,
	/* 0x340 */
	nccTable0irgb_w,	nccTable0qrgb_w,	nccTable0qrgb_w,	nccTable0qrgb_w,
	nccTable0qrgb_w,	nccTable1y_w,		nccTable1y_w,		nccTable1y_w,
	nccTable1y_w,		nccTable1irgb_w,	nccTable1irgb_w,	nccTable1irgb_w,
	nccTable1irgb_w,	nccTable1qrgb_w,	nccTable1qrgb_w,	nccTable1qrgb_w,
	/* 0x380 */
	nccTable1qrgb_w,	NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x3c0 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL
};


static const UINT8 register_alias_map[0x40] =
{
	status,		0x004/4,	vertexAx,	vertexAy,
	vertexBx,	vertexBy,	vertexCx,	vertexCy,
	startR,		dRdX,		dRdY,		startG,
	dGdX,		dGdY,		startB,		dBdX,
	dBdY,		startZ,		dZdX,		dZdY,
	startA,		dAdX,		dAdY,		startS,
	dSdX,		dSdY,		startT,		dTdX,
	dTdY,		startW,		dWdX,		dWdY,

	triangleCMD,0x084/4,	fvertexAx,	fvertexAy,
	fvertexBx,	fvertexBy,	fvertexCx,	fvertexCy,
	fstartR,	fdRdX,		fdRdY,		fstartG,
	fdGdX,		fdGdY,		fstartB,	fdBdX,
	fdBdY,		fstartZ,	fdZdX,		fdZdY,
	fstartA,	fdAdX,		fdAdY,		fstartS,
	fdSdX,		fdSdY,		fstartT,	fdTdX,
	fdTdY,		fstartW,	fdWdX,		fdWdY
};



#ifdef __MWERKS__
#pragma mark VOODOO 2 TABLES
#endif

/*************************************
 *
 *  Voodoo 2 tables
 *
 *************************************/

static void (*voodoo2_handler_w[0x100])(int chips, offs_t offset, data32_t data) =
{
	/* 0x000 */
	NULL,				NULL,				vertexAx_w,			vertexAy_w,
	vertexBx_w,			vertexBy_w,			vertexCx_w,			vertexCy_w,
	startR_w,			startG_w,			startB_w,			startZ_w,
	startA_w,			startS_w,			startT_w,			startW_w,
	/* 0x040 */
	dRdX_w,				dGdX_w,				dBdX_w,				dZdX_w,
	dAdX_w,				dSdX_w,				dTdX_w,				dWdX_w,
	dRdY_w,				dGdY_w,				dBdY_w,				dZdY_w,
	dAdY_w,				dSdY_w,				dTdY_w,				dWdY_w,
	/* 0x080 */
	triangleCMD_w,		NULL,				fvertexAx_w,		fvertexAy_w,
	fvertexBx_w,		fvertexBy_w,		fvertexCx_w,		fvertexCy_w,
	fstartR_w,			fstartG_w,			fstartB_w,			fstartZ_w,
	fstartA_w,			fstartS_w,			fstartT_w,			fstartW_w,
	/* 0x0c0 */
	fdRdX_w,			fdGdX_w,			fdBdX_w,			fdZdX_w,
	fdAdX_w,			fdSdX_w,			fdTdX_w,			fdWdX_w,
	fdRdY_w,			fdGdY_w,			fdBdY_w,			fdZdY_w,
	fdAdY_w,			fdSdY_w,			fdTdY_w,			fdWdY_w,
	/* 0x100 */
	ftriangleCMD_w,		NULL,				NULL,				NULL,
	fbzMode_w,			lfbMode_w,			clipLeftRight_w,	clipLowYHighY_w,
	nopCMD_w,			fastfillCMD_w,		swapbufferCMD_w,	NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x140 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	/* 0x180 */
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	/* 0x1c0 */
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	fogTable_w,			fogTable_w,			fogTable_w,			fogTable_w,
	NULL,				cmdFifoBump_w,		NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x200 */
	NULL,				NULL,				NULL,				videoDimensions_w,
	fbiInit0_w,			NULL,				NULL,				fbiInit3_w,
	NULL,				vSync_w,			clutData_w,			dacData_w,
	NULL,				NULL,				NULL,				NULL,
	/* 0x240 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				sARGB_w,
	NULL,				NULL,				NULL,				NULL,
	/* 0x280 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	sDrawTriCMD_w,		sBeginTriCMD_w,		NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x2c0 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				blt_start_w,		blt_start_w,		NULL,
	NULL,				NULL,				blt_start_w,		NULL,
	/* 0x300 */
	texture_changed_w,	texture_changed_w,	texture_changed_w,	texture_changed_w,
	texture_changed_w,	texture_changed_w,	texture_changed_w,	NULL,
	NULL,				nccTable0y_w,		nccTable0y_w,		nccTable0y_w,
	nccTable0y_w,		nccTable0irgb_w,	nccTable0irgb_w,	nccTable0irgb_w,
	/* 0x340 */
	nccTable0irgb_w,	nccTable0qrgb_w,	nccTable0qrgb_w,	nccTable0qrgb_w,
	nccTable0qrgb_w,	nccTable1y_w,		nccTable1y_w,		nccTable1y_w,
	nccTable1y_w,		nccTable1irgb_w,	nccTable1irgb_w,	nccTable1irgb_w,
	nccTable1irgb_w,	nccTable1qrgb_w,	nccTable1qrgb_w,	nccTable1qrgb_w,
	/* 0x380 */
	nccTable1qrgb_w,	NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	/* 0x3c0 */
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL,
	NULL,				NULL,				NULL,				NULL
};


static const UINT8 voodoo2_cmdfifo_writethrough[0x100] =
{
	0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x000 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x040 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x080 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x0c0 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x100 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x140 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x180 */
	0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,0,	/* 0x1c0 */
	1,0,1,1,1,1,1,1, 1,1,0,1,1,1,1,1,	/* 0x200 */
	0,1,1,1,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x240 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x280 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x2c0 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x300 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x340 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,	/* 0x380 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0	/* 0x3c0 */
};



