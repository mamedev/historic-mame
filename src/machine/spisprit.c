
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#include "spisprit.h"
#include "driver.h"

const u32 *tables[3][16] = {
  {
    str1_00, str1_01, str1_02, str1_03,
    str1_04, str1_05, str1_06, str1_07,
    str1_08, str1_09, str1_10, str1_11,
    str1_12, str1_13, str1_14, str1_15
  },
  {
    str2_00, str2_01, str2_02, str2_03,
    str2_04, str2_05, str2_06, str2_07,
    str2_08, str2_09, str2_10, str2_11,
    str2_12, str2_13, str2_14, str2_15
  },
  {
    str3_00, str3_01, str3_02, str3_03,
    str3_04, str3_05, str3_06, str3_07,
    str3_08, str3_09, str3_10, str3_11,
    str3_12, str3_13, str3_14, str3_15
  },
};

u16 spi_gm(int adr, const u32 **t)
{
  int bit0 = 3-((adr >> 8) & 3);
  int val0 = (adr >> (8+2)) & 63;
  int bit1 = 7-((adr >> (8+2+6)) & 7);
  int val1 = (adr >> (8+2+6+3)) & 1;
  int bit  = 1<<(4*bit1+bit0);
  int val  = 2*val0+val1;
  int i;
  u16 r = 0;
  for(i=0; i<16; i++) {
    if(t[i][val] & bit)
      r |= 1<<i;
  }
  return r;
}

u16 spi_gm0(int i)
{
  return spi_gm(i, tables[0]);
}

u16 spi_gm1(int i)
{
  return spi_gm(i, tables[1]);
}

u16 spi_gm2(int i)
{
  return spi_gm(i, tables[2]);
}

int sw0[16] = {
   4,  2,  6,  1, 10,  9,  7, 13,
   5, 11, 15, 12,  3,  0,  8, 14,
};

int sw1[16] = {
  12,  8, 14, 11,  0, 10, 15,  6,
  13,  4,  3,  5,  9,  2,  1,  7,
};

int sw2[16] = {
   0, 12,  7, 14,  6, 15,  1, 13,
   5,  9,  3, 11,  8, 10,  2,  4,
};


u16 swap(u16 v, int *sw)
{
  u16 r = 0x0000;
  int i;
  for(i=0; i<16; i++)
    if(v & (1<<(sw[i] & 15)))
      r |= 1<<(15-i);
  return r;
}

u16 swap0(int i, u16 v)
{
  return swap(i, sw0);
}

u16 swap1(int i, u16 v)
{
  return swap(i, sw1);
}

u16 swap2(int i, u16 v)
{
  return swap(i, sw2);
}

u16 mix0(u16 x, u16 m)
{
/*
  u16 v = x^m;
  u16 vx = 0;

  if(((v & 0x0100) != 0) ^ ((v & 0x0001) != 0))
    vx |= 0x0080;

  if(((v & 0x0008) != 0) ^ ((v & 0x0400) != 0) ^ ((m & 0x1000) && !(x & 0x1000)))
    vx |= 0x0400;

  if(((m & 0x8000) && !(x & 0x8000)))
    vx ^= 0x0040;

  if(((x & 0x0800) && !(m & 0x0800)))
    vx ^= 0x0004;

  return v^vx;
*/
  return x^m;
}

u16 mix1(u16 x, u16 m)
{
  return x^m;
}

u16 mix2(u16 x, u16 m)
{
  return x^m;
}

void seibuspi_sprite_decrypt(data16_t *src, int rom_size)
{
	int i;

	for(i=0; i<rom_size/2; i++) {
		data16_t x;
		x = src[i+0*(rom_size/2)];  src[i+0*(rom_size/2)] = ~mix0(swap0(x, 0), swap0(spi_gm0(i), 0));
		x = src[i+1*(rom_size/2)];  src[i+1*(rom_size/2)] = ~mix1(swap1(x, 0), swap1(spi_gm1(i), 0));
		x = src[i+2*(rom_size/2)];  src[i+2*(rom_size/2)] = ~mix2(swap2(x, 0), swap2(spi_gm2(i), 0));
	}
}

