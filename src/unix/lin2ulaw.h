#ifndef __LIN2ULAW_H_
#define __LIN2ULAW_H_
/*
 * some types
 *
 */

#define i8      char
#define i16     short
#define i32     int

#define u8      unsigned i8
#define u16     unsigned i16
#define u32     unsigned i32

/*
#define uchar   unsigned char
#define ushort  unsigned short
#define ulong   unsigned long
*/

#define  L8_TO_MU8(x)    linearToUlaw[((u16)(x))<< 8]
#define  L16_TO_MU8(x)   linearToUlaw[(u16)x]

#ifndef __LIN2ULAW_C
extern u8 linearToUlaw[];
#endif

#endif
