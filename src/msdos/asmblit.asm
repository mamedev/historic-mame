;
; Blitting routines for Mame, using MMX for stretching
; Bernd Wiebelt, January 2000

%assign TRY_HALFBRIGHT 1				; doubling with HALFBRIGHT mode?


[BITS 32]

extern _palette_16bit_lookup			; for 16 bit palettized mode
										; assuming entries are _doubled_
										; to their respective 32 bit values

[SECTION .data]
halfbright15_mask:	dd 0x3def3def		; mask for halfbright (5-5-5)
					dd 0x3def3def
MULT4W3:			dd 0x00030003		; multiply 4 words by 3
					dd 0x00030003
halfbright16_mask:	dd 0x7bef7bef		; mask for halfbright (5-6-5)
					dd 0x7bef7bef
p75_bright16_mask:	dd 0x39773977		; mask for 3/4 bright (5-6-5)
					dd 0x39773977


[SECTION .text]

; Here comes the generic template for blitting
; Note that the function names which are used by C are
; created on the fly by macro expansion. Cool feature.

; list of parameters
width		equ 8						; number of bitmap rows to copy
height		equ 12						; number of bitmap dwords to copy
src			equ 16						; source (memory bitmap)
src_offset	equ 20						; offset to next bitmap row
dst			equ 24						; destination (LINEAR framebuffer)
dst_offset	equ 28						; offset to next framebuffer row

%macro blit_template 4-5
	GLOBAL _asmblit_%1x_%2y_%3sl_%4bpp%5
	_asmblit_%1x_%2y_%3sl_%4bpp%5:

	%assign MY  %1-%3					; use better macro parameter names
	%assign MX  %2						; and take care of scanlines
	%assign SL  %3
	%assign BPP %4
	%if (%0 == 4)
		%assign PALETTIZED 0
	%else
		%assign PALETTIZED 1
	%endif


	push ebp							; save frame-pointer
	mov ebp, esp

	add [ebp + width], DWORD 3			; pad to a dword
	shr	DWORD [ebp + width], 2			; 4 dwords per loop


align 32								; align the inner loop

%%next_line:
	mov esi, [ebp + src]
	mov edi, [ebp + dst]
	mov ecx, [ebp + width]
%if (MY > 1)
	mov edx, [ebp + dst_offset] 		; only if y-stretching
%endif

%%next_linesegment

	movq mm0, [esi+0] 					; always read 16 bytes at once
	movq mm4, [esi+8]

%if (PALETTIZED == 1)					; stretch pixels palettized

	mov ebx, [_palette_16bit_lookup]

	movq mm3, mm0

	movd eax, mm3
	and eax, 0x0000ffff
	movd mm0, [ebx+eax*4]

	psrlq mm3, 16
	movd eax, mm3
	and eax, 0x0000ffff
	movd mm1, [ebx+eax*4]

	psrlq mm3, 16
	movd eax, mm3
	and eax, 0x0000ffff
	movd mm2, [ebx+eax*4]

	psrlq mm3, 16
	movd eax, mm3
	and eax, 0x0000ffff
	movd mm3, [ebx+eax*4]

	movq mm7, mm4

	movd eax, mm7
	and eax, 0x0000ffff
	movd mm4, [ebx+eax*4]

	psrlq mm7, 16
	movd eax, mm7
	and eax, 0x0000ffff
	movd mm5, [ebx+eax*4]

	psrlq mm7, 16
	movd eax, mm7
	and eax, 0x0000ffff
	movd mm6, [ebx+eax*4]

	psrlq mm7, 16
	movd eax, mm7
	and eax, 0x0000ffff
	movd mm7, [ebx+eax*4]

	%if (MX == 1)						; now rearrange the stuff
		punpcklwd mm0, mm1				; add words to dwords
		punpcklwd mm2, mm3
		punpcklwd mm4, mm5
		punpcklwd mm6, mm7

		punpckldq mm0, mm2				; add dwords to quadwords
		punpckldq mm4, mm6
	%elif (MX == 2)
		punpckldq mm0, mm1
		punpckldq mm2, mm3
		punpckldq mm4, mm5
		punpckldq mm6, mm7
	%endif
%endif ; (PALETTIZED == 1)


%if (PALETTIZED == 0)					; stretch pixel non-palettized
	%if (MX == 2)
		movq mm2, mm0
		movq mm6, mm4
		%if (BPP == 8)
			punpcklbw mm0, mm0
			punpckhbw mm2, mm2
			punpcklbw mm4, mm4
			punpckhbw mm6, mm6
		%elif (BPP = 16)
			punpcklwd mm0, mm0
			punpckhwd mm2, mm2
			punpcklwd mm4, mm4
			punpckhwd mm6, mm6
		%endif
	%endif
%endif

%if (MX == 1)
	movq [fs:edi+0], mm0					; simply store the stuff back
	movq [fs:edi+8], mm4
	%if (MY >= 2)
		%if ((BPP == 16) && TRY_HALFBRIGHT)
			movq mm7, [halfbright16_mask]
			psrlq mm0, 1
			psrlq mm4, 1
			pand mm0, mm7
			pand mm4, mm7
		%endif
		movq [fs:edi+edx], mm0
		movq [fs:edi+edx+8], mm4
	%endif
%endif

%if (MX == 2)
	movq [fs:edi], mm0
	movq [fs:edi+8], mm2
	movq [fs:edi+16], mm4
	movq [fs:edi+24], mm6
	%if (MY >= 2)
		%if ((BPP == 16) && TRY_HALFBRIGHT)
			psrlq mm0, 1
			psrlq mm2, 1
			psrlq mm4, 1
			psrlq mm6, 1
			pand mm0, [halfbright16_mask]
			pand mm2, [halfbright16_mask]
			pand mm4, [halfbright16_mask]
			pand mm6, [halfbright16_mask]
			%if 1	; (3/4 bright)
				movq mm1, mm0
				movq mm3, mm2
				movq mm5, mm4
				movq mm7, mm6
				psrlq mm1, 1
				psrlq mm3, 1
				psrlq mm5, 1
				psrlq mm7, 1
				pand mm1, [halfbright16_mask]
				pand mm3, [halfbright16_mask]
				pand mm5, [halfbright16_mask]
				pand mm7, [halfbright16_mask]
				paddw mm0, mm1
				paddw mm2, mm3
				paddw mm4, mm5
				paddw mm6, mm7
			%endif
		%endif
		movq [fs:edi+edx], mm0
		movq [fs:edi+edx+8], mm2
		movq [fs:edi+edx+16], mm4
		movq [fs:edi+edx+24], mm6
	%endif
%endif 									; end case (MX == 2)

	lea esi, [esi+16]					; next 16 src bytes
	lea edi, [edi+(16*MX)]				; next 16*MX dst bytes

	dec ecx
	jnz NEAR %%next_linesegment			; row done?

	mov eax, [ebp+src_offset]			; next src row
	add [ebp+src], eax

	mov eax, [ebp+dst_offset]			; next dst row
	imul eax, MY+SL
	add [ebp+dst], eax

	dec DWORD [ebp+height]				;
	jnz NEAR %%next_line				; all rows done?

	mov esp, ebp						; yes, restore ebp
	pop ebp
	emms								; mmx cleanup [necessary?]
	ret


%endmacro

;
; helper macro for creating the blitting functions
; argument 1: bpp, argument 2: palettized/empty
;
%macro create_stretch 1-2
	%assign i 1
	%rep 4
		%assign j 1
		%rep 4
			blit_template i,j,0,%1,%2	; noscanline
			blit_template i,j,1,%1,%2	; scanline
			%assign j j+1
		%endrep
	%assign i i+1
	%endrep
%endmacro


;
; create all blitting routines now
; this is all too easy...
;
create_stretch 8
create_stretch 16
create_stretch 16,_palettized

