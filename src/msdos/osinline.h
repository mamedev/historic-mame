
#ifndef __OSINLINE__
#define __OSINLINE__

/* What goes herein depends heavily on the OS. */

extern char *dirty_new;

/* Clean dirty method */
#define osd_mark_vector_dirty(x,y)	dirty_new[y]=1

#define vec_mult _vec_mult
inline int _vec_mult(int x, int y)
{
	int result;
	asm (
			"movl  %1    , %0    ; "
			"imull %2            ; "    /* do the multiply */
			"movl  %%edx , %%eax ; "
			:  "=&a" (result)           /* the result has to go in eax */
			:  "mr" (x),                /* x and y can be regs or mem */
			   "mr" (y)
			:  "%edx", "%cc"            /* clobbers edx and flags */
		);
	return result;
}

#endif /* __OSINLINE__ */
