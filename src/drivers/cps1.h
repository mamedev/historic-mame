
/* Scroll 1 size (characters) */
extern int cps1_scroll1_size;
extern int cps1_scroll2_size;
extern int cps1_scroll3_size;
extern int cps1_obj_size;
extern int cps1_work_ram_size;
extern int cps1_palette_size;
extern int cps1_output_size;
extern int cps1_ram_size;

extern unsigned char * cps1_scroll1;
extern unsigned char * cps1_scroll2;
extern unsigned char * cps1_scroll3;
extern unsigned char * cps1_obj;
extern unsigned char * cps1_work_ram;
extern unsigned char * cps1_palette;
extern unsigned char * cps1_output;
extern unsigned char * cps1_ram;

extern int cps1_scroll1_r(int offset);
extern void cps1_scroll1_w(int offset, int value);
extern int cps1_scroll2_r(int offset);
extern void cps1_scroll2_w(int offset, int value);
extern int cps1_scroll3_r(int offset);
extern void cps1_scroll3_w(int offset, int value);
extern int cps1_obj_r(int offset);
extern void cps1_obj_w(int offset, int value);
extern int cps1_work_ram_r(int offset);
extern void cps1_work_ram_w(int offset, int value);
extern int cps1_palette_r(int offset);
extern void cps1_palette_w(int offset, int value);

extern void cps1_output_w(int offset, int value);

extern int cps1_input_r(int offset);

/* Object RAM */
extern int cps1_obj_size;
extern unsigned char *cps1_obj;

extern int cps1_paletteram_size;
extern unsigned char *cps1_paletteram;

extern int cps1_interrupt(void);       /* Ghouls and Ghosts */
extern int cps1_interrupt2(void);      /* Strider */

extern int  cps1_vh_start(void);
extern void cps1_vh_stop(void);
extern void cps1_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void cps1_vh_screenrefresh(struct osd_bitmap *bitmap);

struct CPS1config
{
        char *name;
        int   base_scroll1;
        int   base_obj;
        int   base_scroll2;
        int   base_scroll3;
        int   code_start;
        int   alternative;
};

extern struct CPS1config *cps1_game_config;


/* Sound hardware */
extern int cps1_sh_start(void);
extern void cps1_sample_w(int offset, int value);
extern struct ADPCMsample cps1_sample_data[];






