//************************************************************************
//
// Status word functions
//
//************************************************************************

void setstat(void)
{
	int i;
	UINT8 a;

  	I.STATUS &= ~ ST_P;

  	// We set the parity bit.
  	a = lastparity;

  	for (i=0; i<8; i++)   // 8 bits to test
  	{
		if (a & 1)  // If current bit is set
      		I.STATUS ^= ST_P; // we change ST_P bit

      	a >>= 1;    // Next bit.
  	}
}

// getstat sets emulator's lastparity variable according to 9900's STATUS bits.
// It must be called on interrupt return, or when, for some reason,
// the emulated program sets the STATUS register directly.

void getstat(void)
{
  if (I.STATUS & ST_P)
    lastparity = 1;
  else
    lastparity = 0;
}

// A few words about the following functions.
//
// A big portability issue is the behavior of the ">>" instruction with the sign bit, which has
// not been normalised.  Every compiler does whatever it thinks smartest.
// My code assumed that when shifting right signed numbers, the operand is left-filled with a
// copy of sign bit, and that when shifting unsigned variables, it is left-filled with 0s.
// This is probably the most logical behaviour, and it is the behavior of CW PRO3 - most time
// (the exception is that ">>=" instructions always copy the sign bit (!)).  But some compilers
// are bound to disagree.
//
// So, I had to create special functions with predefined tables included, so that this code work
// on every compiler.  BUT this is a real slow-down.
// So, you might have to include a few lines in assembly to make this work better.
// Sorry about this, this problem is really unpleasant and absurd, but it is not my fault.

#if 0 // RIGHT_SHIFT_TYPE == SMART_RIGHT_SHIFT

#else

const UINT16 right_shift_mask_table[17] =
{
    0xFFFF,
    0x7FFF,
    0x3FFF,
    0x1FFF,
    0x0FFF,
    0x07FF,
    0x03FF,
    0x01FF,
    0x00FF,
    0x007F,
    0x003F,
    0x001F,
    0x000F,
    0x0007,
    0x0003,
    0x0001,
    0x0000,
};

const UINT16 inverted_right_shift_mask_table[17] =
{
	0x0000,
    0x8000,
    0xC000,
    0xE000,
    0xF000,
    0xF800,
    0xFC00,
    0xFE00,
    0xFF00,
    0xFF80,
    0xFFC0,
    0xFFE0,
    0xFFF0,
    0xFFF8,
    0xFFFC,
    0xFFFE,
    0xFFFF,
};

UINT16 logical_right_shift(UINT16 val, int c)
{
	return((val>>c) & right_shift_mask_table[c]);
}

INT16 arithmetic_right_shift(INT16 val, int c)
{
	if (val < 0)
    	return((val>>c) | inverted_right_shift_mask_table[c]);
    else
    	return((val>>c) & right_shift_mask_table[c]);
}

#endif


// P bit maintained in lastparity
//
// Set lae
//
INLINE void setst_lae(INT16 val)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E);

  if (val > 0)
    I.STATUS |=  (ST_L | ST_A);
  else if (val < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;
}


//
// Set laep (BYTE)
//
INLINE void setst_byte_laep(INT8 val)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E);

  if (val > 0)
    I.STATUS |=  (ST_L | ST_A);
  else if (val < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  lastparity = val;
}

//
// For COC, CZC, and TB
//
INLINE void setst_e(UINT16 val, UINT16 to)
{
  if (val == to)
    I.STATUS |= ST_E;
  else
    I.STATUS &= ~ ST_E;
}

//
// For CI, C, CB
//
INLINE void setst_c_lae(UINT16 to, UINT16 val)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E);

  if (val == to)
    I.STATUS |= ST_E;
  else
  {
	if ( ((INT16) val) > ((INT16) to) )
      I.STATUS |= ST_A;
	if ( ((UINT16) val) > ((UINT16) to) )
      I.STATUS |=  ST_L;
  }
}

#define wadd(addr,expr) { int lval = setst_add_laeco(readword(addr), (expr)); writeword((addr),lval); }
#define wsub(addr,expr) { int lval = setst_sub_laeco(readword(addr), (expr)); writeword((addr),lval); }

#ifdef __POWERPC__

// The following procedure:
// - computes a+b
// - sets Carry and Overflow
//
// It is in PPC assembly, because I don't know of a simple way to set Overflow in C.
//
// (Well, there is: c=a+b, Overflow = (a^b)>0 ? ((c ^ a) < 0) : 0  It must work,
// but I don't call this "a simple way")
//
INT32 asm setst_add_32_co(register INT32 a, register INT32 b);

INT32 asm setst_add_32_co(register INT32 a, register INT32 b)
{
  addco r3, b, a    // add, and set CA and OV
  mcrxr cr0         // move XER to CR0
  lwz r4, I(RTOC)
  lhz r0, 4(r4)     // load I.STATUS
  bf+ cr1, nooverflow   // if OV is not set, jump to nooverflow.  A jump is more likely than no jump.
  ori r0, r0, ST_O  // else set ST_O
nooverflow:
  bf+ cr2, nocarry  // if CA is not set, jump to nocarry.  A jump is more likely than no jump.
  ori r0, r0, ST_C  // else set ST_C
nocarry:
  sth r0, 4(r4)     // save I.TATUS
  blr               // return
}

#define setst_add_co(a, b) (setst_add_32_co(a << 16, b << 16) >> 16)
#define setst_addbyte_co(a, b) (setst_add_32_co(a << 24, b <<24) >> 24)

// The following procedure:
// - computes b-a
// - sets Carry and Overflow
//
// It is in PPC assembly, because I don't know of a simple way to set Overflow in C.
//
INT32 asm setst_sub_32_co(register INT32 a, register INT32 b);

INT32 asm setst_sub_32_co(register INT32 a, register INT32 b)
{
  subco r3, a, b    // sub, and set CA and OV
  mcrxr cr0         // move XER to CR0
  lwz r4, I(RTOC)
  lhz r0, 4(r4)     // load I.STATUS
  bf+ cr1, nooverflow   // if OV is not set, jump to nooverflow.  A jump is more likely than no jump.
  ori r0, r0, ST_O  // else set ST_O
nooverflow:
  bf- cr2, nocarry  // if CA is not set, jump to nocarry.  No jump is more likely than a jump.
  ori r0, r0, ST_C  // else set ST_C
nocarry:
  sth r0, 4(r4)     // save I.STATUS
  blr               // return
}

#define setst_sub_co(a, b) (setst_sub_32_co(a << 16, b << 16) >> 16)
#define setst_subbyte_co(a, b) (setst_sub_32_co(a << 24, b <<24) >> 24)

#else

/* Could do with some equivalent functions for non power PC's */

UINT16 setst_add_co(UINT16 dst,UINT16 src)
{
	UINT32 res,a,b;

	a = dst & 0xffff;
	b = src & 0xffff;

    res = a + b;

	I.STATUS &= ~(ST_C | ST_O);
	if(res & 0x10000) I.STATUS |= ST_C;

    if (((res) ^ (src)) & ((res) ^ (dst)) & 0x8000) I.STATUS |= ST_O;

	return (UINT16) res;
}

UINT8 setst_addbyte_co(INT8 dst,INT8 src)
{
	unsigned res=dst+src;
	I.STATUS &= ~(ST_C | ST_O);
	if (res & 0x100) I.STATUS |= ST_C;
	if (((res) ^ (src)) & ((res) ^ (dst)) & 0x80) I.STATUS |= ST_O;
	return (UINT8) res;
}

UINT16 setst_sub_co(INT16 dst,INT16 src)
{
	UINT32 res,a,b;

	a = dst & 0xffff;
	b = src & 0xffff;

    res = a - b;

	I.STATUS &= ~(ST_C | ST_O);
	if(res & 0x10000) I.STATUS |= ST_C;
	if (((dst) ^ (src)) & ((dst) ^ (res)) & 0x8000) I.STATUS |= ST_O;

	return (UINT16) res;
}

UINT8 setst_subbyte_co(INT8 dst,INT8 src)
{
	UINT32 res,a,b;

    a = dst & 0xff;
    b = src & 0xff;

	res = a - b;

	I.STATUS &= ~(ST_C | ST_O);

	if (res & 0x100) I.STATUS |= ST_C;

	if (((dst) ^ (src)) & ((dst) ^ (res)) & 0x80) I.STATUS |= ST_O;

	return (UINT8) res;
}

#endif

//
// Set laeco for add, preserve none
//
INLINE INT16 setst_add_laeco(INT32 a, INT32 b)
{
  register INT16 reponse;

  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);

  reponse = setst_add_co(a,b);

  if (reponse > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (reponse < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return reponse;
}


//
//  Set laeco for subtract, preserve none
//
INLINE INT16 setst_sub_laeco(INT32 a, INT32 b)
{
  register INT16 reponse;

  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);

  reponse = setst_sub_co(a, b);

  if (reponse > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (reponse < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return reponse;
}


//
// Set laecop for add, preserve none (BYTE)
//
INLINE INT8 setst_addbyte_laecop(INT32 a, INT32 b)
{
  INT8 reponse;

  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O | ST_P);

  reponse = setst_addbyte_co(a, b);

  if (reponse > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (reponse < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  lastparity = reponse;

  return reponse;
}


//
// Set laeco for subtract, preserve none (BYTE)
//
INLINE INT8 setst_subbyte_laecop(INT32 a, INT32 b)
{
  INT8 reponse;

  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O | ST_P);

  reponse = setst_subbyte_co(a, b);

  if (reponse > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (reponse < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  lastparity = reponse;

  return reponse;
}



//
// For NEG
//
INLINE void setst_laeo(INT16 val)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_O);

  if (val > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (val < 0)
  {
    I.STATUS |= ST_L;
	if (((UINT16) val) == 0x8000)
      I.STATUS |= ST_O;
  }
  else
    I.STATUS |= ST_E;
}



//
// Meat of SRA
//
INLINE UINT16 setst_sra_laec(INT16 a, UINT16 c)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C);

  if (c != 0)
  {
    a = arithmetic_right_shift(a, c-1);
    if (a & 1)  // The carry bit equals the last bit that is shifted out
      I.STATUS |= ST_C;
    a = arithmetic_right_shift(a, 1);
  }

  if (a > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (a < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return a;
}


//
// Meat of SRL.  Same algorithm as SRA, except that we fills in with 0s.
//
INLINE UINT16 setst_srl_laec(UINT16 a,UINT16 c)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C);

  if (c != 0)
  {
    a = logical_right_shift(a, c-1);
    if (a & 1)
      I.STATUS |= ST_C;
    a = logical_right_shift(a, 1);
  }

  if (((INT16) a) > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (((INT16) a) < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return a;
}


//
// Meat of SRC
//
INLINE UINT16 setst_src_laec(UINT16 a,UINT16 c)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C);

  if (c != 0)
  {
    a = logical_right_shift(a, c) | (a << (16-c));
    if (a & 0x8000) // The carry bit equals the last bit that is shifted out
      I.STATUS |= ST_C;
  }

  if (((INT16) a) > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (((INT16) a) < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return a;
}


//
// Meat of SLA
//
INLINE UINT16 setst_sla_laeco(UINT16 a, UINT16 c)
{
  I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);

  if (c != 0)
  {
    {
	  register UINT16 mask;
	  register UINT16 ousted_bits;

      mask = 0xFFFF << (16-c-1);
      ousted_bits = a & mask;

      if (ousted_bits)    // If ousted_bits is neither all 0s
        if (ousted_bits ^ mask)   // nor all 1s,
          I.STATUS |= ST_O;   // we set overflow
    }

    a <<= c-1;
    if (a & 0x8000) // The carry bit equals the last bit that is shifted out
      I.STATUS |= ST_C;

      a <<= 1;
  }

  if (((INT16) a) > 0)
    I.STATUS |=  ST_L | ST_A;
  else if (((INT16) a) < 0)
    I.STATUS |= ST_L;
  else
    I.STATUS |= ST_E;

  return a;
}


/***********************************************************************/

