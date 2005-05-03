/*************************************************************************

    3dfx Voodoo lienar frame buffer access

**************************************************************************/


/*************************************
 *
 *  Function table
 *
 *************************************/

static void lfbwrite_0_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_1_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_2_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_3_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_4_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_5_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_6_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_7_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_8_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_9_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_a_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_b_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_c_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_d_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_e_argb(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_f_argb(offs_t offset, data32_t data, data32_t mem_mask);

static void lfbwrite_0_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_1_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_2_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_3_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_4_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_5_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_6_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_7_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_8_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_9_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_a_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_b_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_c_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_d_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_e_abgr(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_f_abgr(offs_t offset, data32_t data, data32_t mem_mask);

static void lfbwrite_0_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_1_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_2_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_3_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_4_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_5_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_6_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_7_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_8_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_9_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_a_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_b_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_c_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_d_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_e_rgba(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_f_rgba(offs_t offset, data32_t data, data32_t mem_mask);

static void lfbwrite_0_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_1_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_2_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_3_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_4_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_5_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_6_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_7_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_8_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_9_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_a_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_b_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_c_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_d_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_e_bgra(offs_t offset, data32_t data, data32_t mem_mask);
static void lfbwrite_f_bgra(offs_t offset, data32_t data, data32_t mem_mask);

static void (*lfbwrite[16*4])(offs_t offset, data32_t data, data32_t mem_mask) =
{
	lfbwrite_0_argb,	lfbwrite_1_argb,	lfbwrite_2_argb,	lfbwrite_3_argb,
	lfbwrite_4_argb,	lfbwrite_5_argb,	lfbwrite_6_argb,	lfbwrite_7_argb,
	lfbwrite_8_argb,	lfbwrite_9_argb,	lfbwrite_a_argb,	lfbwrite_b_argb,
	lfbwrite_c_argb,	lfbwrite_d_argb,	lfbwrite_e_argb,	lfbwrite_f_argb,

	lfbwrite_0_abgr,	lfbwrite_1_abgr,	lfbwrite_2_abgr,	lfbwrite_3_abgr,
	lfbwrite_4_abgr,	lfbwrite_5_abgr,	lfbwrite_6_abgr,	lfbwrite_7_abgr,
	lfbwrite_8_abgr,	lfbwrite_9_abgr,	lfbwrite_a_abgr,	lfbwrite_b_abgr,
	lfbwrite_c_abgr,	lfbwrite_d_abgr,	lfbwrite_e_abgr,	lfbwrite_f_abgr,

	lfbwrite_0_rgba,	lfbwrite_1_rgba,	lfbwrite_2_rgba,	lfbwrite_3_rgba,
	lfbwrite_4_rgba,	lfbwrite_5_rgba,	lfbwrite_6_rgba,	lfbwrite_7_rgba,
	lfbwrite_8_rgba,	lfbwrite_9_rgba,	lfbwrite_a_rgba,	lfbwrite_b_rgba,
	lfbwrite_c_rgba,	lfbwrite_d_rgba,	lfbwrite_e_rgba,	lfbwrite_f_rgba,

	lfbwrite_0_bgra,	lfbwrite_1_bgra,	lfbwrite_2_bgra,	lfbwrite_3_bgra,
	lfbwrite_4_bgra,	lfbwrite_5_bgra,	lfbwrite_6_bgra,	lfbwrite_7_bgra,
	lfbwrite_8_bgra,	lfbwrite_9_bgra,	lfbwrite_a_bgra,	lfbwrite_b_bgra,
	lfbwrite_c_bgra,	lfbwrite_d_bgra,	lfbwrite_e_bgra,	lfbwrite_f_bgra
};



/*************************************
 *
 *  LFB writes
 *
 *************************************/

WRITE32_HANDLER( voodoo_framebuf_w )
{
	/* if we're blocked on a swap, all writes must go into the FIFO */
	if (blocked_on_swap && memory_fifo_lfb)
	{
		add_to_memory_fifo(voodoo_framebuf_w, offset, data, mem_mask);
		return;
	}

	if (LOG_FRAMEBUFFER && offset % 0x100 == 0)
		logerror("%06X:(B=%d):voodoo_framebuf_w[%06X] = %08X & %08X\n", activecpu_get_pc(), blocked_on_swap, offset, data, ~mem_mask);
	(*lfbwrite[lfb_write_format])(offset, data, mem_mask);
}



/*************************************
 *
 *  LFB reads
 *
 *************************************/

READ32_HANDLER( voodoo_framebuf_r )
{
	UINT16 *buffer = *lfb_read_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	UINT32 result;

	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	result = buffer[y * FRAMEBUF_WIDTH + x] | (buffer[y * FRAMEBUF_WIDTH + x + 1] << 16);

	if (LOG_FRAMEBUFFER && offset % 0x100 == 0)
		logerror("%06X:voodoo_framebuf_r[%06X] = %08X & %08X\n", activecpu_get_pc(), offset, result, ~mem_mask);
	return result;
}



/*************************************
 *
 *  Mode 0 writes
 *
 *************************************/

static void lfbwrite_0_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = data;
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = data >> 16;
//  logerror("%06X:LFB write mode 0 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_0_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data >> 11) & 0x001f) | (data & 0x07e0) | ((data << 11) & 0xf800);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 27) & 0x001f) | ((data >> 16) & 0x07e0) | ((data >> 5) & 0xf800);
//  logerror("%06X:LFB write mode 0 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_0_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 0 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_0_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 0 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 1 writes
 *
 *************************************/

static void lfbwrite_1_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffe0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 15) & 0xffe0) | ((data >> 16) & 0x001f);
//  logerror("%06X:LFB write mode 1 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_1_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data >> 10) & 0x001f) | ((data << 1) & 0x07e0) | ((data << 11) & 0xf800);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 26) & 0x001f) | ((data >> 15) & 0x07e0) | ((data >> 5) & 0xf800);
//  logerror("%06X:LFB write mode 1 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_1_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 1 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_1_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 1 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 2 writes
 *
 *************************************/

static void lfbwrite_2_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffe0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 15) & 0xffe0) | ((data >> 16) & 0x001f);
//  logerror("%06X:LFB write mode 2 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_2_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 2 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_2_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 2 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_2_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 2 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 3 writes
 *
 *************************************/

static void lfbwrite_3_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 3 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_3_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 3 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_3_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 3 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_3_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 3 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 4 writes
 *
 *************************************/

static void lfbwrite_4_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	buffer[y * FRAMEBUF_WIDTH + x] = (((data >> 19) & 0x1f) << 11) | (((data >> 10) & 0x3f) << 5) | (((data >> 3) & 0x1f) << 0);
//  logerror("%06X:LFB write mode 4 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_4_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 4 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_4_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 4 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_4_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 4 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 5 writes
 *
 *************************************/

static void lfbwrite_5_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	buffer[y * FRAMEBUF_WIDTH + x] = (((data >> 19) & 0x1f) << 11) | (((data >> 10) & 0x3f) << 5) | (((data >> 3) & 0x1f) << 0);
//  logerror("%06X:LFB write mode 5 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_5_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 5 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_5_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 5 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_5_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 5 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 6 writes
 *
 *************************************/

static void lfbwrite_6_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 6 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_6_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 6 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_6_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 6 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_6_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 6 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 7 writes
 *
 *************************************/

static void lfbwrite_7_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 7 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_7_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 7 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_7_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 7 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_7_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 7 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 8 writes
 *
 *************************************/

static void lfbwrite_8_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 8 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_8_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 8 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_8_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 8 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_8_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 8 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 9 writes
 *
 *************************************/

static void lfbwrite_9_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 9 ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_9_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 9 ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_9_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 9 RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_9_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode 9 BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 10 writes
 *
 *************************************/

static void lfbwrite_a_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode a ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_a_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode a ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_a_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode a RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_a_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode a BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 11 writes
 *
 *************************************/

static void lfbwrite_b_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode b ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_b_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode b ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_b_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode b RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_b_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode b BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 12 writes
 *
 *************************************/

static void lfbwrite_c_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = data;
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
//  logerror("%06X:LFB write mode c ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_c_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data >> 11) & 0x001f) | (data & 0x07e0) | ((data << 11) & 0xf800);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
//  logerror("%06X:LFB write mode c ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_c_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode c RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_c_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode c BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 13 writes
 *
 *************************************/

static void lfbwrite_d_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffc0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
//  logerror("%06X:LFB write mode d ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_d_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode d ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_d_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode d RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_d_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode d BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 14 writes
 *
 *************************************/

static void lfbwrite_e_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffc0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
//  logerror("%06X:LFB write mode e ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_e_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode e ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_e_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode e RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_e_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode e BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *  Mode 15 writes
 *
 *************************************/

static void lfbwrite_f_argb(offs_t offset, data32_t data, data32_t mem_mask)
{
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	y &= FRAMEBUF_HEIGHT-1;
	if (ACCESSING_LSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data;
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x + 1] = data >> 16;
//  logerror("%06X:LFB write mode f ARGB @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_f_abgr(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode f ABGR @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_f_rgba(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode f RGBA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_f_bgra(offs_t offset, data32_t data, data32_t mem_mask)
{
	osd_die("%06X:Unimplementd LFB write mode f BGRA @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}
