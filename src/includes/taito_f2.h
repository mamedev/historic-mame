
extern data16_t *f2_sprite_extension;
extern size_t f2_spriteext_size;

int taitof2_default_vh_start (void);
int taitof2_quiz_vh_start (void);
int taitof2_finalb_vh_start (void);
int taitof2_megab_vh_start (void);
int taitof2_solfigtr_vh_start (void);
int taitof2_koshien_vh_start (void);
int taitof2_driftout_vh_start (void);
int taitof2_dondokod_vh_start (void);
int taitof2_thundfox_vh_start (void);
int taitof2_growl_vh_start (void);
int taitof2_yuyugogo_vh_start (void);
int taitof2_mjnquest_vh_start (void);
int taitof2_footchmp_vh_start (void);
int taitof2_hthero_vh_start (void);
int taitof2_ssi_vh_start (void);
int taitof2_gunfront_vh_start (void);
int taitof2_ninjak_vh_start (void);
int taitof2_pulirula_vh_start (void);
int taitof2_metalb_vh_start (void);
int taitof2_qzchikyu_vh_start (void);
int taitof2_yesnoj_vh_start (void);
int taitof2_deadconx_vh_start (void);
int taitof2_deadconj_vh_start (void);
int taitof2_dinorex_vh_start (void);
void taitof2_vh_stop (void);
void taitof2_no_buffer_eof_callback(void);
void taitof2_full_buffer_delayed_eof_callback(void);
void taitof2_partial_buffer_delayed_eof_callback(void);
void taitof2_partial_buffer_delayed_thundfox_eof_callback(void);
void taitof2_partial_buffer_delayed_qzchikyu_eof_callback(void);

void taitof2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void taitof2_pri_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void taitof2_pri_roz_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void ssi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void thundfox_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void deadconx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void metalb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void yesnoj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( taitof2_spritebank_w );
READ16_HANDLER ( koshien_spritebank_r );
WRITE16_HANDLER( koshien_spritebank_w );
WRITE16_HANDLER( taitof2_sprite_extension_w );

extern data16_t *cchip_ram;
READ16_HANDLER ( cchip2_word_r );
WRITE16_HANDLER( cchip2_word_w );

