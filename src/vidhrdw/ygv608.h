#ifndef __YGV608_H__
#define __YGV608_H__

/*
 *    Yamaha YGV608 - PVDC2 Pattern mode Video Display Controller 2
 *    - Mark McDougall
 */

// Fundamental contants
#define YGV608_SPRITE_ATTR_TABLE_SIZE 256

typedef struct _ygv_ports {

  // P#0 - pattern name table data port (read/write)
  unsigned na:8;

  // P#1 - sprite data port (read/write)
  unsigned p1:8;

  // P#2 - scroll data port (read/write)
  unsigned p2:8;

  // P#3 - colour palette data port (read/write)
  unsigned p3:8;

  // P#4 - register data port (read/write)
  unsigned p4:8;

  // P#5 - register select port (write only)
  unsigned rn:6;
  unsigned rrai:1;
  unsigned rwai:1;

  // P#6 - status port (read/write)
  unsigned vb:1;
  unsigned hb:1;
  unsigned fc:1;
  unsigned fv:1;
  unsigned fp:1;
  unsigned _res6:3;

  // P#7 - system control port (read/write)
  unsigned sr:1;
  unsigned tn:1;
  unsigned ts:1;
  unsigned tl:1;
  unsigned tc:1;
  unsigned tr:1;
  unsigned _res7:2;

} YGV_PORTS, *pYGV_PORTS;

typedef struct _ygv_regs {

  // R#0 - pattern name table access ptr (r/w)
  unsigned pny:6;
  unsigned b_a:1;
  unsigned pnya:1;

  // R#1 - pattern name table access ptr (r/w)
  unsigned pnx:6;
  unsigned _res1:1;
  unsigned pnxa:1;

  // R#2 - built in ram access control
  unsigned saar:1;
  unsigned saaw:1;
  unsigned scar:1;
  unsigned scaw:1;
  unsigned p2_b_a:1;
  unsigned _res2:1;
  unsigned cpar:1;
  unsigned cpaw:1;

  // R#3 - sprite attribute table access ptr (r/w)
  unsigned saa:8;

  // R#4 - scroll table access ptr (r/w)
  unsigned sca:8;

  // R#5 - color palette access ptr (r/w)
  unsigned cc:8;

  // R#6 - sprite generator base address (r/w)
  unsigned sba:8;

  // R#7 - R#11 - screen control (r/w)

  unsigned dspe:1;
  unsigned md:2;
  unsigned zron:1;
  unsigned _res7:2;
  unsigned flip:1;
  unsigned dckm:1;

  unsigned pgs:1;
  unsigned _res8:1;
  unsigned rlsc:1;
  unsigned rlrt:1;
  unsigned vds:2;
  unsigned hds:2;

  unsigned slv:3;
  unsigned slh:3;
  unsigned pts:2;

  unsigned mca:2;
  unsigned mcb:2;
  unsigned sprd:1;
  unsigned spas:1;
  unsigned spa:2;

  unsigned ctpa:1;
  unsigned ctpb:1;
  unsigned prm:2;
  unsigned cbdr:1;
  unsigned yse:1;
  unsigned scm:2;

  // R#12 - color palette selection (r/w)
  unsigned apf:3;
  unsigned bpf:3;
  unsigned spf:2;

  // R#13 - border colour (wo)
  unsigned bdc:8;

  // R#14 - R#16 - interrupt control

  unsigned iev:1;
  unsigned iep:1;
  unsigned _res14:6;

  unsigned il:8;

  unsigned ih:5;
  unsigned _res16:1;
  unsigned il8:1;
  unsigned fpm:1;

  // R#17 - R#24 - base address (wo)

  unsigned ba0:3;
  unsigned _res17_1:1;
  unsigned ba1:3;
  unsigned _res17_2:1;

  unsigned ba2:3;
  unsigned _res18_1:1;
  unsigned ba3:3;
  unsigned _res18_2:1;

  unsigned ba4:3;
  unsigned _res19_1:1;
  unsigned ba5:3;
  unsigned _res19_2:1;

  unsigned ba6:3;
  unsigned _res20_1:1;
  unsigned ba7:3;
  unsigned _res20_2:1;

  unsigned bb0:3;
  unsigned _res21_1:1;
  unsigned bb1:3;
  unsigned _res21_2:1;

  unsigned bb2:3;
  unsigned _res22_1:1;
  unsigned bb3:3;
  unsigned _res22_2:1;

  unsigned bb4:3;
  unsigned _res23_1:1;
  unsigned bb5:3;
  unsigned _res23_2:1;

  unsigned bb6:3;
  unsigned _res24_1:1;
  unsigned bb7:3;
  unsigned _res24_2:1;

  // R#25 - R#38 - enlargement, contraction and rotation parameters (wo)

  unsigned ax0:8;

  unsigned ax8:8;

  unsigned ax16:5;
  unsigned _res27:3;

  unsigned dx0:8;

  unsigned dx8:5;
  unsigned _res29:3;

  unsigned dxy0:8;

  unsigned dxy8:5;
  unsigned _res31:3;

  unsigned ay0:8;

  unsigned char ay8;

  unsigned ay16:5;
  unsigned _res34:3;

  unsigned dy0:8;

  unsigned dy8:5;
  unsigned _res36:3;

  unsigned dyx0:8;

  unsigned dyx8:5;
  unsigned _res38:3;

  // R#39 - R#46 - display scan control (wo)

  unsigned hbw:5;
  unsigned hsw:3;

  unsigned hdw:6;
  unsigned htl89:2;

  unsigned hdsp:8;

  unsigned htl:8;

  unsigned vbw:5;
  unsigned vsw:3;

  unsigned vdw:6;
  unsigned vsls:1;
  unsigned _res44:1;

  unsigned vdp:6;
  unsigned tres:1;
  unsigned vtl8:1;

  unsigned vtl:8;

  // R#47 - R#49 - rom transfer control (wo)

  unsigned tb5:8;

  unsigned tb13:8;

  unsigned tn4:8;

} YGV_REGS, *pYGV_REGS;

#define YGV608_MAX_SPRITES (YGV608_SPRITE_ATTR_TABLE_SIZE>>2)

// R#7(md)
#define MD_2PLANE_8BIT      0x00
#define MD_2PLANE_16BIT     0x01
#define MD_1PLANE_16COLOUR  0x02
#define MD_1PLANE_256COLOUR 0x03
#define MD_1PLANE           (MD_1PLANE_16COLOUR & MD_1PLANE_256COLOUR)
#define MD_MASK             0x06

// R#8
#define PGS_64X32         0x0
#define PGS_32X64         0x1
#define PGS_MASK          0x01

// R#9
#define SLV_SCREEN        0x00
#define SLV_8             0x04
#define SLV_16            0x05
#define SLV_32            0x06
#define SLV_64            0x07
#define SLH_SCREEN        0x00
#define SLH_8             0x04
#define SLH_16            0x05
#define SLH_32            0x06
#define SLH_64            0x07
#define PTS_8X8           0x00
#define PTS_16X16         0x01
#define PTS_32X32         0x02
#define PTS_64X64         0x03
#define PTS_MASK          0xC0

// R#10
#define SPAS_SPRITESIZE    0
#define SPAS_SPRITEREVERSE 1

// R#10(spas)=1
#define SZ_8X8            0x00
#define SZ_16X16          0x01
#define SZ_32X32          0x02
#define SZ_64X64          0x03

// R#10(spas)=0
#define SZ_NOREVERSE      0x00
#define SZ_VERTREVERSE    0x01
#define SZ_HORIZREVERSE   0x02
#define SZ_BOTHREVERSE    0x03

// R#11(prm)
#define PRM_SABDEX        0x00
#define PRM_ASBDEX        0x01
#define PRM_SEABDX        0x02
#define PRM_ASEBDX        0x03

// R#40
#define HDW_MASK          0x3f

// R#44
#define VDW_MASK          0x3f

typedef struct {

  unsigned sy:8;    // y dot position 7:0
  unsigned sx:8;    // x dot position 7:0
  unsigned sy8:1;   // y dot position 8 (0-511)
  unsigned sx8:1;   // x dot position 8 (0-511)
  unsigned sz:2;    // size, reverse display
  unsigned sc4:4;   // colour palette
  unsigned sn:8;    // pattern name (0-255)

} SPRITE_ATTR, *PSPRITE_ATTR;


typedef struct _ygv608 {

  union {
    unsigned char b[8];
    YGV_PORTS     s;
  } ports;

  union {
    unsigned char b[50];
    YGV_REGS      s;
  } regs;

  /*
   *  Built in ram
   */

  unsigned char pattern_name_table[4096];

  union {
    unsigned char b[YGV608_SPRITE_ATTR_TABLE_SIZE];
    SPRITE_ATTR   s[YGV608_MAX_SPRITES];
  } sprite_attribute_table;

  unsigned char scroll_data_table[2][256];
  unsigned char colour_palette[256][3];

  /*
   *  Shortcut variables
   */

  unsigned int bits16;          // bits per pattern (8/16)
  unsigned int page_x, page_y;  // pattern page size
  unsigned int pny_shift;       // y coord multiplier
  unsigned char na8_mask;       // mask on/off na11/9:8
  int col_shift;                // shift in scroll table column index

  // rotation, zoom shortcuts
  UINT32 ax, dx, dxy, ay, dy, dyx;

  // base address shortcuts
  unsigned int base_addr[2][8];
  unsigned int base_y_shift;    // for extracting pattern y coord 'base'

  unsigned char screen_resize;  // screen requires resize
  unsigned char tilemap_resize; // tilemap requires resize

} YGV608, *pYGV608;


int ygv608_timed_interrupt( void );
int  ygv608_vh_start( void );
void ygv608_vh_stop( void );
void ygv608_vh_update( struct osd_bitmap *bitmap, int full_refresh );

READ16_HANDLER( ygv608_r );
WRITE16_HANDLER( ygv608_w );

#endif



