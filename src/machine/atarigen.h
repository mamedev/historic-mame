/***************************************************************************

  atarigen.h

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/

#define COLOR_PALETTE_555 1
#define COLOR_PALETTE_4444 2

void atarigen_init_machine (void (*sound_int)(void), int slapstic);

void atarigen_eeprom_enable_w (int offset, int data);
int atarigen_eeprom_r (int offset);
void atarigen_eeprom_w (int offset, int data);

int atarigen_hiload (void);
void atarigen_hisave (void);

int atarigen_slapstic_r (int offset);
void atarigen_slapstic_w (int offset, int data);

void atarigen_sound_reset (void);
void atarigen_sound_w (int offset, int data);
int atarigen_6502_sound_r (int offset);

void atarigen_6502_sound_w (int offset, int data);
int atarigen_sound_r (int offset);

void atarigen_init_remap (int _colortype);
void atarigen_alloc_fixed_colors (int *usage, int base, int colors, int palettes);
void atarigen_alloc_dynamic_colors (int base, int number);
void atarigen_update_colors (int intensity);

extern int atarigen_cpu_to_sound, atarigen_cpu_to_sound_ready;
extern int atarigen_sound_to_cpu, atarigen_sound_to_cpu_ready;
extern int atarigen_special_color;

extern unsigned char *atarigen_paletteram;
extern unsigned char *atarigen_playfieldram;
extern unsigned char *atarigen_spriteram;
extern unsigned char *atarigen_alpharam;
extern unsigned char *atarigen_vscroll;
extern unsigned char *atarigen_hscroll;
extern unsigned char *atarigen_eeprom;
extern unsigned char *atarigen_slapstic;

extern int atarigen_playfieldram_size;
extern int atarigen_spriteram_size;
extern int atarigen_alpharam_size;
extern int atarigen_paletteram_size;
extern int atarigen_eeprom_size;
