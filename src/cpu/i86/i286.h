/****************************************************************************/
/*            real mode i286 emulator by Fabrice Frances                    */
/*           (initial work based on David Hedley's pcemu)                   */
/*                                                                          */
/****************************************************************************/

#include "i86.h"

#if 1
#undef GetMemB
#undef GetMemW
#undef PutMemB
#undef PutMemW

/* ASG 971005 -- changed to cpu_readmem20/cpu_writemem20 */
#define GetMemB(Seg,Off) (i286_ICount-=6,(BYTE)cpu_readmem24lew((DefaultBase(Seg)+(Off))&I.amask))
#define GetMemW(Seg,Off) (i286_ICount-=10,(WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemB(Seg,Off,x) { i286_ICount-=7; cpu_writemem24lew((DefaultBase(Seg)+(Off))&I.amask,(x)); }
#define PutMemW(Seg,Off,x) { i286_ICount-=11; PutMemB(Seg,Off,(BYTE)(x)); PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)); }

#undef PEEKBYTE
#define PEEKBYTE(ea) ((BYTE)cpu_readmem24lew((ea)&I.amask))

#undef ReadByte
#undef ReadWord
#undef WriteByte
#undef WriteWord

#define ReadByte(ea) (i286_ICount-=6,(BYTE)cpu_readmem24lew((ea)&I.amask))
#define ReadWord(ea) (i286_ICount-=10,cpu_readmem24lew((ea)&I.amask)+(cpu_readmem24lew(((ea)+1)&I.amask)<<8))
#define WriteByte(ea,val) { i286_ICount-=7; cpu_writemem24lew((ea)&I.amask,val); }
#define WriteWord(ea,val) { i286_ICount-=11; cpu_writemem24lew((ea)&I.amask,(BYTE)(val)); cpu_writemem24lew(((ea)+1)&I.amask,(val)>>8); }

#undef CHANGE_PC
#define CHANGE_PC(addr) change_pc24lew(addr)
#endif
