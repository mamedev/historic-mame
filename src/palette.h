/******************************************************************************

  palette.h

  Palette handling functions.

  There are several levels of abstraction in the way MAME handles the palette,
  and several display modes which can be used by the drivers.

  Palette
  -------
  Note: in the following text, "color" refers to a color in the emulated
  game's virtual palette. For example, a game might be able to display 1024
  colors at the same time. If the game uses RAM to change the available
  colors, the term "palette" refers to the colors available at any given time,
  not to the whole range of colors which can be produced by the hardware. The
  latter is referred to as "color space".
  The term "pen" refers to one of the maximum of 256 colors can use to
  generate the display. PC users might want to think of them as the colors
  available in VGA, but be warned: the mapping of MAME pens to the VGA
  registers is not 1:1, so MAME's pen 10 will not necessarily be mapped to
  VGA's color #10 (actually this is never the case). This is done to ensure
  portability, since on some systems it is not possible to do a 1:1 mapping.

  So, to summarize, the three layers of palette abstraction are:

  P1) The game virtual palette (the "colors")
  P2) MAME's 256 colors palette (the "pens")
  P3) The OS specific hardware color registers (the "OS specific pens")

  The array Machine->pens is a lookup table which maps game colors to OS
  specific pens (P1 to P3). When you are working on bitmaps at the pixel level,
  *always* use Machine->pens to map the color numbers. *Never* use constants.
  For example if you want to make pixel (x,y) of color 3, do:
  bitmap->line[y][x] = Machine->pens[3];


  Lookup table
  ------------
  Palettes are only half of the story. To map the gfx data to colors, the
  graphics routines use a lookup table. For example if we have 4bpp tiles,
  which can have 256 different color codes, the lookup table for them will have
  256 * 2^4 = 4096 elements. For games using a palette RAM, the lookup table is
  usually a 1:1 map. For games using PROMs, the lookup table is often larger
  than the palette itself so for example the game can display 256 colors out
  of a palette of 16.

  The palette and the lookup table are initialized to default values by the
  main core, but can be initialized by the driver using the function
  MachineDriver->vh_convert_color_prom(). For games using palette RAM, that
  function is usually not needed, and the lookup table can be set up by
  properly initializing the color_codes_start and total_color_codes fields in
  the GfxDecodeInfo array.
  When vh_convert_color_prom() initializes the lookup table, it maps gfx codes
  to game colors (P1 above). The lookup table will be converted by the core to
  map to OS specific pens (P3 above), and stored in Machine->colortable.


  Display modes
  -------------
  The avaialble display modes can be summarized in four categories:
  1) Static palette. Use this for games which use PROMs for color generation.
     The palette is initialized by vh_convert_color_prom(), and never changed
     again.
  2) Dynamic palette. Use this for games which use RAM for color generation and
     have no more than 256 colors in the palette. The palette can be
     dynamically modified by the driver using the function
     palette_change_color(). MachineDriver->video_attributes must contain the
     flag VIDEO_MODIFIES_PALETTE.
  3) Dynamic shrinked palette. Use this for games which use RAM for color
     generation and have more than 256 colors in the palette. The palette can
     be dynamically modified by the driver using the function
     palette_change_color(). MachineDriver->video_attributes must contain the
     flag VIDEO_MODIFIES_PALETTE.
     The difference with case 2) above is that the driver must do some
     additional work to allow for palette reduction without loss of quality.
     The function palette_recalc() must be called every frame before doing any
     rendering. The argument to the function is an array telling which of the
     game colors are used, and how. This way the core can pick only the needed
     colors, and make the palette fit into 256 colors. The return code of
     palette_recalc() tells the driver whether the lookup table has changed,
	 and therefore whether a screen refresh is needed.
  4) 16-bit color. This should only be used for games which use more than 256
     colors at a time. It is slower and more awkward to use than the other
     modes, so it should be avoided whenever possible.
	 When this mode is used, the driver must manually modify the lookup table
	 to match changes in the palette. But video can be downgraded to 8-bit, but
	 with a noticeable decrease in quality.
     MachineDriver->video_attributes must contain the flag
     VIDEO_SUPPPORTS_16BIT.

  The important difference between cases 3) and 4) is that in 3), color cycling
  (which many games use) is essentially free, while in 4) every palette change
  requires a screen refresh. The color quality in 3) is also better than in 4).
  The dynamic shrinking of the palette works this way: as colors are requested,
  they are associated to a pen. When a color is no longer needed, the pen is
  freed and can be used for another color. When the code runs out of free pens,
  it compacts the palette, putting together colors with the same RGB
  components, then starts again to allocate pens for each new color. The bottom
  line of all this is that the pen assignment will automatically adapt to the
  game needs, and colors which change often will be assigned an exclusive pen,
  which can be modified using the video cards hardware registers without need
  for a screen refresh.

******************************************************************************/

#ifndef PALETTE_H
#define PALETTE_H

#define MAX_PENS 256	/* unless 16-bit mode is used, can't handle more */
						/* than 256 colors on screen */

int palette_start(void);
void palette_stop(void);
void palette_init(void);

void palette_change_color(int color,unsigned char red,unsigned char green,unsigned char blue);

/* This array is used by palette_recalc() to know which colors are used, and which */
/* ones are transparent (see defines below). By default, it is initialized to */
/* PALETTE_COLOR_USED for all colors; this is enough in some cases. */
extern unsigned char *palette_used_colors;

int palette_recalc(void);

#define PALETTE_COLOR_UNUSED 0		/* This color is not needed for this frame */
#define PALETTE_COLOR_USED 1		/* This color is currenly used, either in the */
	/* visible screen itself, or for parts which are cached in temporary bitmaps */
	/*by the driver. */
	/* palette_recalc() will try to use always the same pens for the used colors; */
	/* if it is forced to rearrange the pens, it will return TRUE to signal the */
	/* driver that it must refresh the display. */
#define PALETTE_COLOR_TRANSPARENT 2	/* All colors using this attribute will be */
	/* mapped to the same pen, and no other colors will be mapped to that pen. */
	/* This way, transparencies can be handled by copybitmap(). */

/* if you use PALETTE_COLOR_TRANSPARENT, to do a transparency blit with copybitmap() */
/* pass it TRANSPARENCY_PEN, palette_transparent_pen. */
extern unsigned short palette_transparent_pen;

/* The transparent color can also be used as a background color, to avoid doing */
/* a fillbitmap() + copybitmap() when there is nothing between the background and */
/* the transparent layer. Use this function to set the background color. */
void palette_change_transparent_color(unsigned char red,unsigned char green,unsigned char blue);


/* helper macros for 16-bit support: */
#define rgbpenindex(r,g,b) ((Machine->scrbitmap->depth==16) ? ((((r)>>3)<<10)+(((g)>>3)<<5)+((b)>>3)) : ((((r)>>5)<<5)+(((g)>>5)<<2)+((b)>>6)))
#define rgbpen(r,g,b) (Machine->pens[rgbpenindex(r,g,b)])
#define setgfxcolorentry(gfx,i,r,g,b) ((gfx)->colortable[i] = rgbpen (r,g,b))

#endif
