/* helper function to join 16-bit ROMs and form a 32-bit data stream */
void konami_rom_deinterleave(int memory_region);

/*
You don't have to decode the graphics: the vh_start() routines will do that
for you, using the plane order passed.
Of course the ROM data must be in the correct order. This is a way to ensure
that the ROM test will pass.
The konami_rom_deinterleave() function above will do the reorganization for
you in most cases (but see tmnt.c for additional bit rotations or byte
permutations which may be required).
*/
#define NORMAL_PLANE_ORDER 0,1,2,3
#define REVERSE_PLANE_ORDER 3,2,1,0

/*
The callback is passed:
- layer number (0 = FIX, 1 = A, 2 = B)
- bank (range 0-3, output of the pins CAB1 and CAB2)
- code (range 00-FF, output of the pins VC3-VC10)
  NOTE: code is in the range 0000-FFFF for X-Men, which uses extra RAM
- color (range 00-FF, output of the pins COL0-COL7)
The callback must put:
- in code the resulting tile number
- in color the resulting color index
- if necessary, in flags the flags for the TileMap code (e.g. TILE_FLIPX)
*/
int K052109_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int layer,int bank,int *code,int *color,unsigned char *flags));
void K052109_vh_stop(void);
/* plain 8-bit access */
int K052109_r(int offset);
void K052109_w(int offset,int data);
void K052109_set_RMRD_line(int state);
void K052109_tilemap_update(void);
void K052109_tilemap_draw(struct osd_bitmap *bitmap,int num,int flags);
int K052109_is_IRQ_enabled(void);


/*
The callback is passed:
- code (range 00-1FFF, output of the pins CA5-CA17)
- color (range 00-FF, output of the pins OC0-OC7)
The callback must put:
- in code the resulting sprite number
- in color the resulting color index
- if necessary, in priority the priority of the sprite wrt tilemaps
*/
int K051960_vh_start(int gfx_memory_region,int plane0,int plane1,int plane2,int plane3,
		void (*callback)(int *code,int *color,int *priority));
void K051960_vh_stop(void);
int K051960_r(int offset);
void K051960_w(int offset,int data);
int K051960_word_r(int offset);
void K051960_word_w(int offset,int data);
int K051937_r(int offset);
void K051937_w(int offset,int data);
void K051960_draw_sprites(struct osd_bitmap *bitmap,int min_priority,int max_priority);
void K051960_mark_sprites_colors(void);
int K051960_is_IRQ_enabled(void);

/* special handling for the chips sharing address space */
int K052109_051960_r(int offset);
void K052109_051960_w(int offset,int data);



void K053251_w(int offset,int data);
enum { K053251_CI0=0,K053251_CI1,K053251_CI2,K053251_CI3,K053251_CI4 };
int K053251_get_priority(int ci);
int K053251_get_palette_index(int ci);
