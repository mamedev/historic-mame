INLINE UINT8 READ8(UINT32 a)
{
	return ppc.read8(a);
}

INLINE UINT16 READ16(UINT32 a)
{
	if( a & 0x1 )
		return ppc.read16_unaligned(a);
	else
		return ppc.read16(a);
}

INLINE UINT32 READ32(UINT32 a)
{
	if( a & 0x3 )
		return ppc.read32_unaligned(a);
	else
		return ppc.read32(a);
}

INLINE UINT64 READ64(UINT32 a)
{
	if( a & 0x7 )
		return ppc.read64_unaligned(a);
	else
		return ppc.read64(a);
}

INLINE void WRITE8(UINT32 a, UINT8 d)
{
	ppc.write8(a, d);
}

INLINE void WRITE16(UINT32 a, UINT16 d)
{
	if( a & 0x1 )
		ppc.write16_unaligned(a, d);
	else
		ppc.write16(a, d);
}

INLINE void WRITE32(UINT32 a, UINT32 d)
{
	if( ppc.reserved ) {
		if( a == ppc.reserved_address ) {
			ppc.reserved = 0;
		}
	}

	if( a & 0x3 )
		ppc.write32_unaligned(a, d);
	else
		ppc.write32(a, d);
}

INLINE void WRITE64(UINT32 a, UINT64 d)
{
	if( a & 0x7 )
		ppc.write64_unaligned(a, d);
	else
		ppc.write64(a, d);
}

/***********************************************************************/

static UINT16 ppc_read16_unaligned(UINT32 a)
{
	return ((UINT16)ppc.read8(a+0) << 8) | ((UINT16)ppc.read8(a+1) << 0);
}

static UINT32 ppc_read32_unaligned(UINT32 a)
{
	return ((UINT32)ppc.read8(a+0) << 24) | ((UINT32)ppc.read8(a+1) << 16) |
				   ((UINT32)ppc.read8(a+2) << 8) | ((UINT32)ppc.read8(a+3) << 0);
}

static UINT64 ppc_read64_unaligned(UINT32 a)
{
	return ((UINT64)READ32(a+0) << 32) | (UINT64)(READ32(a+4));
}

static void ppc_write16_unaligned(UINT32 a, UINT16 d)
{
	ppc.write8(a+0, (UINT8)(d >> 8));
	ppc.write8(a+1, (UINT8)(d));
}

static void ppc_write32_unaligned(UINT32 a, UINT32 d)
{
	ppc.write8(a+0, (UINT8)(d >> 24));
	ppc.write8(a+1, (UINT8)(d >> 16));
	ppc.write8(a+2, (UINT8)(d >> 8));
	ppc.write8(a+3, (UINT8)(d >> 0));
}

static void ppc_write64_unaligned(UINT32 a, UINT64 d)
{
	ppc.write32(a+0, (UINT32)(d >> 32));
	ppc.write32(a+4, (UINT32)(d));
}


