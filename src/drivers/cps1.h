#ifndef _CPS1_H_
#define _CPS1_H_

extern unsigned char * cps1_ram;        /* M68000 RAM */
extern unsigned char * cps1_gfxram;     /* Video RAM */
extern unsigned char * cps1_output;     /* Output ports */
extern int cps1_ram_size;
extern int cps1_gfxram_size;
extern int cps1_output_size;

int cps1_input_r(int offset);    /* Input ports */
int cps1_player_input_r(int offset);    /* Input ports */

int cps1_output_r(int offset);
void cps1_output_w(int offset, int data);
int cps1_gfxram_r(int offset);
void cps1_gfxram_w(int offset, int data);

int cps1_interrupt(void);       /* Ghouls and Ghosts */
int cps1_interrupt2(void);      /* Everything else */
int cps1_interrupt3(void);      /* (apart from Street Fighter) */

int  cps1_vh_start(void);
void cps1_vh_stop(void);
void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* Game specific data */
struct CPS1config
{
        char *name;             /* game driver name */
        int   bank_obj;
        int   bank_scroll1;
        int   bank_scroll2;
        int   bank_scroll3;
        int   alternative;      /* Game alternative */
        int   cpsb_addr;        /* CPS board B test register address */
        int   cpsb_value;       /* CPS board B test register expected value */
        int   gng_sprite_kludge;  /* Ghouls n Ghosts sprite kludge */
};

extern struct CPS1config *cps1_game_config;


/* Sound hardware */
//extern int cps1_sh_start(void);
//extern void cps1_sample_w(int offset, int value);

#endif
