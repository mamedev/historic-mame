#ifndef vector_h
#define vector_h

#define USE_AVG	0
#define USE_DVG 1
//#define VG_DEBUG

extern unsigned char *vectorram;

void vg_go (int offset, int data);
void vg_reset (int offset, int data);
int vg_init (int len, int usingDvg);
void vg_stop (void);

#endif