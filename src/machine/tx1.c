/*===================================================================*/
/*               TX-1/Buggy Boy (Tatsumi) Hardware                   */
/*         SN74S516 Arithmetic Unit and Interface Emulation          */
/*                       VERY PRELIMINARY!                           */
/*===================================================================*/

#include <stdio.h>
#include "driver.h"
#include "machine/random.h"

extern INT16 *AU_PTR;
extern int inst_index;

/* Internal registers and so forth */
struct Regs
{
      INT16 X;    /* Multiplicand and divisor */
      INT16 X1;   /* Previous X */
      INT16 Y;    /* Multiplier */

      INT16 Operand2;

      union       /* 32-bit accumulator */
      {
         #ifdef LSB_FIRST
         struct { UINT16 W; INT16 Z;} ZW_16;
         #else
         struct { INT16 Z; UINT16 W;} ZW_16;
         #endif
         INT32 ZW_32;
      } acc;

     INT16 ins_seq;  /* Instruciton sequence */
} AU_Regs;


/* Main portion of the arithmetic unit emulation. Accessed by AU_R and AU_W */
/* Only a few instructions implemented currently */

void MMI_74S516(int ins, UINT16 *data)
{

   if ((ins!=7) && ((AU_Regs.ins_seq & 0xf)==7))   /* If last instruction was a reading operation, clear sequence. */
      AU_Regs.ins_seq = 0;

   /* Take INS and append to instruction */
   AU_Regs.ins_seq <<=4;
   AU_Regs.ins_seq = AU_Regs.ins_seq | (ins & 0xf);

      switch ( AU_Regs.ins_seq )
      {
        /* X1 . Y */
         case 0x0: AU_Regs.Y = *data;
                  AU_Regs.ins_seq = 0;
                  break;

        /* -X1 . Y */
         case 0x1: AU_Regs.Y = *data;
                AU_Regs.ins_seq = 0;
                break;

        /* X1 . Y + Kz.Kw */
         case 0x2: AU_Regs.Y = *data;
                AU_Regs.ins_seq = 0;
                break;

        /* -X1 . Y + Kz.Kw */
         case 0x3: AU_Regs.Y = *data;
                AU_Regs.ins_seq = 0;
                break;

        /* Partial: Load X */
         case 0x5:  AU_Regs.X = *data;
                   break;
         case 0x6:  AU_Regs.X = *data;
                   break;


        /* X * Y (Fractional) */
         case 0x50: AU_Regs.Y = *data;
                   AU_Regs.ins_seq = 0;
                   break;

        /* X * Y (Integer) */                                           // Ok
         case 0x60: AU_Regs.Y = *data;
                   AU_Regs.acc.ZW_32 = ((INT16)AU_Regs.X * (INT16)AU_Regs.Y);
                   AU_Regs.X1 = AU_Regs.X;
                   AU_Regs.ins_seq = 0;
                   break;

        /* -X * Y (Fractional) */
         case 0x51: AU_Regs.Y = *data;
                   AU_Regs.ins_seq = 0;
                   break;

        /* -X * Y (Integer) */                                               // Ok
         case 0x61: AU_Regs.Y = *data;
                   AU_Regs.acc.ZW_32 = (-(INT16)AU_Regs.X * (INT16)AU_Regs.Y);
                   AU_Regs.X1 = AU_Regs.X;
                   AU_Regs.ins_seq = 0;
                   break;


        /* X * Y + Kz.Kw (Fractional) */
         case 0x52: AU_Regs.Y = *data;
                   AU_Regs.ins_seq = 0;
                   break;

        /* X * Y + Kz.Kw (Integer) */                                       // Ok
         case 0x62: AU_Regs.Y = *data;
                   AU_Regs.acc.ZW_32 += ((INT16)AU_Regs.X * (INT16)AU_Regs.Y);
                   AU_Regs.X1 = AU_Regs.X;
                   AU_Regs.ins_seq = 0;
                   break;


        /* -X * Y + Kz.Kw (Fractional) */
         case 0x53: AU_Regs.Y = *data;
                   AU_Regs.ins_seq = 0;
                   break;

        /* -X * Y + Kz.Kw (Integer) */                                       // Ok
         case 0x63: AU_Regs.Y = *data;
                   AU_Regs.acc.ZW_32 += (-(INT16)AU_Regs.X * (INT16)AU_Regs.Y);
                   AU_Regs.X1 = AU_Regs.X;
                   AU_Regs.ins_seq = 0;
                   break;



        /* Kw / X (Fractional) Nothing Loaded */
         case 0x54: AU_Regs.ins_seq = 0;
                   break;

        /* Kw / X (Integer) Nothing Loaded */
         case 0x64: AU_Regs.acc.ZW_32 /= (INT16)AU_Regs.X;
                   AU_Regs.X1 = AU_Regs.X;
                   AU_Regs.ins_seq = 0;
                   break;

        /* Kz / X (Fractional) Nothing Loaded */
         case 0x55: AU_Regs.ins_seq = 0;
                   break;


        /* Kz / X (Integer) Nothing Loaded */
         case 0x65: AU_Regs.acc.ZW_32=AU_Regs.ins_seq;
                   AU_Regs.ins_seq=0;
                   break;


        /* This can either load Z,Nothing or 0? */
         case 0x56: AU_Regs.Operand2 = *data;
                   break;


         case 0x560: AU_Regs.ins_seq = 0;
                    break;

         case 0x561: AU_Regs.ins_seq = 0;
                    break;

         case 0x562: AU_Regs.ins_seq = 0;
                    break;

         case 0x563: AU_Regs.ins_seq = 0;
                    break;

         case 0x564: AU_Regs.ins_seq = 0;
                    break;

         case 0x565: AU_Regs.ins_seq = 0;
                    break;

        // Not complete instruction
         case 0x566:
                    AU_Regs.ins_seq = 0;
                    break;


         case 0x660: AU_Regs.ins_seq = 0;
                    break;

         case 0x661: AU_Regs.ins_seq = 0;
                    break;

         case 0x662: AU_Regs.ins_seq = 0;
                    break;

         case 0x663: AU_Regs.ins_seq = 0;
                    break;

        /* Z,W /X */
        // result in Z, remainder in W
         case 0x664: AU_Regs.acc.ZW_16.W = *data;
                    AU_Regs.acc.ZW_16.Z = AU_Regs.Operand2;
                    AU_Regs.acc.ZW_16.Z = (INT16)((INT32)AU_Regs.acc.ZW_32 / (INT16)AU_Regs.X);     // wrong :(
                    AU_Regs.acc.ZW_16.W = (INT16)((INT32)AU_Regs.acc.ZW_32 % (INT16)AU_Regs.X);      //correct!
                    AU_Regs.X1 = AU_Regs.X;
                    AU_Regs.ins_seq = 0;
                    break;

         case 0x665: AU_Regs.ins_seq = 0;
                    break;

         case 0x5660:AU_Regs.ins_seq = 0;
                    break;

         case 0x5661:AU_Regs.ins_seq = 0;
                    break;

         case 0x5662:AU_Regs.ins_seq = 0;
                    break;

         case 0x5663:AU_Regs.ins_seq = 0;
                    break;

         case 0x5664:AU_Regs.ins_seq = 0;
                    break;

         case 0x5665:AU_Regs.ins_seq = 0;
                    break;

         case 0x5666:AU_Regs.ins_seq = 0;
                    break;

         case 0x5667:AU_Regs.ins_seq = 0;
                    break;

         case 0x6660:AU_Regs.ins_seq = 0;
                    break;

         case 0x6661:AU_Regs.ins_seq = 0;
                    break;

         case 0x6662:AU_Regs.ins_seq = 0;
                    break;

         case 0x6663:AU_Regs.ins_seq = 0;
                    break;

         /* W/X */
         case 0x6664:AU_Regs.acc.ZW_32 = (INT16)AU_Regs.acc.ZW_16.W / (INT16)AU_Regs.X;
                    AU_Regs.X1 = AU_Regs.X;
                    AU_Regs.ins_seq = 0;
                    break;

         case 0x6665:
                    AU_Regs.ins_seq = 0;
                    break;

         case 0x6666:
                    AU_Regs.ins_seq = 0;
                    break;

         case 0x6667:
                    AU_Regs.ins_seq = 0;
                    break;

         case 0x66: AU_Regs.Operand2 = *data;
                   break;

         case 0x4: AU_Regs.ins_seq = 0;
                   break;

       /* Reading Operations */

         case 0x7:    *data = AU_Regs.acc.ZW_16.Z;
                      break;

         case 0x77:   *data = AU_Regs.acc.ZW_16.W;
                      break;

         case 0x777:  *data = AU_Regs.acc.ZW_16.Z;
                      break;

         case 0x7777: *data = AU_Regs.acc.ZW_16.W;
                      break;

         default:     break;
      }

}



/******************************************************/
/*                                                    */
/*     MMI SN74S516T Arithmetic Unit and Interface    */
/*                                                    */
/******************************************************/

/*
The arithmetic unit is used extensively to calculate both object and road attributes in both games.
It can take instructions directly from slave address bus A1-A3 or from a pair of PROMS (BB1.163 and BB2.162).
Two 16KB function data ROMs are accessible by the chip and the slave CPU (via a hardware index and pointer).

The interface between the arithmetic unit and the slave CPU is different between Buggy Boy and TX-1.

In the case of an Buggy Boy PCB:
Without AU: No objects visible, road is unchanging in direction.
Without instruction PROMS: Hand/Go is displayed correctly as well as the end of race animation sequence (buggy spins and goes up in smoke).

Therfore, once enough simple instructions are implemented, it should be possible do display some objects correctly.

/MLPCS  =  !A10.!A11.A12.A13.!A14.!A15
/DPRCS  =  A10.A11.A12.A13.!A14.!A15
/INSALD =  /AT3RD.!AT3WDRART + !AT3WDRART.A15 + !AT3WDRART.!A13 + !AT3WDRART.A14 + !AT3WDRART.!A12 + !A7
/CNTST  =  /AT3RD.!AT3WDRART + !AT3WDRART.A15 + !AT3WDRART.!A13 + !AT3WDRART.A14 + !AT3WDRART.!A12 + !A8
/SPCS  =  !A15.!A14.!A13.A12 + A14.!A13.!A12 + !A14.A12.A9 + !A14.A13.!A12 + !A14.!A11 + A15

3000-31ff = Direct instruction input (AAB1-AAB3 connect to I0-2 of AU).

3800-39ff
3c00-3dff = SPCS

3c00-3cff
3d00-3dff
3e00-3eff
3f00-3fff = DATA ROM output enable.


The function data ROMs are checksummed on startup (and on entering service mode),
A test of the AU and interface follows (with some ROM mirror checks thrown in for good measure):

ST
04 [300C] <- AA55
   [3000] <- 55AA
   [300E] =  E355 ?
04 [300E] =  5572 ?

05 [300C] <- AA55
   [3002] <- 55AA
   [300E] =  1CAA ?
05 [300E] =  AA8E ?

06 [300C] <- 5A5A
   [3004] <- A5A5
   [300E] =  FCC6 ?
06 [300E] =  E890 ?

07 [300C] <- AA55
   [3006] <- 55AA
   [300E] =  1971 ?
07 [300E] =  931E ?

08 [300C] <- 1000
   [300C] <- 5678
   [3008] <- 8765
   [300E] =  ff88 ?
08 [300E] =  0765 ?

09 [300C] <- 0200
   [300C] <- FFFF
   [300C] <- 55AA
   [3008] <- FFFF
   [300E] =  002a ?
09 [300E] =  01aa ?

10 [3752] <- AA55
   [3600] =  AA55 ?

10 [3600] <- 55AA
   [3680] =  55AA ?
11 [3680] =  A55A ?
12 [3600] =  55AA ?

13 [3754] <- 2DB5
   [3A00] <- 0100
   [3600] =  B6D4 ?

14 [3600] <- 2DB5
   [3A00] <- 0200
   [3600] =  5B6A ?

15 [3600] <- 2DB5
   [3A00] <- 0400
   [3600] =  2DB5 ?

16 [3600] <- 2DB5
   [3A00] <- 0800
   [3600] =  96DA ?

17 [3600] <- 2DB5
   [3A00] <- 1000
   [3600] =  4B6D ?

18 [3600] <- 2DB5
   [3A00] <- 2000
   [3600] =  A5B6 ?

19 [3600] <- 1568
   [3A00] <- 2000
   [3E00] =  1BF2 ?

1A [7A6A] =  0000 ? <- WRONG???

1B [3680] <- AA55
   [7A6A] =  55AA ? (ROM check)

1C [3200] <- AA55
   [300E] =  E355 ?

1D [3754] <- AA55
   [7A6A] =  55AA ? (ROM check)

1E [3680] <- AA55
   [7A6A] =  55AA ? (ROM check)

1F [7A6C] =  AA55 ? (ROM check)

20 [300E] =  1CAA ?

21 [300C] <- 55AA
   [3200] <- AA55
   [300E] =  E355 ?

22 [3E00] =  14D5 ?
23 [308C] <- 55AA
   [7A70] =  15AA ?

24 [7A6E] =  2000 ?
25 [3600] =  42B5 ?
26 [3E80] =  1CC2 ?
27 [308E] =  099F ?
28 [3680] =  4099 ?
29 [3680] =  2D40 ?
2A [3C00] =  5Cf4 ?
2B [3600] =  5Cf4 ?
2C [300C] <- 55AA
   [3A80] =  FFFF ?
2D [300E] =  FFFF ?
2E [300E] =  AA56 ?
2F [3600] <- 0000
   [4000] =  00BC ?
30 [3600] =  0000 ?
31 [310C] <- 0078
   [308C] <- 3F70
   [3680] <- 0087 ?
32 I got bored...
*/

READ8_HANDLER(BB_AU_R)
{
data8_t *AU_instr = (data8_t *)memory_region(REGION_USER1);
//data8_t *AU_PROM1 = (data8_t *)memory_region(REGION_PROMS) + 0x1700;
//data8_t *AU_PROM2 = (data8_t *)memory_region(REGION_PROMS) + 0x1900;

INT8 value = 0;

      switch (offset)
      {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0xe:    MMI_74S516(offset>>1, AU_PTR);   /* Typically Instruction 7 - read result(s) */
                     value = *AU_PTR;
                     break;

        case 0xf:    value = (*AU_PTR)>>8;
                     break;

        case 0x0e00:
        case 0x0e01: value = AU_instr[inst_index];
                     break;

        case 0x0680:
                     break;                       /* Use to change upper ROM address portion? */

        case 0x0726:
                     break;

        default:     value = 0;
                     break;
      }

    return value;

}

WRITE8_HANDLER(BB_AU_W)
{
//data8_t *AU_PROM1 = (data8_t *)memory_region(REGION_PROMS) + 0x1700;
//data8_t *AU_PROM2 = (data8_t *)memory_region(REGION_PROMS) + 0x1900;

      switch (offset)
      {

        case 0x0:    *AU_PTR = (data & 0xff);
                     break;
        case 0x1:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR); /* Load values */
                     break;

        case 0x2:    *AU_PTR = (data & 0xff);
                     break;
        case 0x3:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0x4:    *AU_PTR = (data & 0xff);
                     break;
        case 0x5:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0x6:    *AU_PTR = (data & 0xff);
                     break;
        case 0x7:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0x8:    *AU_PTR = (data & 0xff);
                     break;
        case 0x9:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0xa:    *AU_PTR = (data & 0xff);
                     break;
        case 0xb:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0xc:    *AU_PTR = (data & 0xff);
                     break;
        case 0xd:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

        case 0xe:    *AU_PTR = (data & 0xff);
                     break;
        case 0xf:    *AU_PTR |= data<<8;
                     MMI_74S516(offset>>1, AU_PTR);
                     break;

       /* Accessing FN ROMs */
        case 0x0600:
                     inst_index &= 0xff00;      /* clear lower byte */
                     inst_index |= data;

                     break;

        case 0x0601:
                     inst_index &= 0x00ff;     /* clear upper byte */
                     inst_index |=(data << 8); /* upper */
                     break;


        case 0x0680: break;                       /* Increment instruction PROM address */
        case 0x0726: break;
      }
}

