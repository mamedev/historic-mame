/*** TMS34010: Portable TMS34010 emulator ************************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998

    Opcodes

*****************************************************************************/

/* clears flags */
#define CLR_V (V_FLAG = 0)

#define ZEXTEND(val,width) if (width) (val) &= ((UINT32)0xffffffff>>(32-(width)))
#define EXTEND(val,width)  if (width)														  \
						   {																  \
						       (val) &= ((UINT32)0xffffffff>>(32-(width))); 			 	  \
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
#define SET_C_SUB(a,b)  (C_FLAG = (((UINT32)  (b)) >((UINT32)(a))))
#define SET_C_ADD(a,b)  (C_FLAG = (((UINT32)(~(a)))<((UINT32)(b))))
#define SET_NZV_SUB(a,b,r)   {SET_NZ(r);SET_V_SUB(a,b,r);}
#define SET_NZCV_SUB(a,b,r)  {SET_NZV_SUB(a,b,r);SET_C_SUB(a,b);}
#define SET_NZCV_ADD(a,b,r)  {SET_NZ(r);SET_V_ADD(a,b,r);SET_C_ADD(a,b);}

/* XY manipulation macros */

#define GET_X(val) ((INT16)((val) & 0xffff))
#define GET_Y(val) ((INT16)(((UINT32)(val)) >> 16))
#define COMBINE_XY(x,y) (((UINT32)((UINT16)(x))) | (((UINT16)(y)) << 16))
#define XYTOL(val) ((((INT32)((UINT16)GET_Y(val)) <<state.xytolshiftcount1) | \
				    (((INT32)((UINT16)GET_X(val)))<<state.xytolshiftcount2)) + OFFSET)


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
	INT16  sx, sy, ex, ey;
	INT16 wsx,wsy,wex,wey;
	INT16 csx,csy,cex,cey;

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
	UINT32 boundary;

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;
	INT16 x,y;

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT B,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

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

		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

	INT16 x,y;

	if ((PBH || PBV) && errorlog)
	{
		fprintf(errorlog, "PIXBLT L,XY  %08X - Corner Adjust not supported\n", PC);
	}

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT L,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

	INT16 x,y;

	if ((PBH || PBV) && errorlog)
	{
		fprintf(errorlog, "PIXBLT XY,L  %08X - Corner Adjust not supported\n", PC);
	}

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

	INT16 x,y;

	if ((PBH || PBV) && errorlog)
	{
		fprintf(errorlog, "PIXBLT XY,XY  %08X - Corner Adjust not supported\n", PC);
	}

	if (!P_FLAG)
	{
		if (state.window_checking && errorlog)
		{
			fprintf(errorlog, "PIXBLT XY,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);
		}

		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

	if (!P_FLAG)
	{
		// Setup
		P_FLAG = 1;
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
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
	UINT32 boundary;

	INT16 x,y;
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
		BREG(10) = (UINT16)GET_X(DYDX);
		BREG(11) = (UINT16)GET_Y(DYDX);
		BREG(12) = DADDR;

		if ((INT16)BREG(10)<=0 ||
			(INT16)BREG(11)<=0)
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
	UINT32 algorithm = state.op & 0x80;

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
		INT16 x1,x2,y1,y2,newx,newy;

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

#define ADD_XY(R)								\
{												\
	INT16 x1 = GET_X(R##REG(DSTREG));			\
	INT16 y1 = GET_Y(R##REG(DSTREG));			\
	INT16 x2 = GET_X(R##REG(SRCREG));			\
	INT16 y2 = GET_Y(R##REG(SRCREG));			\
	INT16 newx = x1+x2;							\
	INT16 newy = y1+y2;							\
	R##REG(DSTREG) = COMBINE_XY(newx,newy);		\
	   N_FLAG = !newx;							\
	NOTZ_FLAG =  newy;							\
	   V_FLAG = (newx & 0x8000);				\
  	   C_FLAG = (newy & 0x8000);				\
	COPY_##R##SP;								\
}
static void add_xy_a(void) { ADD_XY(A); }
static void add_xy_b(void) { ADD_XY(B); }

#define SUB_XY(R)								\
{												\
	INT16 x1 = GET_X(R##REG(DSTREG));			\
	INT16 y1 = GET_Y(R##REG(DSTREG));			\
	INT16 x2 = GET_X(R##REG(SRCREG));			\
	INT16 y2 = GET_Y(R##REG(SRCREG));			\
	INT16 newx = x1-x2;							\
	INT16 newy = y1-y2;							\
	R##REG(DSTREG) = COMBINE_XY(newx,newy);		\
	   N_FLAG = (x2 == x1);						\
	   C_FLAG = (y2 >  y1);						\
	NOTZ_FLAG = (y2 != y1);						\
	   V_FLAG = (x2 >  x1);						\
	COPY_##R##SP;								\
}
static void sub_xy_a(void) { SUB_XY(A); }
static void sub_xy_b(void) { SUB_XY(B); }

#define CMP_XY(R)								\
{												\
	INT16 x1 = GET_X(R##REG(DSTREG));			\
	INT16 y1 = GET_Y(R##REG(DSTREG));			\
	INT16 x2 = GET_X(R##REG(SRCREG));			\
	INT16 y2 = GET_Y(R##REG(SRCREG));			\
	INT16 newx = x1-x2;							\
	INT16 newy = y1-y2;							\
	   N_FLAG = !newx;							\
	NOTZ_FLAG =  newy;							\
	   V_FLAG = (newx & 0x8000);				\
	   C_FLAG = (newy & 0x8000);				\
}
static void cmp_xy_a(void) { CMP_XY(A); }
static void cmp_xy_b(void) { CMP_XY(B); }

#define CPW(R)										\
{													\
	INT32 res = 0;									\
	INT16 x       = GET_X(R##REG(SRCREG));			\
	INT16 y       = GET_Y(R##REG(SRCREG));			\
	INT16 wstartx = GET_X(WSTART);					\
	INT16 wstarty = GET_Y(WSTART);					\
	INT16 wendx   = GET_X(WEND);					\
	INT16 wendy   = GET_Y(WEND);					\
													\
	res |= ((wstartx > x) ? 0x20  : 0);				\
	res |= ((x > wendx)   ? 0x40  : 0);				\
	res |= ((wstarty > y) ? 0x80  : 0);				\
	res |= ((y > wendy)   ? 0x100 : 0);				\
	R##REG(DSTREG) = V_FLAG = res;					\
	COPY_##R##SP;									\
}
static void cpw_a(void) { CPW(A); }
static void cpw_b(void) { CPW(B); }

#define CVXYL(R)									\
{													\
    R##REG(DSTREG) = XYTOL(R##REG(SRCREG));			\
	COPY_##R##SP;									\
}
static void cvxyl_a(void) { CVXYL(A); }
static void cvxyl_b(void) { CVXYL(B); }

#define MOVX(R)										\
{													\
	INT16 x = GET_X(R##REG(SRCREG));				\
	INT16 y = GET_Y(R##REG(DSTREG));				\
	R##REG(DSTREG) = COMBINE_XY(x,y);				\
	COPY_##R##SP;									\
}
static void movx_a(void) { MOVX(A); }
static void movx_b(void) { MOVX(B); }

#define MOVY(R)										\
{													\
	INT16 x = GET_X(R##REG(DSTREG));				\
	INT16 y = GET_Y(R##REG(SRCREG));				\
	R##REG(DSTREG) = COMBINE_XY(x,y);				\
	COPY_##R##SP;									\
}
static void movy_a(void) { MOVY(A); }
static void movy_b(void) { MOVY(B); }

#define PIXT_RI(R)			                        \
{							 						\
	WPIXEL(R##REG(DSTREG),R##REG(SRCREG));			\
	FINISH_PIX_OP;									\
}
static void pixt_ri_a(void) { PIXT_RI(A); }
static void pixt_ri_b(void) { PIXT_RI(B); }

#define PIXT_RIXY(R)		                        \
{													\
	if (state.window_checking && errorlog)			\
	{												\
		fprintf(errorlog, "PIXT R,XY  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);	\
	}												\
													\
	WPIXEL(XYTOL(R##REG(DSTREG)),R##REG(SRCREG));	\
	FINISH_PIX_OP;									\
}
static void pixt_rixy_a(void) { PIXT_RIXY(A); }
static void pixt_rixy_b(void) { PIXT_RIXY(B); }

#define PIXT_IR(R)			                        	\
{														\
	R##REG(DSTREG) = V_FLAG = RPIXEL(R##REG(SRCREG));	\
	COPY_##R##SP;										\
	FINISH_PIX_OP;										\
}
static void pixt_ir_a(void) { PIXT_IR(A); }
static void pixt_ir_b(void) { PIXT_IR(B); }

#define PIXT_II(R)			                       	\
{													\
	WPIXEL(R##REG(DSTREG),RPIXEL(R##REG(SRCREG)));	\
	FINISH_PIX_OP;									\
}
static void pixt_ii_a(void) { PIXT_II(A); }
static void pixt_ii_b(void) { PIXT_II(B); }

#define PIXT_IXYR(R)			              			      	\
{																\
	R##REG(DSTREG) = V_FLAG = RPIXEL(XYTOL(R##REG(SRCREG)));	\
	COPY_##R##SP;												\
	FINISH_PIX_OP;												\
}
static void pixt_ixyr_a(void) { PIXT_IXYR(A); }
static void pixt_ixyr_b(void) { PIXT_IXYR(B); }

#define PIXT_IXYIXY(R)			              			      		\
{																	\
	WPIXEL(XYTOL(R##REG(DSTREG)),RPIXEL(XYTOL(R##REG(SRCREG))));	\
	FINISH_PIX_OP;													\
}
static void pixt_ixyixy_a(void) { PIXT_IXYIXY(A); }
static void pixt_ixyixy_b(void) { PIXT_IXYIXY(B); }

#define DRAV(R)			              			      		\
{															\
	INT16 x1,y1,x2,y2,newx,newy;							\
															\
	if (state.window_checking && errorlog)					\
	{														\
		fprintf(errorlog, "DRAV  %08X - Window Checking Mode %d not supported\n", PC, state.window_checking);	\
	}														\
															\
	WPIXEL(XYTOL(R##REG(DSTREG)),COLOR1);					\
															\
	x1 = GET_X(R##REG(DSTREG));								\
	y1 = GET_Y(R##REG(DSTREG));								\
	x2 = GET_X(R##REG(SRCREG));								\
	y2 = GET_Y(R##REG(SRCREG));								\
	newx = x1+x2;											\
	newy = y1+y2;											\
	R##REG(DSTREG) = COMBINE_XY(newx,newy);					\
	COPY_##R##SP;											\
	FINISH_PIX_OP;											\
}															
static void drav_a(void) { DRAV(A); }
static void drav_b(void) { DRAV(B); }


/* General Instructions */
#define ABS(R)			              			      		\
{															\
	INT32 r;		  										\
	r = 0 - R##REG(DSTREG);									\
	SET_NZV_SUB(0,R##REG(DSTREG),r);						\
	if (!N_FLAG)											\
	{														\
		R##REG(DSTREG) = r;									\
		COPY_##R##SP;										\
	}														\
}
static void abs_a(void) { ABS(A); }
static void abs_b(void) { ABS(B); }

#define ADD(R)			              			      		\
{							 								\
	INT32 t,r;												\
	t = R##REG(SRCREG);										\
	r = t + R##REG(DSTREG);									\
	SET_NZCV_ADD(t,R##REG(DSTREG),r);						\
	R##REG(DSTREG) = r;										\
	COPY_##R##SP;											\
}
static void add_a(void) { ADD(A); }
static void add_b(void) { ADD(B); }

#define ADDC(R)			              			      		\
{			  												\
	/* I'm not sure to which side the carry is added to, should	*/	\
	/* verify it against the examples */					\
	INT32 t,r;												\
	t = R##REG(SRCREG) + (C_FLAG?1:0);						\
	r = t + R##REG(DSTREG);									\
	SET_NZCV_ADD(t,R##REG(DSTREG),r);						\
	R##REG(DSTREG) = r;										\
	COPY_##R##SP;											\
}
static void addc_a(void) { ADDC(A); }
static void addc_b(void) { ADDC(B); }

#define ADDI_W(R)			              			      	\
{			  												\
	INT32 t,r;												\
	t = PARAM_WORD;											\
	EXTEND_W(t);											\
	r = t + R##REG(DSTREG);									\
	SET_NZCV_ADD(t,R##REG(DSTREG),r);						\
	R##REG(DSTREG) = r;										\
	COPY_##R##SP;											\
}
static void addi_w_a(void) { ADDI_W(A); }
static void addi_w_b(void) { ADDI_W(B); }

#define ADDI_L(R)			              			      	\
{			  												\
	INT32 t,r;												\
	t = PARAM_LONG();										\
	r = t + R##REG(DSTREG);									\
	SET_NZCV_ADD(t,R##REG(DSTREG),r);						\
	R##REG(DSTREG) = r;										\
	COPY_##R##SP;											\
}
static void addi_l_a(void) { ADDI_L(A); }
static void addi_l_b(void) { ADDI_L(B); }

#define ADDK(R)				              			      	\
{			  												\
	INT32 t,r;												\
	t = PARAM_K; if (!t) t = 32;							\
	r = t + R##REG(DSTREG);									\
	SET_NZCV_ADD(t,R##REG(DSTREG),r);						\
	R##REG(DSTREG) = r;										\
	COPY_##R##SP;											\
}
static void addk_a(void) { ADDK(A); }
static void addk_b(void) { ADDK(B); }

#define AND(R)				              			      	\
{			  												\
	R##REG(DSTREG) &= R##REG(SRCREG);						\
	SET_Z(R##REG(DSTREG));									\
	COPY_##R##SP;											\
}
static void and_a(void) { AND(A); }
static void and_b(void) { AND(B); }

#define ANDI(R)				              			      	\
{			  												\
	R##REG(DSTREG) &= ~PARAM_LONG();						\
	SET_Z(R##REG(DSTREG));									\
	COPY_##R##SP;											\
}
static void andi_a(void) { ANDI(A); }
static void andi_b(void) { ANDI(B); }

#define ANDN(R)				              			      	\
{			  												\
	R##REG(DSTREG) &= ~R##REG(SRCREG);						\
	SET_Z(R##REG(DSTREG));									\
	COPY_##R##SP;											\
}
static void andn_a(void) { ANDN(A); }
static void andn_b(void) { ANDN(B); }

#define BTST_K(R)				              			    \
{							 								\
	SET_Z(R##REG(DSTREG) & (1<<(31-PARAM_K)));				\
}
static void btst_k_a(void) { BTST_K(A); }
static void btst_k_b(void) { BTST_K(B); }

#define BTST_R(R)				              			    \
{															\
	SET_Z(R##REG(DSTREG) & (1<<(R##REG(SRCREG)&0x1f)));		\
}
static void btst_r_a(void) { BTST_R(A); }
static void btst_r_b(void) { BTST_R(B); }

static void clrc(void)
{
	C_FLAG = 0;
}

#define CMP(R)				       		       			    \
{															\
	INT32 r;												\
	r = R##REG(DSTREG) - R##REG(SRCREG);					\
	SET_NZCV_SUB(R##REG(DSTREG),R##REG(SRCREG),r);			\
}
static void cmp_a(void) { CMP(A); }
static void cmp_b(void) { CMP(B); }

#define CMPI_W(R)			       		       			    \
{															\
	INT32 t,r;												\
	t = ~PARAM_WORD;										\
	EXTEND_W(t);											\
	r = R##REG(DSTREG) - t;									\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);						\
}
static void cmpi_w_a(void) { CMPI_W(A); }
static void cmpi_w_b(void) { CMPI_W(B); }

#define CMPI_L(R)			       		       			    \
{															\
	INT32 t,r;												\
	t = ~PARAM_LONG();										\
	r = R##REG(DSTREG) - t;									\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);						\
}
static void cmpi_l_a(void) { CMPI_L(A); }
static void cmpi_l_b(void) { CMPI_L(B); }

static void dint(void)
{
	IE_FLAG = 0;
}

#define DIVS(R)			       		       			    	\
{															\
	V_FLAG = N_FLAG = 0;									\
	NOTZ_FLAG = 1;											\
	if (!(DSTREG & 0x01))									\
	{														\
		if (!R##REG(SRCREG))								\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			INT64 dividend  = COMBINE_64_32_32(R##REG(DSTREG), R##REG(DSTREG+1));	\
			INT64 quotient  = DIV_64_64_32(dividend, R##REG(SRCREG)); 				\
			INT32 remainder = MOD_32_64_32(dividend, R##REG(SRCREG)); 				\
			UINT32 signbits = ((quotient & 0x80000000) ? 0xffffffff : 0); 			\
			if (HI32_32_64(quotient) != signbits)			\
			{												\
				V_FLAG = 1;									\
			}												\
			else											\
			{												\
				R##REG(DSTREG)   = quotient;				\
				R##REG(DSTREG+1) = remainder;				\
				SET_NZ(R##REG(DSTREG));						\
				COPY_##R##SP;								\
			}												\
		}													\
	}														\
	else													\
	{														\
		if (!R##REG(SRCREG))								\
		{													\
			V_FLAG = 1;										\
		}													\
		else												\
		{													\
			R##REG(DSTREG) /= R##REG(SRCREG);				\
			SET_NZ(R##REG(DSTREG));							\
			COPY_##R##SP;									\
		}													\
	}														\
}
static void divs_a(void) { DIVS(A); }
static void divs_b(void) { DIVS(B); }

#define DIVU(R)			       		       			    						\
{										  										\
	V_FLAG = 0;																	\
	NOTZ_FLAG = 1;																\
	if (!(DSTREG & 0x01))														\
	{																			\
		if (!R##REG(SRCREG))													\
		{																		\
			V_FLAG = 1;															\
		}																		\
		else																	\
		{																		\
			UINT64 dividend  = COMBINE_U64_U32_U32(R##REG(DSTREG), R##REG(DSTREG+1)); \
			UINT64 quotient  = DIV_U64_U64_U32(dividend, R##REG(SRCREG)); 		\
			UINT32 remainder = MOD_U32_U64_U32(dividend, R##REG(SRCREG)); 		\
			if (HI32_U32_U64(quotient) != 0)									\
			{																	\
				V_FLAG = 1;														\
			}																	\
			else																\
			{																	\
				R##REG(DSTREG)   = quotient;									\
				R##REG(DSTREG+1) = remainder;									\
				SET_Z(R##REG(DSTREG));											\
				COPY_##R##SP;													\
			}																	\
		}																		\
	}																			\
	else																		\
	{																			\
		if (!R##REG(SRCREG))													\
		{																		\
			V_FLAG = 1;															\
		}																		\
		else																	\
		{																		\
			R##REG(DSTREG) = (UINT32)R##REG(DSTREG) / (UINT32)R##REG(SRCREG);  	\
			SET_Z(R##REG(DSTREG));												\
			COPY_##R##SP;														\
		}																		\
	}																			\
}
static void divu_a(void) { DIVU(A); }
static void divu_b(void) { DIVU(B); }

static void eint(void)
{
	IE_FLAG = 1;
}

#define EXGF(F,R)			       		       			    	\
{																\
	UINT32 temp = (FE##F##_FLAG ? 0x20 : 0) | FW(F);			\
	FE##F##_FLAG = (R##REG(DSTREG)&0x20);						\
	FW(F) = (R##REG(DSTREG)&0x1f);								\
	SET_FW();													\
	R##REG(DSTREG) = temp;										\
	COPY_##R##SP;												\
}
static void exgf0_a(void) { EXGF(0,A); }
static void exgf0_b(void) { EXGF(0,B); }
static void exgf1_a(void) { EXGF(1,A); }
static void exgf1_b(void) { EXGF(1,B); }

#define LMO(R)			       		       			    		\
{																\
	UINT32 r,i;													\
	r = R##REG(SRCREG);											\
	SET_Z(r);													\
	if (r)														\
	{															\
		for	(i = 0; i < 32; i++)								\
		{														\
			if (r & 0x80000000)									\
			{													\
				R##REG(DSTREG) = i;								\
				break;											\
			}													\
			r <<= 1;											\
		}														\
	}															\
	else														\
	{															\
		R##REG(DSTREG) = 0;										\
	}															\
	COPY_##R##SP;												\
}
static void lmo_a(void) { LMO(A); }
static void lmo_b(void) { LMO(B); }

#define MMFM(R)			       		       			    		\
{																\
	INT32 i;													\
	UINT32 l = (UINT32) PARAM_WORD;								\
  	for (i = 15; i >= 0 ; i--)									\
	{															\
		if (l & 0x8000)											\
		{														\
			R##REG(i) = RLONG(R##REG(DSTREG));					\
			R##REG(DSTREG)+=0x20;								\
		}														\
		l <<= 1;												\
	}															\
	COPY_##R##SP;												\
}
static void mmfm_a(void) { MMFM(A); }
static void mmfm_b(void) { MMFM(B); }

#define MMTM(R)			       		       			    		\
{			  													\
	UINT32 i;													\
	UINT32 l = (UINT32) PARAM_WORD;								\
	UINT32 bitaddr = R##REG(DSTREG);							\
	SET_N(bitaddr^0x80000000);									\
  	for (i = 0; i  < 16; i++)									\
	{															\
		if (l & 0x8000)											\
		{														\
			bitaddr-=0x20;										\
			WLONG(bitaddr,R##REG(i));							\
			R##REG(DSTREG)=bitaddr;								\
		}														\
		l <<= 1;												\
	}															\
	COPY_##R##SP;												\
}
static void mmtm_a(void) { MMTM(A); }
static void mmtm_b(void) { MMTM(B); }

#define MODS(R)			       		       			    		\
{				  												\
	if (R##REG(SRCREG) != 0)									\
	{															\
		CLR_V;													\
		R##REG(DSTREG) = R##REG(DSTREG) % R##REG(SRCREG);		\
		SET_Z(R##REG(DSTREG));									\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		V_FLAG = 1;												\
	}															\
}
static void mods_a(void) { MODS(A); }
static void mods_b(void) { MODS(B); }

#define MODU(R)			       		       			    		\
{				  												\
	if (R##REG(SRCREG) != 0)									\
	{															\
		CLR_V;													\
		R##REG(DSTREG) = (UINT32)R##REG(DSTREG) % (UINT32)R##REG(SRCREG);	\
		SET_Z(R##REG(DSTREG));									\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		V_FLAG = 1;												\
	}															\
}
static void modu_a(void) { MODU(A); }
static void modu_b(void) { MODU(B); }

#define MPYS(R)			       		       			    		\
{																\
	INT32 m1;													\
																\
	m1 = R##REG(SRCREG);										\
	EXTEND(m1, FW(1));											\
																\
	if (!(DSTREG & 0x01))										\
	{															\
		INT64 product = MUL_64_32_32(m1, R##REG(DSTREG));		\
		R##REG(DSTREG)   = HI32_32_64(product);					\
		R##REG(DSTREG+1) = LO32_32_64(product);					\
		SET_Z(product!=0);										\
		SET_N(R##REG(DSTREG));									\
	}															\
	else														\
	{															\
		R##REG(DSTREG) *= m1;									\
		SET_NZ(R##REG(DSTREG));									\
	}															\
	COPY_##R##SP;												\
}
static void mpys_a(void) { MPYS(A); }
static void mpys_b(void) { MPYS(B); }

#define MPYU(R)			       		       			    		\
{				  												\
	UINT32 m1;													\
																\
	m1 = R##REG(SRCREG);										\
	ZEXTEND(m1, FW(1));											\
																\
	if (!(DSTREG & 0x01))										\
	{															\
		UINT64 product = MUL_U64_U32_U32(m1, R##REG(DSTREG));	\
		R##REG(DSTREG)   = HI32_U32_U64(product);				\
		R##REG(DSTREG+1) = LO32_U32_U64(product);				\
		SET_Z(product!=0);										\
	}															\
	else														\
	{															\
		R##REG(DSTREG) = (UINT32)R##REG(DSTREG) * m1;			\
		SET_Z(R##REG(DSTREG));									\
	}															\
	COPY_##R##SP;												\
}
static void mpyu_a(void) { MPYU(A); }
static void mpyu_b(void) { MPYU(B); }

#define NEG(R)			       		       			    		\
{			  													\
	INT32 r;													\
	r = 0 - R##REG(DSTREG);										\
	SET_NZCV_SUB(0,R##REG(DSTREG),r);							\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void neg_a(void) { NEG(A); }
static void neg_b(void) { NEG(B); }

#define NEGB(R)			       		       			    		\
{			  													\
	INT32 r,t;													\
	t = R##REG(DSTREG) + (C_FLAG?1:0);							\
	r = 0 - t;													\
	SET_NZCV_SUB(0,t,r);										\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void negb_a(void) { NEGB(A); }
static void negb_b(void) { NEGB(B); }

static void nop(void)
{
}

#define NOT(R)			       		       			    		\
{								 								\
	R##REG(DSTREG) ^= 0xffffffff;								\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void not_a(void) { NOT(A); }
static void not_b(void) { NOT(B); }

#define OR(R)			       		       			    		\
{			  													\
	R##REG(DSTREG) |= R##REG(SRCREG);							\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void or_a(void) { OR(A); }
static void or_b(void) { OR(B); }

#define ORI(R)			       		       			    		\
{			  													\
	R##REG(DSTREG) |= PARAM_LONG();								\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void ori_a(void) { ORI(A); }
static void ori_b(void) { ORI(B); }

#define RL(R,K)			       		       			    		\
{			 													\
	UINT32 k = K;												\
	if (k)														\
	{															\
		UINT32 b = ((UINT32)R##REG(DSTREG))>>(32-k);			\
		C_FLAG = (R##REG(DSTREG)&(1<<(32-k)));					\
		R##REG(DSTREG)<<=k;										\
		R##REG(DSTREG)|=b;										\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_Z(R##REG(DSTREG));										\
}
static void rl_k_a(void) { RL(A,PARAM_K); }
static void rl_k_b(void) { RL(B,PARAM_K); }
static void rl_r_a(void) { RL(A,AREG(SRCREG)&0x1f); }
static void rl_r_b(void) { RL(B,BREG(SRCREG)&0x1f); }

static void setc(void)
{
	C_FLAG = 1;
}

#define SETF(F)													\
{																\
	FE##F##_FLAG = state.op & 0x20;								\
	FW(F) = state.op & 0x1f;									\
	SET_FW();													\
}
static void setf0(void) { SETF(0); }
static void setf1(void) { SETF(1); }

#define SEXT(F,R)												\
{							   									\
	EXTEND(R##REG(DSTREG),FW(F));								\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void sext0_a(void) { SEXT(0,A); }
static void sext0_b(void) { SEXT(0,B); }
static void sext1_a(void) { SEXT(1,A); }
static void sext1_b(void) { SEXT(1,B); }

#define SLA(R,K)												\
{				 												\
	UINT32 k = K;												\
	if (k)														\
	{															\
		INT32 res = 0;											\
		UINT32 mask = (0xffffffff<<(31-k))&0x7fffffff;			\
		if (SIGN(R##REG(DSTREG))) res = mask;					\
		V_FLAG = ((R##REG(DSTREG) & mask) != res);				\
		C_FLAG =  (R##REG(DSTREG)&(1<<(32-k)));					\
		R##REG(DSTREG)<<=k;										\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		C_FLAG = V_FLAG = 0;									\
	}															\
	SET_NZ(R##REG(DSTREG));										\
}
static void sla_k_a(void) { SLA(A,PARAM_K); }
static void sla_k_b(void) { SLA(B,PARAM_K); }
static void sla_r_a(void) { SLA(A,AREG(SRCREG)&0x1f); }
static void sla_r_b(void) { SLA(B,BREG(SRCREG)&0x1f); }

#define SLL(R,K)												\
{			 													\
	UINT32 k = K;													\
	if (k)														\
	{															\
		C_FLAG = (R##REG(DSTREG)&(1<<(32-k)));					\
		R##REG(DSTREG)<<=k;										\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_Z(R##REG(DSTREG));										\
}
static void sll_k_a(void) { SLL(A,PARAM_K); }
static void sll_k_b(void) { SLL(B,PARAM_K); }
static void sll_r_a(void) { SLL(A,AREG(SRCREG)&0x1f); }
static void sll_r_b(void) { SLL(B,BREG(SRCREG)&0x1f); }

#define SRA(R,K)												\
{			  													\
	UINT32 k = (32-(K)) & 0x1f;									\
	if (k)														\
	{															\
		C_FLAG = (R##REG(DSTREG)&(1<<(k-1)));					\
		R##REG(DSTREG) >>= k;									\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_NZ(R##REG(DSTREG));										\
}
static void sra_k_a(void) { SRA(A,PARAM_K); }
static void sra_k_b(void) { SRA(B,PARAM_K); }
static void sra_r_a(void) { SRA(A,AREG(SRCREG)); }
static void sra_r_b(void) { SRA(B,BREG(SRCREG)); }

#define SRL(R,K)												\
{																\
	UINT32 k = (32-(K)) & 0x1f;									\
	if (k)														\
	{															\
		C_FLAG = (R##REG(DSTREG)&(1<<(k-1)));					\
		R##REG(DSTREG) = ((UINT32)R##REG(DSTREG)) >> k;			\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		C_FLAG = 0;												\
	}															\
	SET_Z(R##REG(DSTREG));										\
}
static void srl_k_a(void) { SRL(A,PARAM_K); }
static void srl_k_b(void) { SRL(B,PARAM_K); }
static void srl_r_a(void) { SRL(A,AREG(SRCREG)); }
static void srl_r_b(void) { SRL(B,BREG(SRCREG)); }

#define SUB(R)			       		       			    		\
{			  													\
	INT32 r;													\
	r = R##REG(DSTREG) - R##REG(SRCREG);						\
	SET_NZCV_SUB(R##REG(DSTREG),R##REG(SRCREG),r);				\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void sub_a(void) { SUB(A); }
static void sub_b(void) { SUB(B); }

#define SUBB(R)			       		       			    		\
{			  													\
	INT32 r,t;													\
	t = R##REG(SRCREG) + (C_FLAG?1:0);							\
	r = R##REG(DSTREG) - t;										\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);							\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void subb_a(void) { SUBB(A); }
static void subb_b(void) { SUBB(B); }

#define SUBI_W(R)			       		       			    	\
{			  													\
	INT32 t,r;													\
	t = ~PARAM_WORD;											\
	EXTEND_W(t);												\
	r = R##REG(DSTREG) - t;										\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);							\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void subi_w_a(void) { SUBI_W(A); }
static void subi_w_b(void) { SUBI_W(B); }

#define SUBI_L(R)			       		       			    	\
{			  													\
	INT32 t,r;													\
	t = ~PARAM_LONG();											\
	r = R##REG(DSTREG) - t;										\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);							\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void subi_l_a(void) { SUBI_L(A); }
static void subi_l_b(void) { SUBI_L(B); }

#define SUBK(R)			       		       			    		\
{			  													\
	INT32 t,r;													\
	t = PARAM_K; if (!t) t = 32;								\
	r = R##REG(DSTREG) - t;										\
	SET_NZCV_SUB(R##REG(DSTREG),t,r);							\
	R##REG(DSTREG) = r;											\
	COPY_##R##SP;												\
}
static void subk_a(void) { SUBK(A); }
static void subk_b(void) { SUBK(B); }

#define XOR(R)			       		       			    		\
{			  													\
	R##REG(DSTREG) ^= R##REG(SRCREG);							\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void xor_a(void) { XOR(A); }
static void xor_b(void) { XOR(B); }

#define XORI(R)			       		       			    		\
{			  													\
	R##REG(DSTREG) ^= PARAM_LONG();								\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void xori_a(void) { XORI(A); }
static void xori_b(void) { XORI(B); }

#define ZEXT(F,R)												\
{																\
	ZEXTEND(R##REG(DSTREG),FW(F));								\
	SET_Z(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void zext0_a(void) { ZEXT(0,A); }
static void zext0_b(void) { ZEXT(0,B); }
static void zext1_a(void) { ZEXT(1,A); }
static void zext1_b(void) { ZEXT(1,B); }


/* Move Instructions */
#define MOVI_W(R)		       		       			    		\
{			  													\
	R##REG(DSTREG)=PARAM_WORD;									\
	CLR_V;														\
	EXTEND_W(R##REG(DSTREG));									\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void movi_w_a(void) { MOVI_W(A); }
static void movi_w_b(void) { MOVI_W(B); }

#define MOVI_L(R)		       		       			    		\
{			  													\
	R##REG(DSTREG)=PARAM_LONG();								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void movi_l_a(void) { MOVI_L(A); }
static void movi_l_b(void) { MOVI_L(B); }

#define MOVK(R)		       		       			    			\
{																\
	INT32 k = PARAM_K; if (!k) k = 32;							\
	R##REG(DSTREG) = k;											\
	COPY_##R##SP;												\
}
static void movk_a(void) { MOVK(A); }
static void movk_b(void) { MOVK(B); }

#define MOVB_RN(R)		       		       			    		\
{																\
	WBYTE(R##REG(DSTREG),R##REG(SRCREG));						\
}
static void movb_rn_a(void) { MOVB_RN(A); }
static void movb_rn_b(void) { MOVB_RN(B); }

#define MOVB_NR(R)		       		       			    		\
{			  													\
	R##REG(DSTREG) = RBYTE(R##REG(SRCREG));						\
	CLR_V;														\
	EXTEND_B(R##REG(DSTREG));									\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void movb_nr_a(void) { MOVB_NR(A); }
static void movb_nr_b(void) { MOVB_NR(B); }

#define MOVB_NN(R)												\
{																\
	WBYTE(R##REG(DSTREG),RBYTE(R##REG(SRCREG)));				\
}
static void movb_nn_a(void) { MOVB_NN(A); }
static void movb_nn_b(void) { MOVB_NN(B); }

#define MOVB_R_NO(R)	       		       			    		\
{							  									\
	INT32 o = PARAM_WORD;										\
	EXTEND_W(o);												\
	WBYTE(R##REG(DSTREG)+o,R##REG(SRCREG));						\
}
static void movb_r_no_a(void) { MOVB_R_NO(A); }
static void movb_r_no_b(void) { MOVB_R_NO(B); }

#define MOVB_NO_R(R)	       		       			    		\
{			  													\
	INT32 o = PARAM_WORD;										\
	EXTEND_W(o);												\
	R##REG(DSTREG) = RBYTE(R##REG(SRCREG)+o);					\
	CLR_V;														\
	EXTEND_B(R##REG(DSTREG));									\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void movb_no_r_a(void) { MOVB_NO_R(A); }
static void movb_no_r_b(void) { MOVB_NO_R(B); }

#define MOVB_NO_NO(R)	       		       			    		\
{																\
	INT32 o1,o2;												\
	o1 = PARAM_WORD;											\
	EXTEND_W(o1);												\
	o2 = PARAM_WORD;											\
	EXTEND_W(o2);												\
	WBYTE(R##REG(DSTREG)+o2,RBYTE(R##REG(SRCREG)+o1));			\
}
static void movb_no_no_a(void) { MOVB_NO_NO(A); }
static void movb_no_no_b(void) { MOVB_NO_NO(B); }

#define MOVB_RA(R)	       		       			    			\
{																\
	WBYTE(PARAM_LONG(),R##REG(DSTREG));							\
}
static void movb_ra_a(void) { MOVB_RA(A); }
static void movb_ra_b(void) { MOVB_RA(B); }

#define MOVB_AR(R)	       		       			    			\
{			  													\
	R##REG(DSTREG) = RBYTE(PARAM_LONG());						\
	CLR_V;														\
	EXTEND_B(R##REG(DSTREG));									\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void movb_ar_a(void) { MOVB_AR(A); }
static void movb_ar_b(void) { MOVB_AR(B); }

static void movb_aa(void)
{
	UINT32 bitaddrs=PARAM_LONG();
	WBYTE(PARAM_LONG(),RBYTE(bitaddrs));
}

#define MOVE_RR(RS,RD)	       		       			    		\
{																\
	RD##REG(DSTREG) = RS##REG(SRCREG);							\
	CLR_V;														\
	SET_NZ(RD##REG(DSTREG));									\
	COPY_##RD##SP;												\
}
static void move_rr_a (void) { MOVE_RR(A,A); }
static void move_rr_b (void) { MOVE_RR(B,B); }
static void move_rr_ax(void) { MOVE_RR(A,B); }
static void move_rr_bx(void) { MOVE_RR(B,A); }

#define MOVE_RN(F,R)	       		       			    		\
{																\
	WFIELD##F(R##REG(DSTREG),R##REG(SRCREG));					\
}
static void move0_rn_a (void) { MOVE_RN(0,A); }
static void move0_rn_b (void) { MOVE_RN(0,B); }
static void move1_rn_a (void) { MOVE_RN(1,A); }
static void move1_rn_b (void) { MOVE_RN(1,B); }

#define MOVE_R_DN(F,R)	       		       			    		\
{																\
	R##REG(DSTREG)-=FW_INC(F);									\
	WFIELD##F(R##REG(DSTREG),R##REG(SRCREG));					\
	COPY_##R##SP;												\
}
static void move0_r_dn_a (void) { MOVE_R_DN(0,A); }
static void move0_r_dn_b (void) { MOVE_R_DN(0,B); }
static void move1_r_dn_a (void) { MOVE_R_DN(1,A); }
static void move1_r_dn_b (void) { MOVE_R_DN(1,B); }

#define MOVE_R_NI(F,R)	       		       			    		\
{			  													\
    WFIELD##F(R##REG(DSTREG),R##REG(SRCREG));					\
    R##REG(DSTREG)+=FW_INC(F);									\
	COPY_##R##SP;												\
}
static void move0_r_ni_a (void) { MOVE_R_NI(0,A); }
static void move0_r_ni_b (void) { MOVE_R_NI(0,B); }
static void move1_r_ni_a (void) { MOVE_R_NI(1,A); }
static void move1_r_ni_b (void) { MOVE_R_NI(1,B); }

#define MOVE_NR(F,R)	       		       			    		\
{																\
	R##REG(DSTREG) = RFIELD##F(R##REG(SRCREG));					\
	EXTEND_F##F(R##REG(DSTREG));								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void move0_nr_a (void) { MOVE_NR(0,A); }
static void move0_nr_b (void) { MOVE_NR(0,B); }
static void move1_nr_a (void) { MOVE_NR(1,A); }
static void move1_nr_b (void) { MOVE_NR(1,B); }

#define MOVE_DN_R(F,R)	       		       			    		\
{			  													\
	R##REG(SRCREG)-=FW_INC(F);									\
	R##REG(DSTREG) = RFIELD##F(R##REG(SRCREG));					\
	EXTEND_F##F(R##REG(DSTREG));								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}																
static void move0_dn_r_a (void) { MOVE_DN_R(0,A); }
static void move0_dn_r_b (void) { MOVE_DN_R(0,B); }
static void move1_dn_r_a (void) { MOVE_DN_R(1,A); }
static void move1_dn_r_b (void) { MOVE_DN_R(1,B); }

#define MOVE_NI_R(F,R)	       		       			    		\
{			  													\
	INT32 data = RFIELD##F(R##REG(SRCREG));						\
	R##REG(SRCREG)+=FW_INC(F);									\
	R##REG(DSTREG) = data;										\
	EXTEND_F##F(R##REG(DSTREG));								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void move0_ni_r_a (void) { MOVE_NI_R(0,A); }
static void move0_ni_r_b (void) { MOVE_NI_R(0,B); }
static void move1_ni_r_a (void) { MOVE_NI_R(1,A); }
static void move1_ni_r_b (void) { MOVE_NI_R(1,B); }

#define MOVE_NN(F,R)	       		       			    		\
{										  						\
	WFIELD##F(R##REG(DSTREG),RFIELD##F(R##REG(SRCREG)));		\
}
static void move0_nn_a (void) { MOVE_NN(0,A); }
static void move0_nn_b (void) { MOVE_NN(0,B); }
static void move1_nn_a (void) { MOVE_NN(1,A); }
static void move1_nn_b (void) { MOVE_NN(1,B); }

#define MOVE_DN_DN(F,R)	       		       			    		\
{			  													\
	INT32 data;													\
	R##REG(SRCREG)-=FW_INC(F);									\
	data = RFIELD##F(R##REG(SRCREG));							\
	R##REG(DSTREG)-=FW_INC(F);									\
	WFIELD##F(R##REG(DSTREG),data);								\
	COPY_##R##SP;												\
}
static void move0_dn_dn_a (void) { MOVE_DN_DN(0,A); }
static void move0_dn_dn_b (void) { MOVE_DN_DN(0,B); }
static void move1_dn_dn_a (void) { MOVE_DN_DN(1,A); }
static void move1_dn_dn_b (void) { MOVE_DN_DN(1,B); }

#define MOVE_NI_NI(F,R)	       		       			    		\
{			  													\
	INT32 data = RFIELD##F(R##REG(SRCREG));						\
	R##REG(SRCREG)+=FW_INC(F);									\
	WFIELD##F(R##REG(DSTREG),data);								\
	R##REG(DSTREG)+=FW_INC(F);									\
	COPY_##R##SP;												\
}
static void move0_ni_ni_a (void) { MOVE_NI_NI(0,A); }
static void move0_ni_ni_b (void) { MOVE_NI_NI(0,B); }
static void move1_ni_ni_a (void) { MOVE_NI_NI(1,A); }
static void move1_ni_ni_b (void) { MOVE_NI_NI(1,B); }

#define MOVE_R_NO(F,R)	       		       			    		\
{								  								\
	INT32 o = PARAM_WORD;										\
	EXTEND_W(o);												\
	WFIELD##F(R##REG(DSTREG)+o,R##REG(SRCREG));					\
}
static void move0_r_no_a (void) { MOVE_R_NO(0,A); }
static void move0_r_no_b (void) { MOVE_R_NO(0,B); }
static void move1_r_no_a (void) { MOVE_R_NO(1,A); }
static void move1_r_no_b (void) { MOVE_R_NO(1,B); }

#define MOVE_NO_R(F,R)	       		       			    		\
{			  													\
	INT32 o = PARAM_WORD;										\
	EXTEND_W(o);												\
	R##REG(DSTREG) = RFIELD##F(R##REG(SRCREG)+o);				\
	EXTEND_F##F(R##REG(DSTREG));								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void move0_no_r_a (void) { MOVE_NO_R(0,A); }
static void move0_no_r_b (void) { MOVE_NO_R(0,B); }
static void move1_no_r_a (void) { MOVE_NO_R(1,A); }
static void move1_no_r_b (void) { MOVE_NO_R(1,B); }

#define MOVE_NO_NI(F,R)	       		       			    		\
{			  													\
	INT32 data;													\
	INT32 o = PARAM_WORD;										\
	EXTEND_W(o);												\
	data = RFIELD##F(R##REG(SRCREG)+o);							\
	WFIELD##F(R##REG(DSTREG),data);								\
	R##REG(DSTREG)+=FW_INC(F);									\
	COPY_##R##SP;												\
}
static void move0_no_ni_a (void) { MOVE_NO_NI(0,A); }
static void move0_no_ni_b (void) { MOVE_NO_NI(0,B); }
static void move1_no_ni_a (void) { MOVE_NO_NI(1,A); }
static void move1_no_ni_b (void) { MOVE_NO_NI(1,B); }

#define MOVE_NO_NO(F,R)	       		       			    		\
{				 												\
	INT32 data;													\
	INT32 o1 = PARAM_WORD;										\
	INT32 o2 = PARAM_WORD;										\
	EXTEND_W(o1);												\
	EXTEND_W(o2);												\
	data = RFIELD##F(R##REG(SRCREG)+o1);						\
	WFIELD##F(R##REG(DSTREG)+o2,data);							\
}
static void move0_no_no_a (void) { MOVE_NO_NO(0,A); }
static void move0_no_no_b (void) { MOVE_NO_NO(0,B); }
static void move1_no_no_a (void) { MOVE_NO_NO(1,A); }
static void move1_no_no_b (void) { MOVE_NO_NO(1,B); }

#define MOVE_RA(F,R)	       		       			    		\
{							  									\
	WFIELD##F(PARAM_LONG(),R##REG(DSTREG));						\
}
static void move0_ra_a (void) { MOVE_RA(0,A); }
static void move0_ra_b (void) { MOVE_RA(0,B); }
static void move1_ra_a (void) { MOVE_RA(1,A); }
static void move1_ra_b (void) { MOVE_RA(1,B); }

#define MOVE_AR(F,R)	       		       			    		\
{			  													\
	R##REG(DSTREG) = RFIELD##F(PARAM_LONG());					\
	EXTEND_F##F(R##REG(DSTREG));								\
	CLR_V;														\
	SET_NZ(R##REG(DSTREG));										\
	COPY_##R##SP;												\
}
static void move0_ar_a (void) { MOVE_AR(0,A); }
static void move0_ar_b (void) { MOVE_AR(0,B); }
static void move1_ar_a (void) { MOVE_AR(1,A); }
static void move1_ar_b (void) { MOVE_AR(1,B); }

#define MOVE_A_NI(F,R)	       		       			    		\
{			  													\
    WFIELD##F(R##REG(DSTREG),RFIELD##F(PARAM_LONG()));			\
    R##REG(DSTREG)+=FW_INC(F);									\
	COPY_##R##SP;												\
}
static void move0_a_ni_a (void) { MOVE_A_NI(0,A); }
static void move0_a_ni_b (void) { MOVE_A_NI(0,B); }
static void move1_a_ni_a (void) { MOVE_A_NI(1,A); }
static void move1_a_ni_b (void) { MOVE_A_NI(1,B); }

#define MOVE_AA(F)		       		       			    		\
{																\
	UINT32 bitaddrs=PARAM_LONG();								\
	WFIELD##F(PARAM_LONG(),RFIELD##F(bitaddrs));				\
}
static void move0_aa (void) { MOVE_AA(0); }
static void move1_aa (void) { MOVE_AA(1); }


/* Program Control and Context Switching */
#define CALL(R)													\
{																\
	PUSH(PC);													\
	PC = R##REG(DSTREG);										\
}
static void call_a (void) { CALL(A); }
static void call_b (void) { CALL(B); }

static void callr(void)
{
	PUSH(PC+0x10);
	PC += (((INT16)PARAM_WORD)<<4);
}

static void calla(void)
{
	PUSH(PC+0x20);
	PC = PARAM_LONG();
}

#define DSJ(R)													\
{																\
	if (--R##REG(DSTREG))										\
	{															\
		PC += (((INT16)PARAM_WORD)<<4);							\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
	}															\
	COPY_##R##SP;												\
}
static void dsj_a (void) { DSJ(A); }
static void dsj_b (void) { DSJ(B); }

#define DSJEQ(R)												\
{																\
	if (!NOTZ_FLAG)												\
	{															\
		if (--R##REG(DSTREG))									\
		{														\
			PC += (((INT16)PARAM_WORD)<<4);						\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
		}														\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
	}															\
}
static void dsjeq_a (void) { DSJEQ(A); }
static void dsjeq_b (void) { DSJEQ(B); }

#define DSJNE(R)												\
{																\
	if (NOTZ_FLAG)												\
	{															\
		if (--R##REG(DSTREG))									\
		{														\
			PC += (((INT16)PARAM_WORD)<<4);						\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
		}														\
		COPY_##R##SP;											\
	}															\
	else														\
	{															\
		SKIP_WORD;												\
	}															\
}
static void dsjne_a (void) { DSJNE(A); }
static void dsjne_b (void) { DSJNE(B); }

#define DSJS(R)													\
{									   							\
	if (state.op & 0x0400)										\
	{															\
		if (--R##REG(DSTREG)) PC -= ((PARAM_K)<<4);				\
	}															\
	else														\
	{															\
		if (--R##REG(DSTREG)) PC += ((PARAM_K)<<4);				\
	}															\
	COPY_##R##SP;												\
}
static void dsjs_a (void) { DSJS(A); }
static void dsjs_b (void) { DSJS(B); }

static void emu(void)
{
	/* In RUN state, this instruction is a NOP */
}

#define EXGPC(R)												\
{			  													\
	INT32 temppc = R##REG(DSTREG);								\
	R##REG(DSTREG) = PC;										\
	PC = temppc;												\
	COPY_##R##SP;												\
}
static void exgpc_a (void) { EXGPC(A); }
static void exgpc_b (void) { EXGPC(B); }

#define GETPC(R)												\
{																\
	R##REG(DSTREG) = PC;										\
	COPY_##R##SP;												\
}
static void getpc_a (void) { GETPC(A); }
static void getpc_b (void) { GETPC(B); }

#define GETST(R)												\
{			  													\
	R##REG(DSTREG) = GET_ST();									\
	COPY_##R##SP;												\
}
static void getst_a (void) { GETST(A); }
static void getst_b (void) { GETST(B); }

#define j_xx_8(TAKE)			  								\
{	   															\
	if (DSTREG)													\
	{															\
		if (TAKE)												\
		{														\
			PC += (PARAM_REL8 << 4);							\
		}														\
	}															\
	else														\
	{															\
		if (TAKE)												\
		{														\
			PC = PARAM_LONG();									\
		}														\
		else													\
		{														\
			SKIP_LONG;											\
		}														\
	}															\
}

#define j_xx_0(TAKE)											\
{																\
	if (DSTREG)													\
	{															\
		if (TAKE)												\
		{														\
			PC += (PARAM_REL8 << 4);							\
		}														\
	}															\
	else														\
	{															\
		if (TAKE)												\
		{														\
			INT16 ls = (INT16) PARAM_WORD;						\
			PC += (ls << 4);									\
		}														\
		else													\
		{														\
			SKIP_WORD;											\
		}														\
	}															\
}

#define j_xx_x(TAKE)											\
{																\
	if (TAKE)													\
	{															\
		PC += (PARAM_REL8 << 4);								\
	}															\
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

#define JUMP(R)													\
{																\
	PC = R##REG(DSTREG);										\
}
static void jump_a (void) { JUMP(A); }
static void jump_b (void) { JUMP(B); }

static void popst(void)
{
	SET_ST(POP());
}

static void pushst(void)
{
	PUSH(GET_ST());
}

#define PUTST(R)												\
{																\
	SET_ST(R##REG(DSTREG));										\
}
static void putst_a (void) { PUTST(A); }
static void putst_b (void) { PUTST(B); }

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

#define REV(R)													\
{																\
    R##REG(DSTREG) = 0x0008;									\
	COPY_##R##SP;												\
}
static void rev_a (void) { REV(A); }
static void rev_b (void) { REV(B); }

static void trap(void)
{
	UINT32 t = PARAM_N;
	if (t)
	{
		PUSH(PC);
		PUSH(GET_ST());
	}
	RESET_ST();
	PC = RLONG(0xffffffe0-(t<<5));
}
