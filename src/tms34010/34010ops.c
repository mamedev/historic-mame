/*** TMS34010: Portable TMS34010 emulator ************************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998

    Opcodes

*****************************************************************************/

/* clears flags */
#define CLR_V (V_FLAG = 0)

#define ZEXTEND(val,width) if (width) (val) &= ((unsigned int)0xffffffff>>(32-(width)))
#define EXTEND(val,width)  if (width)														  \
						   {																  \
						       (val) &= ((unsigned int)0xffffffff>>(32-(width))); 			  \
							   (val) |= (((val)&(1<<((width)-1)))?((0xffffffff)<<(width)):0); \
						   }
#define EXTEND_B(val) EXTEND(val,8)
#define EXTEND_W(val) EXTEND(val,16)
#define EXTEND_F0(val) if (FE0_FLAG) EXTEND((val),FW(0))
#define EXTEND_F1(val) if (FE1_FLAG) EXTEND((val),FW(1))

#define SIGN(val) ((val)&0x80000000)
#define SET_Z(val) (NOTZ_FLAG = (val))
#define SET_N(val) (N_FLAG = SIGN(val))
#define SET_NZ(val) {SET_Z(val); SET_N(val);}
#define SET_V_SUB(a,b,r) (V_FLAG = ((SIGN(a)!=SIGN(b))&&(SIGN(a)!=SIGN(r))))
#define SET_V_ADD(a,b,r) (V_FLAG = ((SIGN(a)==SIGN(b))&&(SIGN(a)!=SIGN(r))))
#define SET_C_SUB(a,b)  (C_FLAG = (((unsigned int)  (b)) >((unsigned int)(a))))
#define SET_C_ADD(a,b)  (C_FLAG = (((unsigned int)(~(a)))<((unsigned int)(b))))
#define SET_NZV_SUB(a,b,r)   {SET_NZ(r);SET_V_SUB(a,b,r);}
#define SET_NZCV_SUB(a,b,r)  {SET_NZ(r);SET_V_SUB(a,b,r);SET_C_SUB(a,b);}
#define SET_NZCV_ADD(a,b,r)  {SET_NZ(r);SET_V_ADD(a,b,r);SET_C_ADD(a,b);}

#define ASP_TO_BSP   (BREG(15) = AREG(15))
#define BSP_TO_ASP   (AREG(15) = BREG(15))

/* XY manipulation macros */

#define GET_X(val) ((signed short)((val) & 0xffff))
#define GET_Y(val) ((signed short)(((unsigned int)(val)) >> 16))
#define COMBINE_XY(x,y) (((unsigned int)((unsigned short)(x))) | (((unsigned int)(y)) << 16))
#define XYTOL(val) ((((int)((unsigned short)GET_Y(val)) <<state.xytolshiftcount1) | \
				    (((int)((unsigned short)GET_X(val)))<<state.xytolshiftcount2)) + OFFSET)


static void unimpl(void)
{
	PUSH(PC);
	PUSH(GET_ST());
	RESET_ST();
	PC = RLONG(0xfffffc20);
}

/* Graphics Instructions */

static void adjust_xy_to_window(void)
{
	/* Window clipping mode 3 */
	signed short  sx, sy, ex, ey;
	signed short wsx,wsy,wex,wey;
	signed short csx,csy,cex,cey;

	sx = GET_X(DADDR);
	sy = GET_Y(DADDR);
	ex = sx + GET_X(DYDX);
	ey = sy + GET_Y(DYDX);

	wsx = GET_X(WSTART);
	wsy = GET_Y(WSTART);
	wex = GET_X(WEND) + 1;
	wey = GET_Y(WEND) + 1;

	csx = (sx >= wsx) ? sx : wsx;
	csy = (sy >= wsy) ? sy : wsy;
	cex = (ex <= wex) ? ex : wex;
	cey = (ey <= wey) ? ey : wey;

	DADDR = COMBINE_XY(csx, csy);
	DYDX  = COMBINE_XY(cex - csx, cey - csy);
}

static void pixblt_b_l(void)
{
	int boundary;

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(DADDR, (RFIELD_01(SADDR) ? COLOR1 : COLOR0));
		if (--BREG(10))
		{
			// Next column
			DADDR += IOREG(REG_PSIZE);
			SADDR++;
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			DADDR = BREG(12) = BREG(12) + DPTCH;
			SADDR = BREG(13) = BREG(13) + SPTCH;
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void pixblt_b_xy(void)
{
	int boundary;
	signed short x,y;

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT B,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(XYTOL(DADDR), (RFIELD_01(SADDR) ? COLOR1 : COLOR0));
		if (--BREG(10))
		{
			// Next column
			x = GET_X(DADDR) + 1;
			y = GET_Y(DADDR);
			DADDR = COMBINE_XY(x,y);
			SADDR++;
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			x = GET_X(BREG(12));
			y = GET_Y(BREG(12)) + 1;
			DADDR = BREG(12) = COMBINE_XY(x,y);
			SADDR = BREG(13) = BREG(13) + SPTCH;
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void pixblt_l_l(void)
{
	int boundary;

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;

		BREG(14) = IOREG(REG_PSIZE);
		if (PBH)
		{
			BREG(14) = -BREG(14);
			DADDR += BREG(14);
			SADDR += BREG(14);
		}

		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(DADDR,RPIXEL(SADDR));
		if (--BREG(10))
		{
			// Next column
			DADDR += BREG(14);
			SADDR += BREG(14);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);

			if (PBV)
			{
				DADDR = BREG(12) = BREG(12) - DPTCH;
				SADDR = BREG(13) = BREG(13) - SPTCH;
			}
			else
			{
				DADDR = BREG(12) = BREG(12) + DPTCH;
				SADDR = BREG(13) = BREG(13) + SPTCH;
			}
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void pixblt_l_xy(void)
{
	int boundary;

	signed short x,y;

	if (PBH || PBV)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT L,XY  %08X - Corner Adjust not supported\n", PC);
		}
	}

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT L,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(XYTOL(DADDR),RPIXEL(SADDR));
		if (--BREG(10))
		{
			// Next column
			x = GET_X(DADDR) + 1;
			y = GET_Y(DADDR);
			DADDR = COMBINE_XY(x,y);
			SADDR += IOREG(REG_PSIZE);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			x = GET_X(BREG(12));
			y = GET_Y(BREG(12)) + 1;
			DADDR = BREG(12) = COMBINE_XY(x,y);
			SADDR = BREG(13) = BREG(13) + SPTCH;
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void pixblt_xy_l(void)
{
	int boundary;

	signed short x,y;

	if (PBH || PBV)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT XY,L  %08X - Corner Adjust not supported\n", PC);
		}
	}

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(DADDR,RPIXEL(XYTOL(SADDR)));
		if (--BREG(10))
		{
			// Next column
			DADDR += IOREG(REG_PSIZE);
			x = GET_X(SADDR) + 1;
			y = GET_Y(SADDR);
			SADDR = COMBINE_XY(x,y);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			DADDR = BREG(12) = BREG(12) + DPTCH;
			x = GET_X(BREG(13));
			y = GET_Y(BREG(13)) + 1;
			SADDR = BREG(13) = COMBINE_XY(x,y);
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void pixblt_xy_xy(void)
{
	int boundary;

	signed short x,y;

	if (PBH || PBV)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT XY,XY  %08X - Corner Adjust not supported\n", PC);
		}
	}

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT XY,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
		BREG(13) = SADDR;
	}

	do {
		boundary = WPIXEL(XYTOL(DADDR),RPIXEL(XYTOL(SADDR)));
		if (--BREG(10))
		{
			// Next column
			x = GET_X(DADDR) + 1;
			y = GET_Y(DADDR);
			DADDR = COMBINE_XY(x,y);
			x = GET_X(SADDR) + 1;
			y = GET_Y(SADDR);
			SADDR = COMBINE_XY(x,y);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			x = GET_X(BREG(12));
			y = GET_Y(BREG(12)) + 1;
			DADDR = BREG(12) = COMBINE_XY(x,y);
			x = GET_X(BREG(13));
			y = GET_Y(BREG(13)) + 1;
			SADDR = BREG(13) = COMBINE_XY(x,y);
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void fill_l(void)
{
	int boundary;

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;
	}

	do {
		boundary = WPIXEL(DADDR,COLOR1);
		if (--BREG(10))
		{
			// Next column
			DADDR += IOREG(REG_PSIZE);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			DADDR = BREG(12) = BREG(12) + DPTCH;
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void fill_xy(void)
{
	int boundary;

	signed short x,y;
	if (!P_FLAG)
	{
		switch (state.window_checking)
		{
		case 0: break;
		case 3: adjust_xy_to_window(); break;
		default:
			if (errorlog) fprintf(errorlog, "FILL XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
			break;
		}

		// Setup
		BREG(10) = (unsigned short)GET_X(DYDX);
		BREG(11) = (unsigned short)GET_Y(DYDX);
		BREG(12) = DADDR;

		if ((signed short)BREG(10)<=0 ||
			(signed short)BREG(11)<=0)
		{
			return;
		}

		P_FLAG = 1;
	}

	do {
		boundary = WPIXEL(XYTOL(DADDR),COLOR1);
		if (--BREG(10))
		{
			// Next column
			x = GET_X(DADDR) + 1;
			y = GET_Y(DADDR);
			DADDR = COMBINE_XY(x,y);
		}
		else
		{
			// Next row
			if (!--BREG(11))
			{
				//Done
				FINISH_PIX_OP;
				return;
			}
			BREG(10) = GET_X(DYDX);
			x = GET_X(BREG(12));
			y = GET_Y(BREG(12)) + 1;
			DADDR = BREG(12) = COMBINE_XY(x,y);
		}
	}
	while (!boundary);

	PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
}
static void line(void)
{
	int algorithm = state.op & 0x80;

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "LINE XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}
	}

	P_FLAG = 1;
	if (COUNT > 0)
	{
		signed short x1,x2,y1,y2,newx,newy;

		COUNT--;
		WPIXEL(XYTOL(DADDR),COLOR1);
		if (( algorithm && SADDR > 0 ) ||
			(!algorithm && SADDR >= 0))
		{
			SADDR += GET_Y(DYDX)*2 - GET_X(DYDX)*2;
			x1 = GET_X(INC1);
			y1 = GET_Y(INC1);
		}
		else
		{
			SADDR += GET_Y(DYDX)*2;
			x1 = GET_X(INC2);
			y1 = GET_Y(INC2);
		}
		x2 = GET_X(DADDR);
		y2 = GET_Y(DADDR);
		newx = x1+x2;
		newy = y1+y2;
		DADDR = COMBINE_XY(newx,newy);

		PC -= 0x10;  // Not done yet, check for interrupts and restart instruction
		return;
	}
	FINISH_PIX_OP;
}
static void add_xy_a(void)
{
	signed short x1 = GET_X(AREG(DSTREG));
	signed short y1 = GET_Y(AREG(DSTREG));
	signed short x2 = GET_X(AREG(SRCREG));
	signed short y2 = GET_Y(AREG(SRCREG));
	signed short newx = x1+x2;
	signed short newy = y1+y2;
	AREG(DSTREG) = COMBINE_XY(newx,newy);
	   N_FLAG = !newx;
	NOTZ_FLAG =  newy;
	   V_FLAG = (newx & 0x8000);
  	   C_FLAG = (newy & 0x8000);
	ASP_TO_BSP;
}
static void add_xy_b(void)
{
	signed short x1 = GET_X(BREG(DSTREG));
	signed short y1 = GET_Y(BREG(DSTREG));
	signed short x2 = GET_X(BREG(SRCREG));
	signed short y2 = GET_Y(BREG(SRCREG));
	signed short newx = x1+x2;
	signed short newy = y1+y2;
	BREG(DSTREG) = COMBINE_XY(newx,newy);
	   N_FLAG = !newx;
	NOTZ_FLAG =  newy;
	   V_FLAG = (newx & 0x8000);
	   C_FLAG = (newy & 0x8000);
	BSP_TO_ASP;
}
static void sub_xy_a(void)
{
	signed short x1 = GET_X(AREG(DSTREG));
	signed short y1 = GET_Y(AREG(DSTREG));
	signed short x2 = GET_X(AREG(SRCREG));
	signed short y2 = GET_Y(AREG(SRCREG));
	signed short newx = x1-x2;
	signed short newy = y1-y2;
	AREG(DSTREG) = COMBINE_XY(newx,newy);
	   N_FLAG = (x2 == x1);
	   C_FLAG = (y2 >  y1);
	NOTZ_FLAG = (y2 != y1);
	   V_FLAG = (x2 >  x1);
	ASP_TO_BSP;
}
static void sub_xy_b(void)
{
	signed short x1 = GET_X(BREG(DSTREG));
	signed short y1 = GET_Y(BREG(DSTREG));
	signed short x2 = GET_X(BREG(SRCREG));
	signed short y2 = GET_Y(BREG(SRCREG));
	signed short newx = x1-x2;
	signed short newy = y1-y2;
	BREG(DSTREG) = COMBINE_XY(newx,newy);
	   N_FLAG = (x2 == x1);
	   C_FLAG = (y2 >  y1);
	NOTZ_FLAG = (y2 != y1);
	   V_FLAG = (x2 >  x1);
	BSP_TO_ASP;
}
static void cmp_xy_a(void)
{
	signed short x1 = GET_X(AREG(DSTREG));
	signed short y1 = GET_Y(AREG(DSTREG));
	signed short x2 = GET_X(AREG(SRCREG));
	signed short y2 = GET_Y(AREG(SRCREG));
	signed short newx = x1-x2;
	signed short newy = y1-y2;
	   N_FLAG = !newx;
	NOTZ_FLAG =  newy;
	   V_FLAG = (newx & 0x8000);
	   C_FLAG = (newy & 0x8000);
}
static void cmp_xy_b(void)
{
	signed short x1 = GET_X(BREG(DSTREG));
	signed short y1 = GET_Y(BREG(DSTREG));
	signed short x2 = GET_X(BREG(SRCREG));
	signed short y2 = GET_Y(BREG(SRCREG));
	signed short newx = x1-x2;
	signed short newy = y1-y2;
	   N_FLAG = !newx;
	NOTZ_FLAG =  newy;
 	   V_FLAG = (newx & 0x8000);
	   C_FLAG = (newy & 0x8000);
}
static void cpw_a(void)
{
	int res = 0;
	signed short x       = GET_X(AREG(SRCREG));
	signed short y       = GET_Y(AREG(SRCREG));
	signed short wstartx = GET_X(WSTART);
	signed short wstarty = GET_Y(WSTART);
	signed short wendx   = GET_X(WEND);
	signed short wendy   = GET_Y(WEND);

	res |= ((wstartx > x) ? 0x20  : 0);
	res |= ((x > wendx)   ? 0x40  : 0);
	res |= ((wstarty > y) ? 0x80  : 0);
	res |= ((y > wendy)   ? 0x100 : 0);
	AREG(DSTREG) = V_FLAG = res;
	ASP_TO_BSP;
}
static void cpw_b(void)
{
	int res = 0;
	signed short x       = GET_X(BREG(SRCREG));
	signed short y       = GET_Y(BREG(SRCREG));
	signed short wstartx = GET_X(WSTART);
	signed short wstarty = GET_Y(WSTART);
	signed short wendx   = GET_X(WEND);
	signed short wendy   = GET_Y(WEND);

	res |= ((wstartx > x) ? 0x20  : 0);
	res |= ((x > wendx)   ? 0x40  : 0);
	res |= ((wstarty > y) ? 0x80  : 0);
	res |= ((y > wendy)   ? 0x100 : 0);
	BREG(DSTREG) = V_FLAG = res;
	BSP_TO_ASP;
}
static void cvxyl_a(void)
{
    AREG(DSTREG) = XYTOL(AREG(SRCREG));
	ASP_TO_BSP;
}
static void cvxyl_b(void)
{
    BREG(DSTREG) = XYTOL(BREG(SRCREG));
	BSP_TO_ASP;
}
static void movx_a(void)
{
	signed short x = GET_X(AREG(SRCREG));
	signed short y = GET_Y(AREG(DSTREG));
	AREG(DSTREG) = COMBINE_XY(x,y);
	ASP_TO_BSP;
}
static void movx_b(void)
{
	signed short x = GET_X(BREG(SRCREG));
	signed short y = GET_Y(BREG(DSTREG));
	BREG(DSTREG) = COMBINE_XY(x,y);
	BSP_TO_ASP;
}
static void movy_a(void)
{
	signed short x = GET_X(AREG(DSTREG));
	signed short y = GET_Y(AREG(SRCREG));
	AREG(DSTREG) = COMBINE_XY(x,y);
	ASP_TO_BSP;
}
static void movy_b(void)
{
	signed short x = GET_X(BREG(DSTREG));
	signed short y = GET_Y(BREG(SRCREG));
	BREG(DSTREG) = COMBINE_XY(x,y);
	BSP_TO_ASP;
}
static void pixt_ri_a(void)
{
	WPIXEL(AREG(DSTREG),AREG(SRCREG));
	FINISH_PIX_OP;
}
static void pixt_ri_b(void)
{
	WPIXEL(BREG(DSTREG),BREG(SRCREG));
	FINISH_PIX_OP;
}
static void pixt_rixy_a(void)
{
	if (state.window_checking && errorlog)
	{
		fprintf(errorlog, "PIXT R,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
	}

	WPIXEL(XYTOL(AREG(DSTREG)),AREG(SRCREG));
	FINISH_PIX_OP;
}
static void pixt_rixy_b(void)
{
	WPIXEL(XYTOL(BREG(DSTREG)),BREG(SRCREG));
	FINISH_PIX_OP;
}
static void pixt_ir_a(void)
{
	AREG(DSTREG) = V_FLAG = RPIXEL(AREG(SRCREG));
	ASP_TO_BSP;
	FINISH_PIX_OP;
}
static void pixt_ir_b(void)
{
	BREG(DSTREG) = V_FLAG = RPIXEL(BREG(SRCREG));
	BSP_TO_ASP;
	FINISH_PIX_OP;
}
static void pixt_ii_a(void)
{
	WPIXEL(AREG(DSTREG),RPIXEL(AREG(SRCREG)));
	FINISH_PIX_OP;
}
static void pixt_ii_b(void)
{
	WPIXEL(BREG(DSTREG),RPIXEL(BREG(SRCREG)));
	FINISH_PIX_OP;
}
static void pixt_ixyr_a(void)
{
	AREG(DSTREG) = V_FLAG = RPIXEL(XYTOL(AREG(SRCREG)));
	ASP_TO_BSP;
	FINISH_PIX_OP;
}
static void pixt_ixyr_b(void)
{
	BREG(DSTREG) = V_FLAG = RPIXEL(XYTOL(BREG(SRCREG)));
	BSP_TO_ASP;
	FINISH_PIX_OP;
}
static void pixt_ixyixy_a(void)
{
	WPIXEL(XYTOL(AREG(DSTREG)),RPIXEL(XYTOL(AREG(SRCREG))));
	FINISH_PIX_OP;
}
static void pixt_ixyixy_b(void)
{
	WPIXEL(XYTOL(BREG(DSTREG)),RPIXEL(XYTOL(BREG(SRCREG))));
	FINISH_PIX_OP;
}
static void drav_a(void)
{
	signed short x1,y1,x2,y2,newx,newy;

	if (state.window_checking && errorlog)
	{
		fprintf(errorlog, "DRAV  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
	}

	WPIXEL(XYTOL(AREG(DSTREG)),COLOR1);

	x1 = GET_X(AREG(DSTREG));
	y1 = GET_Y(AREG(DSTREG));
	x2 = GET_X(AREG(SRCREG));
	y2 = GET_Y(AREG(SRCREG));
	newx = x1+x2;
	newy = y1+y2;
	AREG(DSTREG) = COMBINE_XY(newx,newy);
	ASP_TO_BSP;
	FINISH_PIX_OP;
}
static void drav_b(void)
{
	signed short x1,y1,x2,y2,newx,newy;

	if (state.window_checking && errorlog)
	{
		fprintf(errorlog, "DARV  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
	}

	WPIXEL(XYTOL(BREG(DSTREG)),COLOR1);

	x1 = GET_X(BREG(DSTREG));
	y1 = GET_Y(BREG(DSTREG));
	x2 = GET_X(BREG(SRCREG));
	y2 = GET_Y(BREG(SRCREG));
	newx = x1+x2;
	newy = y1+y2;
	BREG(DSTREG) = COMBINE_XY(newx,newy);
	BSP_TO_ASP;
	FINISH_PIX_OP;
}


/* General Instructions */
static void abs_a(void)
{
	int r;
	r = 0 - AREG(DSTREG);
	SET_NZV_SUB(0,AREG(DSTREG),r);
	if (!N_FLAG)
	{
		AREG(DSTREG) = r;
		ASP_TO_BSP;
	}
}
static void abs_b(void)
{
	int r;
	r = 0 - BREG(DSTREG);
	SET_NZV_SUB(0,BREG(DSTREG),r);
	if (!N_FLAG)
	{
		BREG(DSTREG) = r;
		BSP_TO_ASP;
	}
}
static void add_a(void)
{
	int t,r;
	t = AREG(SRCREG);
	r = t + AREG(DSTREG);
	SET_NZCV_ADD(t,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void add_b(void)
{
	int t,r;
	t = BREG(SRCREG);
	r = t + BREG(DSTREG);
	SET_NZCV_ADD(t,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void addc_a(void)
{
	/* I'm not sure to which side the carry is added to, should
	   verify it against the examples */
	int t,r;
	t = AREG(SRCREG) + (C_FLAG?1:0);
	r = t + AREG(DSTREG);
	SET_NZCV_ADD(t,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void addc_b(void)
{
	int t,r;
	t = BREG(SRCREG) + (C_FLAG?1:0);
	r = t + BREG(DSTREG);
	SET_NZCV_ADD(t,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void addi_w_a(void)
{
	int t,r;
	t = PARAM_WORD;
	EXTEND_W(t);
	r = t + AREG(DSTREG);
	SET_NZCV_ADD(t,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void addi_w_b(void)
{
	int t,r;
	t = PARAM_WORD;
	EXTEND_W(t);
	r = t + BREG(DSTREG);
	SET_NZCV_ADD(t,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void addi_l_a(void)
{
	int t,r;
	t = PARAM_LONG();
	r = t + AREG(DSTREG);
	SET_NZCV_ADD(t,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void addi_l_b(void)
{
	int t,r;
	t = PARAM_LONG();
	r = t + BREG(DSTREG);
	SET_NZCV_ADD(t,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void addk_a(void)
{
	int t,r;
	t = PARAM_K; if (!t) t = 32;
	r = t + AREG(DSTREG);
	SET_NZCV_ADD(t,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void addk_b(void)
{
	int t,r;
	t = PARAM_K; if (!t) t = 32;
	r = t + BREG(DSTREG);
	SET_NZCV_ADD(t,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void and_a(void)
{
	AREG(DSTREG) &= AREG(SRCREG);
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void and_b(void)
{
	BREG(DSTREG) &= BREG(SRCREG);
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void andi_a(void)
{
	AREG(DSTREG) &= ~PARAM_LONG();
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void andi_b(void)
{
	BREG(DSTREG) &= ~PARAM_LONG();
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void andn_a(void)
{
	AREG(DSTREG) &= ~AREG(SRCREG);
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void andn_b(void)
{
	BREG(DSTREG) &= ~BREG(SRCREG);
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void btst_k_a(void)
{
	SET_Z(AREG(DSTREG) & (1<<(31-PARAM_K)));
}
static void btst_k_b(void)
{
	SET_Z(BREG(DSTREG) & (1<<(31-PARAM_K)));
}
static void btst_r_a(void)
{
	SET_Z(AREG(DSTREG) & (1<<(AREG(SRCREG)&0x1f)));
}
static void btst_r_b(void)
{
	SET_Z(BREG(DSTREG) & (1<<(BREG(SRCREG)&0x1f)));
}
static void clrc(void)
{
	C_FLAG = 0;
}
static void cmp_a(void)
{
	int r;
	r = AREG(DSTREG) - AREG(SRCREG);
	SET_NZCV_SUB(AREG(DSTREG),AREG(SRCREG),r);
}
static void cmp_b(void)
{
	int r;
	r = BREG(DSTREG) - BREG(SRCREG);
	SET_NZCV_SUB(BREG(DSTREG),BREG(SRCREG),r);
}
static void cmpi_w_a(void)
{
	int t,r;
	t = ~PARAM_WORD;
	EXTEND_W(t);
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
}
static void cmpi_w_b(void)
{
	int t,r;
	t = ~PARAM_WORD;
	EXTEND_W(t);
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
}
static void cmpi_l_a(void)
{
	int t,r;
	t = ~PARAM_LONG();
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
}
static void cmpi_l_b(void)
{
	int t,r;
	t = ~PARAM_LONG();
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
}
static void dint(void)
{
	IE_FLAG = 0;
}
static void divs_a(void)
{
	if (!(DSTREG & 0x01))
	{
		if (errorlog) fprintf(errorlog,"64-bit signed divides are not implemented!\n");
        unimpl();
#ifdef MAME_DEBUG
		debug_key_pressed=1;
#endif
	}
	else
	{
		if (!AREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;
			N_FLAG = 0;
		}
		else
		{
			AREG(DSTREG) /= AREG(SRCREG);
			if (AREG(DSTREG) == 0x80000000) // Can this happen in C?
			{
				N_FLAG = NOTZ_FLAG = 1;
				V_FLAG = 0;
			}
			else
			{
				SET_NZ(AREG(DSTREG));
			}
			ASP_TO_BSP;
		}
	}
}
static void divs_b(void)
{
	if (!(DSTREG & 0x01))
	{
		if (errorlog) fprintf(errorlog,"64-bit signed divides are not implemented!\n");
        unimpl();
#ifdef MAME_DEBUG
		debug_key_pressed=1;
#endif
	}
	else
	{
		if (!BREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;
			N_FLAG = 0;
		}
		else
		{
			BREG(DSTREG) /= BREG(SRCREG);
			if (BREG(DSTREG) == 0x80000000) // Can this happen in C?
			{
				N_FLAG = NOTZ_FLAG = 1;
				V_FLAG = 0;
			}
			else
			{
				SET_NZ(BREG(DSTREG));
			}
			BSP_TO_ASP;
		}
	}
}
#define TO32  (4294967296.0)
static void divu_a(void)
{
	if (!(DSTREG & 0x01))
	{
		if (!AREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;			
		}
		else
		{
			double dividend = (double)((unsigned int) AREG(DSTREG+1) + ((unsigned int) AREG(DSTREG))*TO32);
			double quotient = dividend / AREG(SRCREG);
			if (quotient >= TO32)
			{
				V_FLAG = NOTZ_FLAG = 1;
			}
			else
			{
				AREG(DSTREG)   = (unsigned int)quotient;
				AREG(DSTREG+1) = (unsigned int)(dividend - (double)AREG(SRCREG) * AREG(DSTREG));
				SET_Z(AREG(DSTREG));
				ASP_TO_BSP;
			}
		}
	}
	else
	{
		if (!AREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;
		}
		else
		{
			AREG(DSTREG) = (unsigned int)AREG(DSTREG) / (unsigned int)AREG(SRCREG);
			SET_Z(AREG(DSTREG));
			ASP_TO_BSP;
		}
	}
}
static void divu_b(void)
{
	if (!(DSTREG & 0x01))
	{
		if (!BREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;
		}
		else
		{
			double dividend = (double)((unsigned int) BREG(DSTREG+1) + ((unsigned int) BREG(DSTREG))*TO32);
			double quotient = dividend / BREG(SRCREG);
			if (quotient >= TO32)
			{
				V_FLAG = NOTZ_FLAG = 1;
			}
			else
			{
				BREG(DSTREG)   = (unsigned int)quotient;
				BREG(DSTREG+1) = (unsigned int)(dividend - (double)BREG(SRCREG) * BREG(DSTREG));
				SET_Z(BREG(DSTREG));
				BSP_TO_ASP;
			}
		}
	}
	else
	{
		if (!BREG(SRCREG))
		{
			V_FLAG = NOTZ_FLAG = 1;
		}
		else
		{
			BREG(DSTREG) = (unsigned int)BREG(DSTREG) / (unsigned int)BREG(SRCREG);
			SET_Z(BREG(DSTREG));
			BSP_TO_ASP;
		}
	}
}
static void eint(void)
{
	IE_FLAG = 1;
}
static void exgf0_a(void)
{
	int temp = (FE0_FLAG ? 0x20 : 0) | FW(0);
	FE0_FLAG = (AREG(DSTREG)&0x20);
	FW(0) = (AREG(DSTREG)&0x1f);
	SET_FW();
	AREG(DSTREG) = temp;
	ASP_TO_BSP;
}
static void exgf0_b(void)
{
	int temp = (FE0_FLAG ? 0x20 : 0) | FW(0);
	FE0_FLAG = (BREG(DSTREG)&0x20);
	FW(0) = (BREG(DSTREG)&0x1f);
	SET_FW();
	BREG(DSTREG) = temp;
	BSP_TO_ASP;
}
static void exgf1_a(void)
{
	int temp = (FE1_FLAG ? 0x20 : 0) | FW(1);
	FE1_FLAG = (AREG(DSTREG)&0x20);
	FW(1) = (AREG(DSTREG)&0x1f);
	SET_FW();
	AREG(DSTREG) = temp;
	ASP_TO_BSP;
}
static void exgf1_b(void)
{
	int temp = (FE1_FLAG ? 0x20 : 0) | FW(1);
	FE1_FLAG = (BREG(DSTREG)&0x20);
	FW(1) = (BREG(DSTREG)&0x1f);
	SET_FW();
	BREG(DSTREG) = temp;
	BSP_TO_ASP;
}
static void lmo_a(void)
{
	int r,i;
	r = AREG(SRCREG);
	SET_Z(r);
	if (r)
	{
		for	(i = 0; i < 32; i++)
		{
			if (r & 0x80000000)
			{
				AREG(DSTREG) = i;
				break;
			}

			r <<= 1;
		}
	}
	else
	{
		AREG(DSTREG) = 0;
	}
	ASP_TO_BSP;
}
static void lmo_b(void)
{
	int r,i;
	r = BREG(SRCREG);
	SET_Z(r);
	if (r)
	{
		for	(i = 0; i < 32; i++)
		{
			if (r & 0x80000000)
			{
				BREG(DSTREG) = i;
				break;
			}

			r <<= 1;
		}
	}
	else
	{
		BREG(DSTREG) = 0;
	}
	BSP_TO_ASP;
}
static void mmfm_a(void)
{
	int i;
	unsigned int l = (unsigned int) PARAM_WORD;
  	for (i = 15; i >= 0 ; i--)
	{
		if (l & 0x8000)
		{
			AREG(i) = RLONG(AREG(DSTREG));
			AREG(DSTREG)+=0x20;
		}
		l <<= 1;
	}
	ASP_TO_BSP;
}
static void mmfm_b(void)
{
	int i;
	unsigned int l = (unsigned int) PARAM_WORD;
  	for (i = 15; i >= 0 ; i--)
	{
		if (l & 0x8000)
		{
			BREG(i) = RLONG(BREG(DSTREG));
			BREG(DSTREG)+=0x20;
		}
		l <<= 1;
	}
	BSP_TO_ASP;
}
static void mmtm_a(void)
{
	int i;
	unsigned int l = (unsigned int) PARAM_WORD;
	int bitaddr = AREG(DSTREG);
	SET_N(l^0x80000000);
  	for (i = 0; i  < 16; i++)
	{
		if (l & 0x8000)
		{
			bitaddr-=0x20;
			WLONG(bitaddr,AREG(i));
			AREG(DSTREG)=bitaddr;
		}
		l <<= 1;
	}
	ASP_TO_BSP;
}
static void mmtm_b(void)
{
	int i;
	unsigned int l = (unsigned int) PARAM_WORD;
	int bitaddr = BREG(DSTREG);
	SET_N(l^0x80000000);
  	for (i = 0; i  < 16; i++)
	{
		if (l & 0x8000)
		{
			bitaddr-=0x20;
			WLONG(bitaddr,BREG(i));
			BREG(DSTREG)=bitaddr;
		}
		l <<= 1;
	}
	BSP_TO_ASP;
}
static void mods_a(void)
{
	if (AREG(SRCREG) != 0)
	{
		CLR_V;
		AREG(DSTREG) = AREG(DSTREG) % AREG(SRCREG);
		SET_Z(AREG(DSTREG));
		ASP_TO_BSP;
	}
	else
	{
		V_FLAG = 1;
	}
}
static void mods_b(void)
{
	if (BREG(SRCREG) != 0)
	{
		CLR_V;
		BREG(DSTREG) = BREG(DSTREG) % BREG(SRCREG);
		SET_Z(BREG(DSTREG));
		BSP_TO_ASP;
	}
	else
	{
		V_FLAG = 1;
	}
}
static void modu_a(void)
{
	if (AREG(SRCREG) != 0)
	{
		CLR_V;
		AREG(DSTREG) = (unsigned int)AREG(DSTREG) % (unsigned int)AREG(SRCREG);
		SET_Z(AREG(DSTREG));
		ASP_TO_BSP;
	}
	else
	{
		V_FLAG = 1;
	}
}
static void modu_b(void)
{
	if (BREG(SRCREG) != 0)
	{
		CLR_V;
		BREG(DSTREG) = (unsigned int)BREG(DSTREG) % (unsigned int)BREG(SRCREG);
		SET_Z(BREG(DSTREG));
		BSP_TO_ASP;
	}
	else
	{
		V_FLAG = 1;
	}
}
static void mpys_a(void)
{
	int m1;

	m1 = AREG(SRCREG);
	EXTEND(m1, FW(1));

	if (!(DSTREG & 0x01))
	{
		double factor1 = (double)m1;
		double factor2 = (double)AREG(DSTREG);
		double product = factor1 * factor2;
		AREG(DSTREG) = (int)(product/TO32);
		AREG(DSTREG+1) = (int)product;
		SET_NZ(AREG(DSTREG)|AREG(DSTREG+1)); /* FIXME this is not right */
		ASP_TO_BSP;
	}
	else
	{
		AREG(DSTREG) *= m1;
		SET_NZ(AREG(DSTREG));
		ASP_TO_BSP;
	}
}
static void mpys_b(void)
{
	int m1;

	m1 = BREG(SRCREG);
	EXTEND(m1, FW(1));

	if (!(DSTREG & 0x01))
	{
		double factor1 = (double)m1;
		double factor2 = (double)BREG(DSTREG);
		double product = factor1 * factor2;
		BREG(DSTREG) = (int)(product/TO32);
		BREG(DSTREG+1) = (int)product;
		SET_NZ(BREG(DSTREG)|BREG(DSTREG+1)); /* FIXME this is not right */
		BSP_TO_ASP;
	}
	else
	{
		BREG(DSTREG) *= m1;
		SET_NZ(BREG(DSTREG));
		BSP_TO_ASP;
	}
}
static void mpyu_a(void)
{
	unsigned int m1;

	m1 = AREG(SRCREG);
	ZEXTEND(m1, FW(1));

	if (!(DSTREG & 0x01))
	{
		double factor1 = (double)m1;
		double factor2 = (double)AREG(DSTREG);
		double product = factor1 * factor2;
		AREG(DSTREG) = (unsigned int)(product/TO32);
		AREG(DSTREG+1) = (unsigned int)product;
		SET_Z(AREG(DSTREG)|AREG(DSTREG+1));
		ASP_TO_BSP;
	}
	else
	{
		AREG(DSTREG) = (unsigned int)AREG(DSTREG) * m1;
		SET_Z(AREG(DSTREG));
		ASP_TO_BSP;
	}
}
static void mpyu_b(void)
{
	unsigned int m1;

	m1 = BREG(SRCREG);
	ZEXTEND(m1, FW(1));

	if (!(DSTREG & 0x01))
	{
		double factor1 = (double)m1;
		double factor2 = (double)BREG(DSTREG);
		double product = factor1 * factor2;
		BREG(DSTREG) = (unsigned int)(product/TO32);
		BREG(DSTREG+1) = (unsigned int)product;
		SET_Z(BREG(DSTREG)|BREG(DSTREG+1));
		BSP_TO_ASP;
	}
	else
	{
		BREG(DSTREG) = (unsigned int)BREG(DSTREG) * m1;
		SET_Z(BREG(DSTREG));
		BSP_TO_ASP;
	}
}
static void neg_a(void)
{
	int r;
	r = 0 - AREG(DSTREG);
	SET_NZV_SUB(0,AREG(DSTREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void neg_b(void)
{
	int r;
	r = 0 - BREG(DSTREG);
	SET_NZV_SUB(0,BREG(DSTREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void negb_a(void)
{
	int r,t;
	t = AREG(DSTREG) + (C_FLAG?1:0);
	r = 0 - t;
	SET_NZV_SUB(0,t,r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void negb_b(void)
{
	int r,t;
	t = BREG(DSTREG) + (C_FLAG?1:0);
	r = 0 - t;
	SET_NZV_SUB(0,t,r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void nop(void)
{
}
static void not_a(void)
{
	AREG(DSTREG) ^= 0xffffffff;
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void not_b(void)
{
	BREG(DSTREG) ^= 0xffffffff;
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void or_a(void)
{
	AREG(DSTREG) |= AREG(SRCREG);
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void or_b(void)
{
	BREG(DSTREG) |= BREG(SRCREG);
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void ori_a(void)
{
	AREG(DSTREG) |= PARAM_LONG();
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void ori_b(void)
{
	BREG(DSTREG) |= PARAM_LONG();
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void rl_k_a(void)
{
	int k = PARAM_K;
	if (k)
	{
		int b = ((unsigned int)AREG(DSTREG))>>(32-k);
		C_FLAG = (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		AREG(DSTREG)|=b;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void rl_r_a(void)
{
	int k = AREG(SRCREG)&0x1f;
	if (k)
	{
		int b = ((unsigned int)AREG(DSTREG))>>(32-k);
		C_FLAG = (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		AREG(DSTREG)|=b;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void rl_k_b(void)
{
	int k = PARAM_K;
	if (k)
	{
		int b = ((unsigned int)BREG(DSTREG))>>(32-k);
		C_FLAG = (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BREG(DSTREG)|=b;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void rl_r_b(void)
{
	int k = BREG(SRCREG)&0x1f;
	if (k)
	{
		int b = ((unsigned int)BREG(DSTREG))>>(32-k);
		C_FLAG = (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BREG(DSTREG)|=b;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void setc(void)
{
	C_FLAG = 1;
}
static void setf0(void)
{
	FE0_FLAG = state.op & 0x20;
	FW(0) = state.op & 0x1f;
	SET_FW();
}
static void setf1(void)
{
	FE1_FLAG = state.op & 0x20;
	FW(1) = state.op & 0x1f;
	SET_FW();
}
static void sext0_a(void)
{
	EXTEND(AREG(DSTREG),FW(0));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void sext0_b(void)
{
	EXTEND(BREG(DSTREG),FW(0));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void sext1_a(void)
{
	EXTEND(AREG(DSTREG),FW(1));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void sext1_b(void)
{
	EXTEND(BREG(DSTREG),FW(1));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void sla_k_a(void)
{
	int k = PARAM_K;
	if (k)
	{
		int res = 0;
		int mask = (0xffffffff<<(31-k))&0x7fffffff;
		if (SIGN(AREG(DSTREG))) res = mask;
		V_FLAG = ((AREG(DSTREG) & mask) != res);
		C_FLAG =  (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = V_FLAG = 0;
	}
	SET_NZ(AREG(DSTREG));
}
static void sla_k_b(void)
{
	int k = PARAM_K;
	if (k)
	{
		int res = 0;
		int mask = (0xffffffff<<(31-k))&0x7fffffff;
		if (SIGN(BREG(DSTREG))) res = mask;
		V_FLAG = ((BREG(DSTREG) & mask) != res);
		C_FLAG =  (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = V_FLAG = 0;
	}
	SET_NZ(BREG(DSTREG));
}
static void sla_r_a(void)
{
	int k = AREG(SRCREG)&0x1f;
	if (k)
	{
		int res = 0;
		int mask = (0xffffffff<<(31-k))&0x7fffffff;
		if (SIGN(AREG(DSTREG))) res = mask;
		V_FLAG = ((AREG(DSTREG) & mask) != res);
		C_FLAG =  (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = V_FLAG = 0;
	}
	SET_NZ(AREG(DSTREG));
}
static void sla_r_b(void)
{
	int k = BREG(SRCREG)&0x1f;
	if (k)
	{
		int res = 0;
		int mask = (0xffffffff<<(31-k))&0x7fffffff;
		if (SIGN(BREG(DSTREG))) res = mask;
		V_FLAG = ((BREG(DSTREG) & mask) != res);
		C_FLAG =  (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = V_FLAG = 0;
	}
	SET_NZ(BREG(DSTREG));
}
static void sll_k_a(void)
{
	int k = PARAM_K;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void sll_k_b(void)
{
	int k = PARAM_K;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void sll_r_a(void)
{
	int k = AREG(SRCREG)&0x1f;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(32-k)));
		AREG(DSTREG)<<=k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void sll_r_b(void)
{
	int k = BREG(SRCREG)&0x1f;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(32-k)));
		BREG(DSTREG)<<=k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void sra_k_a(void)
{
	int k = (32-PARAM_K) & 0x1f;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(k-1)));
		AREG(DSTREG) >>= k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_NZ(AREG(DSTREG));
}
static void sra_k_b(void)
{
	int k = (32-PARAM_K) & 0x1f;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(k-1)));
		BREG(DSTREG) >>= k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_NZ(BREG(DSTREG));
}
static void sra_r_a(void)
{
	int k = (-AREG(SRCREG)) & 0x1f;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(k-1)));
		AREG(DSTREG) >>= k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_NZ(AREG(DSTREG));
}
static void sra_r_b(void)
{
	int k = (-BREG(SRCREG)) & 0x1f;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(k-1)));
		BREG(DSTREG) >>= k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_NZ(BREG(DSTREG));
}
static void srl_k_a(void)
{
	int k = (32-PARAM_K) & 0x1f;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(k-1)));
		AREG(DSTREG) = ((unsigned int)AREG(DSTREG)) >> k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void srl_k_b(void)
{
	int k = (32-PARAM_K) & 0x1f;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(k-1)));
		BREG(DSTREG) = ((unsigned int)BREG(DSTREG)) >> k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void srl_r_a(void)
{
	int k = (-AREG(SRCREG)) & 0x1f;
	if (k)
	{
		C_FLAG = (AREG(DSTREG)&(1<<(k-1)));
		AREG(DSTREG) = ((unsigned int)AREG(DSTREG)) >> k;
		ASP_TO_BSP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(AREG(DSTREG));
}
static void srl_r_b(void)
{
	int k = (-BREG(SRCREG)) & 0x1f;
	if (k)
	{
		C_FLAG = (BREG(DSTREG)&(1<<(k-1)));
		BREG(DSTREG) = ((unsigned int)BREG(DSTREG)) >> k;
		BSP_TO_ASP;
	}
	else
	{
		C_FLAG = 0;
	}
	SET_Z(BREG(DSTREG));
}
static void sub_a(void)
{
	int r;
	r = AREG(DSTREG) - AREG(SRCREG);
	SET_NZCV_SUB(AREG(DSTREG),AREG(SRCREG),r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void sub_b(void)
{
	int r;
	r = BREG(DSTREG) - BREG(SRCREG);
	SET_NZCV_SUB(BREG(DSTREG),BREG(SRCREG),r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void subb_a(void)
{
	int r,t;
	t = AREG(SRCREG) + (C_FLAG?1:0);
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void subb_b(void)
{
	int r,t;
	t = BREG(SRCREG) + (C_FLAG?1:0);
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void subi_w_a(void)
{
	int t,r;
	t = ~PARAM_WORD;
	EXTEND_W(t);
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void subi_w_b(void)
{
	int t,r;
	t = ~PARAM_WORD;
	EXTEND_W(t);
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void subi_l_a(void)
{
	int t,r;
	t = ~PARAM_LONG();
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void subi_l_b(void)
{
	int t,r;
	t = ~PARAM_LONG();
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void subk_a(void)
{
	int t,r;
	t = PARAM_K; if (!t) t = 32;
	r = AREG(DSTREG) - t;
	SET_NZCV_SUB(AREG(DSTREG),t,r);
	AREG(DSTREG) = r;
	ASP_TO_BSP;
}
static void subk_b(void)
{
	int t,r;
	t = PARAM_K; if (!t) t = 32;
	r = BREG(DSTREG) - t;
	SET_NZCV_SUB(BREG(DSTREG),t,r);
	BREG(DSTREG) = r;
	BSP_TO_ASP;
}
static void xor_a(void)
{
	AREG(DSTREG) ^= AREG(SRCREG);
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void xor_b(void)
{
	BREG(DSTREG) ^= BREG(SRCREG);
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void xori_a(void)
{
	AREG(DSTREG) ^= PARAM_LONG();
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void xori_b(void)
{
	BREG(DSTREG) ^= PARAM_LONG();
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void zext0_a(void)
{
	ZEXTEND(AREG(DSTREG),FW(0));
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void zext0_b(void)
{
	ZEXTEND(BREG(DSTREG),FW(0));
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}
static void zext1_a(void)
{
	ZEXTEND(AREG(DSTREG),FW(1));
	SET_Z(AREG(DSTREG));
	ASP_TO_BSP;
}
static void zext1_b(void)
{
	ZEXTEND(BREG(DSTREG),FW(1));
	SET_Z(BREG(DSTREG));
	BSP_TO_ASP;
}


/* Move Instructions */
static void movi_w_a(void)
{
	AREG(DSTREG)=PARAM_WORD;
	CLR_V;
	EXTEND_W(AREG(DSTREG));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void movi_w_b(void)
{
	BREG(DSTREG)=PARAM_WORD;
	CLR_V;
	EXTEND_W(BREG(DSTREG));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void movi_l_a(void)
{
	AREG(DSTREG)=PARAM_LONG();
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void movi_l_b(void)
{
	BREG(DSTREG)=PARAM_LONG();
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void movk_a(void)
{
	int k = PARAM_K; if (!k) k = 32;
	AREG(DSTREG) = k;
	ASP_TO_BSP;
}
static void movk_b(void)
{
	int k = PARAM_K; if (!k) k = 32;
	BREG(DSTREG) = k;
	BSP_TO_ASP;
}
static void movb_rn_a(void)
{
	int bitaddr=AREG(DSTREG);
	WBYTE(bitaddr,AREG(SRCREG));
}
static void movb_rn_b(void)
{
	int bitaddr=BREG(DSTREG);
	WBYTE(bitaddr,BREG(SRCREG));
}
static void movb_nr_a(void)
{
	AREG(DSTREG) = RBYTE(AREG(SRCREG));
	CLR_V;
	EXTEND_B(AREG(DSTREG));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void movb_nr_b(void)
{
	BREG(DSTREG) = RBYTE(BREG(SRCREG));
	CLR_V;
	EXTEND_B(BREG(DSTREG));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void movb_nn_a(void)  /* could be sped by splitting function */
{
	int bitaddr=AREG(DSTREG);
	int data = RBYTE(AREG(SRCREG));
	WBYTE(bitaddr,data);
}
static void movb_nn_b(void)
{
	int bitaddr=BREG(DSTREG);
	int data = RBYTE(BREG(SRCREG));
	WBYTE(bitaddr,data);
}
static void movb_r_no_a(void)
{
	int bitaddr=AREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WBYTE(bitaddr+o,AREG(SRCREG));
}
static void movb_r_no_b(void)
{
	int bitaddr=BREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WBYTE(bitaddr+o,BREG(SRCREG));
}
static void movb_no_r_a(void)  /* could be sped by splitting function */
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	AREG(DSTREG) = RBYTE(AREG(SRCREG)+o);
	CLR_V;
	EXTEND_B(AREG(DSTREG));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void movb_no_r_b(void)
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	BREG(DSTREG) = RBYTE(BREG(SRCREG)+o);
	CLR_V;
	EXTEND_B(BREG(DSTREG));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void movb_no_no_a(void)  /* could be sped by splitting function */
{
	int bitaddr,data,o1,o2;
	o1 = PARAM_WORD;
	EXTEND_W(o1);
	data = RBYTE(AREG(SRCREG)+o1);
	o2 = PARAM_WORD;
	EXTEND_W(o2);
	bitaddr=AREG(DSTREG)+o2;
	WBYTE(bitaddr,data);
}
static void movb_no_no_b(void)
{
	int bitaddr,data,o1,o2;
	o1 = PARAM_WORD;
	EXTEND_W(o1);
	data = RBYTE(BREG(SRCREG)+o1);
	o2 = PARAM_WORD;
	EXTEND_W(o2);
	bitaddr=BREG(DSTREG)+o2;
	WBYTE(bitaddr,data);
}
static void movb_ra_a(void)
{
	int bitaddr=PARAM_LONG();
	WBYTE(bitaddr,AREG(DSTREG));
}

static void movb_ra_b(void)
{
	int bitaddr=PARAM_LONG();
	WBYTE(bitaddr,BREG(DSTREG));
}
static void movb_ar_a(void)
{
	int bitaddr=PARAM_LONG();
	AREG(DSTREG) = RBYTE(bitaddr);
	CLR_V;
	EXTEND_B(AREG(DSTREG));
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}

static void movb_ar_b(void)
{
	int bitaddr=PARAM_LONG();
	BREG(DSTREG) = RBYTE(bitaddr);
	CLR_V;
	EXTEND_B(BREG(DSTREG));
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void movb_aa(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=PARAM_LONG();
	WBYTE(bitaddrd,RBYTE(bitaddr));
}
static void move_rr_a(void)  /* could gain speed by splitting */
{
	AREG(DSTREG) = AREG(SRCREG);
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move_rr_b(void)
{
	BREG(DSTREG) = BREG(SRCREG);
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move_rr_ax(void)
{
	BREG(DSTREG) = AREG(SRCREG);
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move_rr_bx(void)
{
	AREG(DSTREG) = BREG(SRCREG);
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_rn_a(void)
{
	int bitaddr=AREG(DSTREG);
	WFIELD0(bitaddr,AREG(SRCREG));
}
static void move0_rn_b(void)
{
	int bitaddr=BREG(DSTREG);
	WFIELD0(bitaddr,BREG(SRCREG));
}
static void move1_rn_a(void)
{
	int bitaddr=AREG(DSTREG);
	WFIELD1(bitaddr,AREG(SRCREG));
}
static void move1_rn_b(void)
{
	int bitaddr=BREG(DSTREG);
	WFIELD1(bitaddr,BREG(SRCREG));
}
static void move0_r_dn_a(void)
{
	int bitaddr;
	AREG(DSTREG)-=FW_INC(0);
	bitaddr=AREG(DSTREG);
	WFIELD0(bitaddr,AREG(SRCREG));
	ASP_TO_BSP;
}
static void move0_r_dn_b(void)
{
	int bitaddr;
	BREG(DSTREG)-=FW_INC(0);
	bitaddr=BREG(DSTREG);
	WFIELD0(bitaddr,BREG(SRCREG));
	BSP_TO_ASP;
}
static void move1_r_dn_a(void)
{
	int bitaddr;
	AREG(DSTREG)-=FW_INC(1);
	bitaddr=AREG(DSTREG);
	WFIELD1(bitaddr,AREG(SRCREG));
	ASP_TO_BSP;
}
static void move1_r_dn_b(void)
{
	int bitaddr;
	BREG(DSTREG)-=FW_INC(1);
	bitaddr=BREG(DSTREG);
	WFIELD1(bitaddr,BREG(SRCREG));
	BSP_TO_ASP;
}
static void move0_r_ni_a(void)
{
	int bitaddr=AREG(DSTREG);
    WFIELD0(bitaddr,AREG(SRCREG));
    AREG(DSTREG)+=FW_INC(0);
	ASP_TO_BSP;
}
static void move0_r_ni_b(void)
{
	int bitaddr=BREG(DSTREG);
    WFIELD0(bitaddr,BREG(SRCREG));
    BREG(DSTREG)+=FW_INC(0);
	BSP_TO_ASP;
}
static void move1_r_ni_a(void)
{
	int bitaddr=AREG(DSTREG);
    WFIELD1(bitaddr,AREG(SRCREG));
    AREG(DSTREG)+=FW_INC(1);
	ASP_TO_BSP;
}
static void move1_r_ni_b(void)
{
	int bitaddr=BREG(DSTREG);
    WFIELD1(bitaddr,BREG(SRCREG));
    BREG(DSTREG)+=FW_INC(1);
	BSP_TO_ASP;
}
static void move0_nr_a(void)  /* could be sped by splitting function */
{
	AREG(DSTREG) = RFIELD0(AREG(SRCREG));
	EXTEND_F0(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_nr_b(void)
{
	BREG(DSTREG) = RFIELD0(BREG(SRCREG));
	EXTEND_F0(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move1_nr_a(void)
{
	AREG(DSTREG) = RFIELD1(AREG(SRCREG));
	EXTEND_F1(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move1_nr_b(void)
{
	BREG(DSTREG) = RFIELD1(BREG(SRCREG));
	EXTEND_F1(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move0_dn_r_a(void)  /* could be sped by splitting function */
{
	AREG(SRCREG)-=FW_INC(0);
	AREG(DSTREG) = RFIELD0(AREG(SRCREG));
	EXTEND_F0(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_dn_r_b(void)
{
	BREG(SRCREG)-=FW_INC(0);
	BREG(DSTREG) = RFIELD0(BREG(SRCREG));
	EXTEND_F0(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move1_dn_r_a(void)
{
	AREG(SRCREG)-=FW_INC(1);
	AREG(DSTREG) = RFIELD1(AREG(SRCREG));
	EXTEND_F1(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move1_dn_r_b(void)
{
	BREG(SRCREG)-=FW_INC(1);
	BREG(DSTREG) = RFIELD1(BREG(SRCREG));
	EXTEND_F1(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move0_ni_r_a(void)  /* could be sped by splitting function */
{
	AREG(DSTREG) = RFIELD0(AREG(SRCREG));
	AREG(SRCREG)+=FW_INC(0);
	EXTEND_F0(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_ni_r_b(void)
{
	BREG(DSTREG) = RFIELD0(BREG(SRCREG));
	BREG(SRCREG)+=FW_INC(0);
	EXTEND_F0(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move1_ni_r_a(void)
{
	AREG(DSTREG) = RFIELD1(AREG(SRCREG));
	AREG(SRCREG)+=FW_INC(1);
	EXTEND_F1(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move1_ni_r_b(void)
{
	BREG(DSTREG) = RFIELD1(BREG(SRCREG));
	BREG(SRCREG)+=FW_INC(1);
	EXTEND_F1(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move0_nn_a(void)  /* could be sped by splitting function */
{
	int bitaddr=AREG(DSTREG);
	int data = RFIELD0(AREG(SRCREG));
	WFIELD0(bitaddr,data);
}
static void move0_nn_b(void)
{
	int bitaddr=BREG(DSTREG);
	int data = RFIELD0(BREG(SRCREG));
	WFIELD0(bitaddr,data);
}
static void move1_nn_a(void)
{
	int bitaddr=AREG(DSTREG);
	int data = RFIELD1(AREG(SRCREG));
	WFIELD1(bitaddr,data);
}
static void move1_nn_b(void)
{
	int bitaddr=BREG(DSTREG);
	int data = RFIELD1(BREG(SRCREG));
	WFIELD1(bitaddr,data);
}
static void move0_dn_dn_a(void)  /* could be sped by splitting function */
{
	int bitaddr;
	int data;
	AREG(SRCREG)-=FW_INC(0);
	AREG(DSTREG)-=FW_INC(0);
	bitaddr=AREG(DSTREG);
	data = RFIELD0(AREG(SRCREG));
	WFIELD0(bitaddr,data);
	ASP_TO_BSP;
}
static void move0_dn_dn_b(void)
{
	int bitaddr;
	int data;
	BREG(SRCREG)-=FW_INC(0);
	BREG(DSTREG)-=FW_INC(0);
	bitaddr=BREG(DSTREG);
	data = RFIELD0(BREG(SRCREG));
	WFIELD0(bitaddr,data);
	BSP_TO_ASP;
}
static void move1_dn_dn_a(void)
{
	int bitaddr;
	int data;
	AREG(SRCREG)-=FW_INC(1);
	AREG(DSTREG)-=FW_INC(1);
	bitaddr=AREG(DSTREG);
	data = RFIELD1(AREG(SRCREG));
	WFIELD1(bitaddr,data);
	ASP_TO_BSP;
}
static void move1_dn_dn_b(void)
{
	int bitaddr;
	int data;
	BREG(SRCREG)-=FW_INC(1);
	BREG(DSTREG)-=FW_INC(1);
	bitaddr=BREG(DSTREG);
	data = RFIELD1(BREG(SRCREG));
	WFIELD1(bitaddr,data);
	BSP_TO_ASP;
}
static void move0_ni_ni_a(void)  /* could be sped by splitting function */
{
	int bitaddr=AREG(DSTREG);
	int data = RFIELD0(AREG(SRCREG));
	WFIELD0(bitaddr,data);
	AREG(SRCREG)+=FW_INC(0);
	AREG(DSTREG)+=FW_INC(0);
	ASP_TO_BSP;
}
static void move0_ni_ni_b(void)
{
	int bitaddr=BREG(DSTREG);
	int data = RFIELD0(BREG(SRCREG));
	WFIELD0(bitaddr,data);
	BREG(SRCREG)+=FW_INC(0);
	BREG(DSTREG)+=FW_INC(0);
	BSP_TO_ASP;
}
static void move1_ni_ni_a(void)
{
	int bitaddr=AREG(DSTREG);
	int data = RFIELD1(AREG(SRCREG));
	WFIELD1(bitaddr,data);
	AREG(SRCREG)+=FW_INC(1);
	AREG(DSTREG)+=FW_INC(1);
	ASP_TO_BSP;
}
static void move1_ni_ni_b(void)
{
	int bitaddr=BREG(DSTREG);
	int data = RFIELD1(BREG(SRCREG));
	WFIELD1(bitaddr,data);
	BREG(SRCREG)+=FW_INC(1);
	BREG(DSTREG)+=FW_INC(1);
	BSP_TO_ASP;
}
static void move0_r_no_a(void)
{
	int bitaddr=AREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WFIELD0(bitaddr+o,AREG(SRCREG));
}
static void move0_r_no_b(void)
{
	int bitaddr=BREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WFIELD0(bitaddr+o,BREG(SRCREG));
}
static void move1_r_no_a(void)
{
	int bitaddr=AREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WFIELD1(bitaddr+o,AREG(SRCREG));
}
static void move1_r_no_b(void)
{
	int bitaddr=BREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	WFIELD1(bitaddr+o,BREG(SRCREG));
}
static void move0_no_r_a(void)  /* could be sped by splitting function */
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	AREG(DSTREG) = RFIELD0(AREG(SRCREG)+o);
	EXTEND_F0(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_no_r_b(void)
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	BREG(DSTREG) = RFIELD0(BREG(SRCREG)+o);
	EXTEND_F0(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move1_no_r_a(void)
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	AREG(DSTREG) = RFIELD1(AREG(SRCREG)+o);
	EXTEND_F1(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move1_no_r_b(void)
{
	int o = PARAM_WORD;
	EXTEND_W(o);
	BREG(DSTREG) = RFIELD1(BREG(SRCREG)+o);
	EXTEND_F1(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move0_no_ni_a(void)  /* could be sped by splitting function */
{
	int data;
	int bitaddr=AREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	data = RFIELD0(AREG(SRCREG)+o);
	WFIELD0(bitaddr,data);
	AREG(DSTREG)+=FW_INC(0);
	ASP_TO_BSP;
}
static void move0_no_ni_b(void)
{
	int data;
	int bitaddr=BREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	data = RFIELD0(BREG(SRCREG)+o);
	WFIELD0(bitaddr,data);
	BREG(DSTREG)+=FW_INC(0);
	BSP_TO_ASP;
}
static void move1_no_ni_a(void)
{
	int data;
	int bitaddr=AREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	data = RFIELD1(AREG(SRCREG)+o);
	WFIELD1(bitaddr,data);
	AREG(DSTREG)+=FW_INC(1);
	ASP_TO_BSP;
}
static void move1_no_ni_b(void)
{
	int data;
	int bitaddr=BREG(DSTREG);
	int o = PARAM_WORD;
	EXTEND_W(o);
	data = RFIELD1(BREG(SRCREG)+o);
	WFIELD1(bitaddr,data);
	BREG(DSTREG)+=FW_INC(1);
	BSP_TO_ASP;
}
static void move0_no_no_a(void)  /* could be sped by splitting function */
{
	int data;
	int bitaddr;
	int o1 = PARAM_WORD;
	int o2 = PARAM_WORD;
	EXTEND_W(o1);
	EXTEND_W(o2);
	data = RFIELD0(AREG(SRCREG)+o1);
	bitaddr=AREG(DSTREG)+o2;
	WFIELD0(bitaddr,data);
}
static void move0_no_no_b(void)
{
	int data;
	int bitaddr;
	int o1 = PARAM_WORD;
	int o2 = PARAM_WORD;
	EXTEND_W(o1);
	EXTEND_W(o2);
	data = RFIELD0(BREG(SRCREG)+o1);
	bitaddr=BREG(DSTREG)+o2;
	WFIELD0(bitaddr,data);
}
static void move1_no_no_a(void)
{
	int data;
	int bitaddr;
	int o1 = PARAM_WORD;
	int o2 = PARAM_WORD;
	EXTEND_W(o1);
	EXTEND_W(o2);
	data = RFIELD1(AREG(SRCREG)+o1);
	bitaddr=AREG(DSTREG)+o2;
	WFIELD1(bitaddr,data);
}
static void move1_no_no_b(void)
{
	int data;
	int bitaddr;
	int o1 = PARAM_WORD;
	int o2 = PARAM_WORD;
	EXTEND_W(o1);
	EXTEND_W(o2);
	data = RFIELD1(BREG(SRCREG)+o1);
	bitaddr=BREG(DSTREG)+o2;
	WFIELD1(bitaddr,data);
}
static void move0_ra_a(void)
{
	int bitaddr=PARAM_LONG();
	WFIELD0(bitaddr,AREG(DSTREG));
}
static void move0_ra_b(void)
{
	int bitaddr=PARAM_LONG();
	WFIELD0(bitaddr,BREG(DSTREG));
}
static void move1_ra_a(void)
{
	int bitaddr=PARAM_LONG();
	WFIELD1(bitaddr,AREG(DSTREG));
}
static void move1_ra_b(void)
{
	int bitaddr=PARAM_LONG();
	WFIELD1(bitaddr,BREG(DSTREG));
}
static void move0_ar_a(void)
{
	int bitaddr=PARAM_LONG();
	AREG(DSTREG) = RFIELD0(bitaddr);
	EXTEND_F0(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move0_ar_b(void)
{
	int bitaddr=PARAM_LONG();
	BREG(DSTREG) = RFIELD0(bitaddr);
	EXTEND_F0(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move1_ar_a(void)
{
	int bitaddr=PARAM_LONG();
	AREG(DSTREG) = RFIELD1(bitaddr);
	EXTEND_F1(AREG(DSTREG));
	CLR_V;
	SET_NZ(AREG(DSTREG));
	ASP_TO_BSP;
}
static void move1_ar_b(void)
{
	int bitaddr=PARAM_LONG();
	BREG(DSTREG) = RFIELD1(bitaddr);
	EXTEND_F1(BREG(DSTREG));
	CLR_V;
	SET_NZ(BREG(DSTREG));
	BSP_TO_ASP;
}
static void move0_a_ni_a(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=AREG(DSTREG);
    WFIELD0(bitaddrd,RFIELD0(bitaddr));
    AREG(DSTREG)+=FW_INC(0);
	ASP_TO_BSP;
}
static void move0_a_ni_b(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=BREG(DSTREG);
    WFIELD0(bitaddrd,RFIELD0(bitaddr));
    BREG(DSTREG)+=FW_INC(0);
	BSP_TO_ASP;
}
static void move1_a_ni_a(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=AREG(DSTREG);
    WFIELD1(bitaddrd,RFIELD1(bitaddr));
    AREG(DSTREG)+=FW_INC(1);
	ASP_TO_BSP;
}
static void move1_a_ni_b(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=BREG(DSTREG);
    WFIELD1(bitaddrd,RFIELD1(bitaddr));
    BREG(DSTREG)+=FW_INC(1);
	BSP_TO_ASP;
}
static void move0_aa(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=PARAM_LONG();
	WFIELD0(bitaddrd,RFIELD0(bitaddr));
}
static void move1_aa(void)
{
	int bitaddr=PARAM_LONG();
	int bitaddrd=PARAM_LONG();
	WFIELD1(bitaddrd,RFIELD1(bitaddr));
}


/* Program Control and Context Switching */
static void call_a(void)
{
	int NewPC = AREG(DSTREG) & 0xfffffff0;
	PUSH(PC);
	PC = NewPC;
}
static void call_b(void)
{
	int NewPC = BREG(DSTREG) & 0xfffffff0;
	PUSH(PC);
	PC = NewPC;
}
static void callr(void)
{
	int ls = (signed short)PARAM_WORD;

	PUSH(PC);
	PC += (ls<<4);
}
static void calla(void)
{
	int address = PARAM_LONG() & 0xfffffff0;
	PUSH(PC);
	PC = address;
}
static void dsj_a(void)
{
	AREG(DSTREG)--;
	if (AREG(DSTREG))
	{
		int ls = (signed short)PARAM_WORD;
		PC += (ls<<4);
	}
	else
	{
		SKIP_WORD;
	}
	ASP_TO_BSP;
}
static void dsj_b(void)
{
	BREG(DSTREG)--;
	if (BREG(DSTREG))
	{
		int ls = (signed short)PARAM_WORD;
		PC += (ls<<4);
	}
	else
	{
		SKIP_WORD;
	}
	BSP_TO_ASP;
}
static void dsjeq_a(void)
{
	if (!NOTZ_FLAG)
	{
		AREG(DSTREG)--;
		if (AREG(DSTREG))
		{
			int ls = (signed short)PARAM_WORD;
			PC += (ls<<4);
		}
		else
		{
			SKIP_WORD;
		}
		ASP_TO_BSP;
	}
	else
	{
		SKIP_WORD;
	}
}
static void dsjeq_b(void)
{
	if (!NOTZ_FLAG)
	{
		BREG(DSTREG)--;
		if (BREG(DSTREG))
		{
			int ls = (signed short)PARAM_WORD;
			PC += (ls<<4);
		}
		else
		{
			SKIP_WORD;
		}
		BSP_TO_ASP;
	}
	else
	{
		SKIP_WORD;
	}
}
static void dsjne_a(void)
{
	if (NOTZ_FLAG)
	{
		AREG(DSTREG)--;
		if (AREG(DSTREG))
		{
			int ls = (signed short)PARAM_WORD;
			PC += (ls<<4);
		}
		else
		{
			SKIP_WORD;
		}
		ASP_TO_BSP;
	}
	else
	{
		SKIP_WORD;
	}
}
static void dsjne_b(void)
{
	if (NOTZ_FLAG)
	{
		BREG(DSTREG)--;
		if (BREG(DSTREG))
		{
			int ls = (signed short)PARAM_WORD;
			PC += (ls<<4);
		}
		else
		{
			SKIP_WORD;
		}
		BSP_TO_ASP;
	}
	else
	{
		SKIP_WORD;
	}
}
static void dsjs_a(void)
{
	int ls = PARAM_K;
	if (state.op & 0x0400) ls = -ls;
	AREG(DSTREG)--;
	if (AREG(DSTREG)) PC += (ls<<4);
	ASP_TO_BSP;
}
static void dsjs_b(void)
{
	int ls = PARAM_K;
	if (state.op & 0x0400) ls = -ls;
	BREG(DSTREG)--;
	if (BREG(DSTREG)) PC += (ls<<4);
	BSP_TO_ASP;
}
static void emu(void)
{
	/* In RUN state, this instruction is a NOP */
}
static void exgpc_a(void)
{
	int temppc = AREG(DSTREG);
	AREG(DSTREG) = PC;
	PC = temppc;
	ASP_TO_BSP;
}
static void exgpc_b(void)
{
	int temppc = BREG(DSTREG);
	BREG(DSTREG) = PC;
	PC = temppc;
	BSP_TO_ASP;
}
static void getpc_a(void)
{
	AREG(DSTREG) = PC;
	ASP_TO_BSP;
}
static void getpc_b(void)
{
	BREG(DSTREG) = PC;
	BSP_TO_ASP;
}
static void getst_a(void)
{
	AREG(DSTREG) = GET_ST();
	ASP_TO_BSP;
}
static void getst_b(void)
{
	BREG(DSTREG) = GET_ST();
	BSP_TO_ASP;
}
INLINE void j_xx_8(int take)
{
	if (DSTREG)
	{
		if (take)
		{
			signed char ls = PARAM_REL8;
			PC += (ls << 4);
		}
	}
	else
	{
		if (take)
		{
			PC = PARAM_LONG();
		}
		else
		{
			SKIP_LONG;
		}
	}
}
INLINE void j_xx_0(int take)
{
	if (DSTREG)
	{
		if (take)
		{
			signed char ls = PARAM_REL8;
			PC += (ls << 4);
		}
	}
	else
	{
		if (take)
		{
			signed short ls = (signed short) PARAM_WORD;
			PC += (ls << 4);
		}
		else
		{
			SKIP_WORD;
		}
	}
}
INLINE void j_xx_x(int take)
{
	if (take)
	{
		signed char ls = PARAM_REL8;
		PC += (ls << 4);
	}
}
static void j_UC_0(void)
{
	j_xx_0(1);
}
static void j_UC_8(void)
{
	j_xx_8(1);
}
static void j_UC_x(void)
{
	j_xx_x(1);
}
static void j_P_0(void)
{
	j_xx_0(!N_FLAG && NOTZ_FLAG);
}
static void j_P_8(void)
{
	j_xx_8(!N_FLAG && NOTZ_FLAG);
}
static void j_P_x(void)
{
	j_xx_x(!N_FLAG && NOTZ_FLAG);
}
static void j_LS_0(void)
{
	j_xx_0(C_FLAG || !NOTZ_FLAG);
}
static void j_LS_8(void)
{
	j_xx_8(C_FLAG || !NOTZ_FLAG);
}
static void j_LS_x(void)
{
	j_xx_x(C_FLAG || !NOTZ_FLAG);
}
static void j_HI_0(void)
{
	j_xx_0(!C_FLAG && NOTZ_FLAG);
}
static void j_HI_8(void)
{
	j_xx_8(!C_FLAG && NOTZ_FLAG);
}
static void j_HI_x(void)
{
	j_xx_x(!C_FLAG && NOTZ_FLAG);
}
static void j_LT_0(void)
{
	j_xx_0((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_LT_8(void)
{
	j_xx_8((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_LT_x(void)
{
	j_xx_x((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG));
}
static void j_GE_0(void)
{
	j_xx_0((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_GE_8(void)
{
	j_xx_8((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_GE_x(void)
{
	j_xx_x((N_FLAG && V_FLAG) || (!N_FLAG && !V_FLAG));
}
static void j_LE_0(void)
{
	j_xx_0((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_LE_8(void)
{
	j_xx_8((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_LE_x(void)
{
	j_xx_x((N_FLAG && !V_FLAG) || (!N_FLAG && V_FLAG) || !NOTZ_FLAG);
}
static void j_GT_0(void)
{
	j_xx_0((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_GT_8(void)
{
	j_xx_8((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_GT_x(void)
{
	j_xx_x((N_FLAG && V_FLAG && NOTZ_FLAG) || (!N_FLAG && !V_FLAG && NOTZ_FLAG));
}
static void j_C_0(void)
{
	j_xx_0(C_FLAG);
}
static void j_C_8(void)
{
	j_xx_8(C_FLAG);
}
static void j_C_x(void)
{
	j_xx_x(C_FLAG);
}
static void j_NC_0(void)
{
	j_xx_0(!C_FLAG);
}
static void j_NC_8(void)
{
	j_xx_8(!C_FLAG);
}
static void j_NC_x(void)
{
	j_xx_x(!C_FLAG);
}
static void j_EQ_0(void)
{
	j_xx_0(!NOTZ_FLAG);
}
static void j_EQ_8(void)
{
	j_xx_8(!NOTZ_FLAG);
}
static void j_EQ_x(void)
{
	j_xx_x(!NOTZ_FLAG);
}
static void j_NE_0(void)
{
	j_xx_0(NOTZ_FLAG);
}
static void j_NE_8(void)
{
	j_xx_8(NOTZ_FLAG);
}
static void j_NE_x(void)
{
	j_xx_x(NOTZ_FLAG);
}
static void j_V_0(void)
{
	j_xx_0(V_FLAG);
}
static void j_V_8(void)
{
	j_xx_8(V_FLAG);
}
static void j_V_x(void)
{
	j_xx_x(V_FLAG);
}
static void j_NV_0(void)
{
	j_xx_0(!V_FLAG);
}
static void j_NV_8(void)
{
	j_xx_8(!V_FLAG);
}
static void j_NV_x(void)
{
	j_xx_x(!V_FLAG);
}
static void j_N_0(void)
{
	j_xx_0(N_FLAG);
}
static void j_N_8(void)
{
	j_xx_8(N_FLAG);
}
static void j_N_x(void)
{
	j_xx_x(N_FLAG);
}
static void j_NN_0(void)
{
	j_xx_0(!N_FLAG);
}
static void j_NN_8(void)
{
	j_xx_8(!N_FLAG);
}
static void j_NN_x(void)
{
	j_xx_x(!N_FLAG);
}
static void jump_a(void)
{
	PC = AREG(DSTREG);
}
static void jump_b(void)
{
	PC = BREG(DSTREG);
}
static void popst(void)
{
	SET_ST(POP());
}
static void pushst(void)
{
	PUSH(GET_ST());
}
static void putst_a(void)
{
	SET_ST(AREG(DSTREG));
}
static void putst_b(void)
{
	SET_ST(BREG(DSTREG));
}
static void reti(void)
{
	SET_ST(POP());
	PC = POP();
	SET_FW();
}
static void rets(void)
{
	PC = POP();
	SP+=((PARAM_N)<<4);
}
static void rev_a(void)
{
    AREG(DSTREG) = 0x0008;
	ASP_TO_BSP;
}
static void rev_b(void)
{
    BREG(DSTREG) = 0x0008;
	BSP_TO_ASP;
}
static void trap(void)
{
	int t = PARAM_N;
	if (t)
	{
		PUSH(PC);
		PUSH(GET_ST());
	}
	RESET_ST();
	PC = RLONG(0xffffffe0-(t<<5));
}

