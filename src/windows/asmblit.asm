[BITS 32]


extern _asmblit_srcdata
extern _asmblit_srcpitch
extern _asmblit_srcwidth
extern _asmblit_srcheight
extern _asmblit_srclookup

extern _asmblit_dstdata
extern _asmblit_dstpitch

extern _asmblit_dirtydata
extern _asmblit_dirtypitch

extern _color16_rsrc_shift
extern _color16_gsrc_shift
extern _color16_bsrc_shift
extern _color16_rdst_shift
extern _color16_gdst_shift
extern _color16_bdst_shift

extern _color32_rdst_shift
extern _color32_gdst_shift
extern _color32_bdst_shift


[SECTION .data]
dirty_count:	db	0


;//============================================================
;//	Y expander
;//============================================================

%macro store_multiple 4
	mov		[edi+%2],%1
	
%if ((%3+%4) > 1)
	mov		edx,[_asmblit_dstpitch]
%if (%3 <= 1)
	xor		%1,%1
%endif
	mov		[edi+edx+%2],%1
%endif

%if ((%3+%4) > 2)
%if (%3 <= 2)
	xor		%1,%1
%endif
	mov		[edi+edx*2+%2],%1
%endif

%if ((%3+%4) > 3)
	lea		edx,[edi+edx*2]
	add		edx,[_asmblit_dstpitch]
%if (%3 <= 3)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
%endif

%if ((%3+%4) > 4)
	add		edx,[_asmblit_dstpitch]
%if (%3 <= 4)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
%endif

%if ((%3+%4) > 5)
	add		edx,[_asmblit_dstpitch]
%if (%3 <= 5)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
%endif
%endmacro


%macro store_multiple2 6
	mov		[edi+%2],%1
	mov		[edi+%4],%3
	
%if ((%5+%6) > 1)
	mov		edx,[_asmblit_dstpitch]
%if (%5 <= 1)
	xor		%1,%1
%endif
	mov		[edi+edx+%2],%1
	mov		[edi+edx+%4],%3
%endif

%if ((%5+%6) > 2)
%if (%5 <= 2)
	xor		%1,%1
%endif
	mov		[edi+edx*2+%2],%1
	mov		[edi+edx*2+%4],%3
%endif

%if ((%5+%6) > 3)
	lea		edx,[edi+edx*2]
	add		edx,[_asmblit_dstpitch]
%if (%5 <= 3)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
%endif

%if ((%5+%6) > 4)
	add		edx,[_asmblit_dstpitch]
%if (%5 <= 4)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
%endif

%if ((%5+%6) > 5)
	add		edx,[_asmblit_dstpitch]
%if (%5 <= 5)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
%endif
%endmacro


%macro store_multiple3 8
	mov		[edi+%2],%1
	mov		[edi+%4],%3
	mov		[edi+%6],%5
	
%if ((%7+%8) > 1)
	mov		edx,[_asmblit_dstpitch]
%if (%7 <= 1)
	xor		%1,%1
%endif
	mov		[edi+edx+%2],%1
	mov		[edi+edx+%4],%3
	mov		[edi+edx+%6],%5
%endif

%if ((%7+%8) > 2)
%if (%7 <= 2)
	xor		%1,%1
%endif
	mov		[edi+edx*2+%2],%1
	mov		[edi+edx*2+%4],%3
	mov		[edi+edx*2+%6],%5
%endif

%if ((%7+%8) > 3)
	lea		edx,[edi+edx*2]
	add		edx,[_asmblit_dstpitch]
%if (%7 <= 3)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
	mov		[edx+%6],%5
%endif

%if ((%7+%8) > 4)
	add		edx,[_asmblit_dstpitch]
%if (%7 <= 4)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
	mov		[edx+%6],%5
%endif

%if ((%7+%8) > 5)
	add		edx,[_asmblit_dstpitch]
%if (%7 <= 5)
	xor		%1,%1
%endif
	mov		[edx+%2],%1
	mov		[edx+%4],%3
	mov		[edx+%6],%5
%endif
%endmacro


;//============================================================
;//	8bpp to 8bpp blitters
;//============================================================

%macro blit1_8_to_8_x1 2
	mov		al,[esi]
	store_multiple al,0,%1,%2
%endmacro

%macro blit16_8_to_8_x1 2
	%assign iter 0
	%rep 2
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_8_x2 2
	mov		al,[esi]
	mov		ah,al
	store_multiple ax,0,%1,%2
%endmacro

%macro blit16_8_to_8_x2 2
	%assign iter 0
	%rep 4
		mov		eax,[esi+4*iter]
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		store_multiple ebx,8*iter,%1,%2
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_8_x3 2
	mov		al,[esi]
	mov		ah,al
	store_multiple2 ax,0,al,2,%1,%2
%endmacro

%macro blit16_8_to_8_x3 2
	%assign iter 0
	%rep 4
		mov		eax,[esi+4*iter]
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	8bpp to 16bpp blitters
;//============================================================

%macro blit1_8_to_16_x1 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0,%1,%2
%endmacro

%macro blit16_8_to_16_x1 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 ax,4*iter,bx,4*iter+2,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_16_x2 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2,%1,%2
%endmacro

%macro blit16_8_to_16_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_16_x3 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 ax,0,ax,2,ax,4,%1,%2
%endmacro

%macro blit16_8_to_16_x3 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,12*iter,ax,12*iter+4,%1,%2
		store_multiple2 bx,12*iter+6,ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	8bpp to 24bpp blitters
;//============================================================

%macro blit1_8_to_24_x1 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0,%1,%2
	shr		eax,16
	store_multiple al,2,%1,%2
%endmacro

%macro blit16_8_to_24_x1 2
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+3]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_24_x2 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4,%1,%2
%endmacro

%macro blit16_8_to_24_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		movzx	eax,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_24_x3 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0,%1,%2
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8,%1,%2
%endmacro

%macro blit16_8_to_24_x3 2
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4,%1,%2
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12,%1,%2
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20,%1,%2
		movzx	eax,byte [esi+4*iter+3]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	8bpp to 32bpp blitters
;//============================================================

%macro blit1_8_to_32_x1 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple eax,0,%1,%2
%endmacro

%macro blit16_8_to_32_x1 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_32_x2 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4,%1,%2
%endmacro

%macro blit16_8_to_32_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,16*iter,eax,16*iter+4,%1,%2
		store_multiple2 ebx,16*iter+8,ebx,16*iter+12,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_8_to_32_x3 2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 eax,0,eax,4,eax,8,%1,%2
%endmacro

%macro blit16_8_to_32_x3 2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple3 eax,24*iter,eax,24*iter+4,eax,24*iter+8,%1,%2
		store_multiple3 ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	16bpp to 16bpp blitters
;//============================================================

%macro blit1_16_to_16_x1 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0,%1,%2
%endmacro

%macro blit16_16_to_16_x1 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 ax,4*iter,bx,4*iter+2,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_16_x2 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2,%1,%2
%endmacro

%macro blit16_16_to_16_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_16_x3 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 ax,0,ax,2,ax,4,%1,%2
%endmacro

%macro blit16_16_to_16_x3 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,12*iter,ax,12*iter+4,%1,%2
		store_multiple2 bx,12*iter+6,ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	16bpp to 24bpp blitters
;//============================================================

%macro blit1_16_to_24_x1 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0,%1,%2
	shr		eax,16
	store_multiple al,2,%1,%2
%endmacro

%macro blit16_16_to_24_x1 2
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+4]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+6]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_24_x2 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4,%1,%2
%endmacro

%macro blit16_16_to_24_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		movzx	eax,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_24_x3 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0,%1,%2
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8,%1,%2
%endmacro

%macro blit16_16_to_24_x3 2
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4,%1,%2
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12,%1,%2
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+4]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20,%1,%2
		movzx	eax,word [esi+8*iter+6]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	16bpp to 32bpp blitters
;//============================================================

%macro blit1_16_to_32_x1 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple eax,0,%1,%2
%endmacro

%macro blit16_16_to_32_x1 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_32_x2 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4,%1,%2
%endmacro

%macro blit16_16_to_32_x2 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,16*iter,eax,16*iter+4,%1,%2
		store_multiple2 ebx,16*iter+8,ebx,16*iter+12,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_16_to_32_x3 2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 eax,0,eax,4,eax,8,%1,%2
%endmacro

%macro blit16_16_to_32_x3 2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple3 eax,24*iter,eax,24*iter+4,eax,24*iter+8,%1,%2
		store_multiple3 ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	32bpp to 16bpp blitters
;//============================================================

%macro blit1_32_to_16_x1 2
	mov		eax,[esi]
	mov		edx,eax
	mov		cl,[_color32_rdst_shift]
	add		cl,[_color16_rsrc_shift]
	shr		edx,cl
	mov		cl,[_color16_rdst_shift]
	shl		edx,cl
	mov		ebx,eax
	mov		cl,[_color32_gdst_shift]
	add		cl,[_color16_gsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_gdst_shift]
	shl		ebx,cl
	or		edx,ebx
	mov		ebx,eax
	mov		cl,[_color32_bdst_shift]
	add		cl,[_color16_bsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_bdst_shift]
	shl		ebx,cl
	or		edx,ebx
	store_multiple dx,0,%1,%2
%endmacro

%macro blit16_32_to_16_x1 2
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		edx,eax
		mov		cl,[_color32_rdst_shift]
		add		cl,[_color16_rsrc_shift]
		shr		edx,cl
		mov		cl,[_color16_rdst_shift]
		shl		edx,cl
		mov		ebx,eax
		mov		cl,[_color32_gdst_shift]
		add		cl,[_color16_gsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_gdst_shift]
		shl		ebx,cl
		or		edx,ebx
		mov		ebx,eax
		mov		cl,[_color32_bdst_shift]
		add		cl,[_color16_bsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_bdst_shift]
		shl		ebx,cl
		or		edx,ebx
		store_multiple dx,2*iter,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_16_x2 2
	mov		eax,[esi]
	mov		edx,eax
	mov		cl,[_color32_rdst_shift]
	add		cl,[_color16_rsrc_shift]
	shr		edx,cl
	mov		cl,[_color16_rdst_shift]
	shl		edx,cl
	mov		ebx,eax
	mov		cl,[_color32_gdst_shift]
	add		cl,[_color16_gsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_gdst_shift]
	shl		ebx,cl
	or		edx,ebx
	mov		ebx,eax
	mov		cl,[_color32_bdst_shift]
	add		cl,[_color16_bsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_bdst_shift]
	shl		ebx,cl
	or		edx,ebx
	store_multiple2 dx,0,dx,2,%1,%2
%endmacro

%macro blit16_32_to_16_x2 2
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		edx,eax
		mov		cl,[_color32_rdst_shift]
		add		cl,[_color16_rsrc_shift]
		shr		edx,cl
		mov		cl,[_color16_rdst_shift]
		shl		edx,cl
		mov		ebx,eax
		mov		cl,[_color32_gdst_shift]
		add		cl,[_color16_gsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_gdst_shift]
		shl		ebx,cl
		or		edx,ebx
		mov		ebx,eax
		mov		cl,[_color32_bdst_shift]
		add		cl,[_color16_bsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_bdst_shift]
		shl		ebx,cl
		or		edx,ebx
		store_multiple2 dx,4*iter,dx,4*iter+2,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_16_x3 2
	mov		eax,[esi]
	mov		edx,eax
	mov		cl,[_color32_rdst_shift]
	add		cl,[_color16_rsrc_shift]
	shr		edx,cl
	mov		cl,[_color16_rdst_shift]
	shl		edx,cl
	mov		ebx,eax
	mov		cl,[_color32_gdst_shift]
	add		cl,[_color16_gsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_gdst_shift]
	shl		ebx,cl
	or		edx,ebx
	mov		ebx,eax
	mov		cl,[_color32_bdst_shift]
	add		cl,[_color16_bsrc_shift]
	shr		ebx,cl
	mov		cl,[_color16_bdst_shift]
	shl		ebx,cl
	or		edx,ebx
	store_multiple3 dx,0,dx,2,dx,4,%1,%2
%endmacro

%macro blit16_32_to_16_x3 2
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		edx,eax
		mov		cl,[_color32_rdst_shift]
		add		cl,[_color16_rsrc_shift]
		shr		edx,cl
		mov		cl,[_color16_rdst_shift]
		shl		edx,cl
		mov		ebx,eax
		mov		cl,[_color32_gdst_shift]
		add		cl,[_color16_gsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_gdst_shift]
		shl		ebx,cl
		or		edx,ebx
		mov		ebx,eax
		mov		cl,[_color32_bdst_shift]
		add		cl,[_color16_bsrc_shift]
		shr		ebx,cl
		mov		cl,[_color16_bdst_shift]
		shl		ebx,cl
		or		edx,ebx
		store_multiple3 dx,6*iter,dx,6*iter+2,dx,6*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	32bpp to 24bpp blitters
;//============================================================

%macro blit1_32_to_24_x1 2
	mov		eax,[esi]
	store_multiple ax,0,%1,%2
	shr		eax,16
	store_multiple al,2,%1,%2
%endmacro

%macro blit16_32_to_24_x1 2
	%assign iter 0
	%rep 4
		mov		eax,[esi+16*iter]
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+8]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+12]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_24_x2 2
	mov		eax,[esi]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4,%1,%2
%endmacro

%macro blit16_32_to_24_x2 2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter,%1,%2
		shrd	ebx,eax,24
		mov		eax,[esi+8*iter+4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_24_x3 2
	mov		eax,[esi]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0,%1,%2
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8,%1,%2
%endmacro

%macro blit16_32_to_24_x3 2
	%assign iter 0
	%rep 4
		mov		eax,[esi+16*iter]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4,%1,%2
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12,%1,%2
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+8]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20,%1,%2
		mov		eax,[esi+16*iter+12]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28,%1,%2
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	32bpp to 32bpp blitters
;//============================================================

%macro blit1_32_to_32_x1 2
	mov		eax,[esi]
	store_multiple eax,0,%1,%2
%endmacro

%macro blit16_32_to_32_x1 2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple2 eax,8*iter,ebx,8*iter+4,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_32_x2 2
	mov		eax,[esi]
	store_multiple2 eax,0,eax,4,%1,%2
%endmacro

%macro blit16_32_to_32_x2 2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple2 eax,16*iter,eax,16*iter+4,%1,%2
		store_multiple2 ebx,16*iter+8,ebx,16*iter+12,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


%macro blit1_32_to_32_x3 2
	mov		eax,[esi]
	store_multiple3 eax,0,eax,4,eax,8,%1,%2
%endmacro

%macro blit16_32_to_32_x3 2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple3 eax,24*iter,eax,24*iter+4,eax,24*iter+8,%1,%2
		store_multiple3 ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20,%1,%2
		%assign iter iter+1
	%endrep
%endmacro


;//============================================================
;//	blitter expansion
;//============================================================

%macro blit1 5
	%if (%1 == 8)
		%if (%2 == 8)
			%if (%3 == 1)
				blit1_8_to_8_x1 %4,%5
			%elif (%3 == 2)
				blit1_8_to_8_x2 %4,%5
			%elif (%3 == 3)
				blit1_8_to_8_x3 %4,%5
			%elif (%3 == 4)
				blit1_8_to_8_x4 %4,%5
			%endif
		%elif (%2 == 16)
			%if (%3 == 1)
				blit1_8_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit1_8_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit1_8_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit1_8_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit1_8_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit1_8_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit1_8_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit1_8_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit1_8_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit1_8_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit1_8_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit1_8_to_32_x4 %4,%5
			%endif
		%endif
	%elif (%1 == 16)
		%if (%2 == 16)
			%if (%3 == 1)
				blit1_16_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit1_16_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit1_16_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit1_16_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit1_16_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit1_16_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit1_16_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit1_16_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit1_16_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit1_16_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit1_16_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit1_16_to_32_x4 %4,%5
			%endif
		%endif
	%elif (%1 == 32)
		%if (%2 == 16)
			%if (%3 == 1)
				blit1_32_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit1_32_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit1_32_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit1_32_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit1_32_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit1_32_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit1_32_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit1_32_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit1_32_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit1_32_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit1_32_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit1_32_to_32_x4 %4,%5
			%endif
		%endif
	%endif
%endmacro


%macro blit16 5
	%if (%1 == 8)
		%if (%2 == 8)
			%if (%3 == 1)
				blit16_8_to_8_x1 %4,%5
			%elif (%3 == 2)
				blit16_8_to_8_x2 %4,%5
			%elif (%3 == 3)
				blit16_8_to_8_x3 %4,%5
			%elif (%3 == 4)
				blit16_8_to_8_x4 %4,%5
			%endif
		%elif (%2 == 16)
			%if (%3 == 1)
				blit16_8_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit16_8_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit16_8_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit16_8_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit16_8_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit16_8_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit16_8_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit16_8_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit16_8_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit16_8_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit16_8_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit16_8_to_32_x4 %4,%5
			%endif
		%endif
	%elif (%1 == 16)
		%if (%2 == 16)
			%if (%3 == 1)
				blit16_16_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit16_16_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit16_16_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit16_16_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit16_16_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit16_16_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit16_16_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit16_16_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit16_16_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit16_16_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit16_16_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit16_16_to_32_x4 %4,%5
			%endif
		%endif
	%elif (%1 == 32)
		%if (%2 == 16)
			%if (%3 == 1)
				blit16_32_to_16_x1 %4,%5
			%elif (%3 == 2)
				blit16_32_to_16_x2 %4,%5
			%elif (%3 == 3)
				blit16_32_to_16_x3 %4,%5
			%elif (%3 == 4)
				blit16_32_to_16_x4 %4,%5
			%endif
		%elif (%2 == 24)
			%if (%3 == 1)
				blit16_32_to_24_x1 %4,%5
			%elif (%3 == 2)
				blit16_32_to_24_x2 %4,%5
			%elif (%3 == 3)
				blit16_32_to_24_x3 %4,%5
			%elif (%3 == 4)
				blit16_32_to_24_x4 %4,%5
			%endif
		%elif (%2 == 32)
			%if (%3 == 1)
				blit16_32_to_32_x1 %4,%5
			%elif (%3 == 2)
				blit16_32_to_32_x2 %4,%5
			%elif (%3 == 3)
				blit16_32_to_32_x3 %4,%5
			%elif (%3 == 4)
				blit16_32_to_32_x4 %4,%5
			%endif
		%endif
	%endif
%endmacro


;//============================================================
;//	misc utilities
;//============================================================

%macro basic_multiply 2
%if (%2 == 1)
%elif (%2 == 2)
	lea		%1,[%1+%1]
%elif (%2 == 3)
	lea		%1,[%1*2+%1]
%elif (%2 == 4)
	lea		%1,[%1*4]
%elif (%2 == 5)
	imul	%1,5
%endif
%endmacro

%macro scale_for_bpp 2
	basic_multiply %1,(%2/8)
%endmacro


;//============================================================
;//	core blitter
;//============================================================

%macro blit_core 6

GLOBAL _asmblit_%1_to_%2_%3x%4_sl%5_dirty%6
_asmblit_%1_to_%2_%3x%4_sl%5_dirty%6:

	%assign SRCBPP  %1						; bits per pixel (8/16/32 bpp)
	%assign DSTBPP  %2						; bits per pixel (8/16/32 bpp)
	%assign XSCALE  %3						; X-Stretch factor
	%assign YSCALE  %4						; Y-Stretch factor
	%assign SCANS   %5						; emulate scanlines
	%assign DIRTY   %6						; perform dirty checking
	
%if (SCANS == 2)
	%assign BLANKS	1
%else
	%assign BLANKS	0
%endif

%if (SCANS != 0)
	%assign EXTRA	1
%else
	%assign EXTRA	0
%endif

	;// prepare
	pushad

	;// load the source/dest pointers
	mov		esi,[_asmblit_srcdata]
	mov		edi,[_asmblit_dstdata]
	
	;// load the palette pointer
%if (SRCBPP < 32)
	mov		ecx,[_asmblit_srclookup]
%endif

	;// load the dirty pointer
%if (DIRTY != 0)
	mov		edx,[_asmblit_dirtydata]
	mov		byte [dirty_count],16
%endif

	;// y loop
%%yloop:
	mov		ebp,[_asmblit_srcwidth]
	shr		ebp,4
	push	esi
	push	edi
%if (DIRTY != 0)
	push	edx
%endif

	;// big X loop
%%bigxloop:
%if (DIRTY != 0)
	test	byte [edx],0xff
	lea		edx,[edx+1]
	jz		near %%notdirty
	push	edx
%endif
	blit16	SRCBPP,DSTBPP,XSCALE,YSCALE,BLANKS
%if (DIRTY != 0)
	pop		edx
%%notdirty:
%endif
	add		esi,16*SRCBPP/8
	add		edi,16*XSCALE*DSTBPP/8
	dec		ebp
	jne		near %%bigxloop
	
%if (DIRTY != 0)
	test	byte [edx],0xff
	jz		near %%skipsmall
%endif
	mov		ebp,[_asmblit_srcwidth]
	and		ebp,15
	je		near %%skipsmall

	;// small X loop
%%smallxloop:
	blit1	SRCBPP,DSTBPP,XSCALE,YSCALE,BLANKS
	add		esi,SRCBPP/8
	add		edi,XSCALE*DSTBPP/8
	dec		ebp
	jne		near %%smallxloop
%%skipsmall:

%if (DIRTY != 0)
	pop		edx
%endif
	pop		edi
	pop		esi
	mov		eax,[_asmblit_dstpitch]
	add		esi,[_asmblit_srcpitch]
%if (DIRTY != 0)
	dec		byte [dirty_count]
	jnz		%%dontadvance
	mov		byte [dirty_count],16
	add		edx,[_asmblit_dirtypitch]
%%dontadvance:
%endif
	basic_multiply eax,YSCALE+EXTRA
	dec		dword [_asmblit_srcheight]
	lea		edi,[edi+eax]
	jne		near %%yloop

	;// finish
	popad
	ret
%endmacro


;//============================================================
;//	core expansion
;//============================================================

[SECTION .text]

%macro expand_blitters 2
	%assign DIRTY 0
	%rep 2
		%assign SCANLINES 0
		%rep 3
			%assign YSCALE 1
			%rep 4
				%assign XSCALE 1
				%rep 3
					blit_core %1,%2,XSCALE,YSCALE,SCANLINES,DIRTY
					%assign XSCALE XSCALE+1
				%endrep
				%assign YSCALE YSCALE+1
			%endrep
			%assign SCANLINES SCANLINES+1
		%endrep
		%assign DIRTY DIRTY+1
	%endrep
%endmacro

expand_blitters 8,8
expand_blitters 8,16
expand_blitters 8,24
expand_blitters 8,32

expand_blitters 16,16
expand_blitters 16,24
expand_blitters 16,32

expand_blitters 32,16
expand_blitters 32,24
expand_blitters 32,32
