/* Video emulation definitions */
extern data16_t *cninja_pf1_rowscroll,*cninja_pf2_rowscroll;
extern data16_t *cninja_pf3_rowscroll,*cninja_pf4_rowscroll;
extern data16_t *cninja_pf1_data,*cninja_pf2_data;
extern data16_t *cninja_pf3_data,*cninja_pf4_data;

int  cninja_vh_start(void);
int  edrandy_vh_start(void);
int  robocop2_vh_start(void);
int  stoneage_vh_start(void);
void cninja_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void edrandy_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void robocop2_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( cninja_pf1_data_w );
WRITE16_HANDLER( cninja_pf2_data_w );
WRITE16_HANDLER( cninja_pf3_data_w );
WRITE16_HANDLER( cninja_pf4_data_w );
WRITE16_HANDLER( cninja_control_0_w );
WRITE16_HANDLER( cninja_control_1_w );

WRITE16_HANDLER( cninja_palette_24bit_w );
WRITE16_HANDLER( robocop2_pri_w );
