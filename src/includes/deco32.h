
VIDEO_START( captaven );
VIDEO_START( fghthist );
VIDEO_START( dragngun );
VIDEO_START( tattass );

VIDEO_EOF( captaven );

VIDEO_UPDATE( captaven );
VIDEO_UPDATE( fghthist );
VIDEO_UPDATE( dragngun );
VIDEO_UPDATE( tattass );

extern data32_t *deco32_pf1_data,*deco32_pf2_data,*deco32_pf3_data,*deco32_pf4_data;
extern data32_t *deco32_pf12_control,*deco32_pf34_control;
extern data32_t *deco32_pf1_colscroll,*deco32_pf2_colscroll,*deco32_pf3_colscroll,*deco32_pf4_colscroll;
extern data32_t *deco32_pf1_rowscroll,*deco32_pf2_rowscroll,*deco32_pf3_rowscroll,*deco32_pf4_rowscroll;

WRITE32_HANDLER( deco32_pf1_data_w );
WRITE32_HANDLER( deco32_pf2_data_w );
WRITE32_HANDLER( deco32_pf3_data_w );
WRITE32_HANDLER( deco32_pf4_data_w );
WRITE32_HANDLER( deco32_paletteram_w );
WRITE32_HANDLER( deco32_pri_w );
WRITE32_HANDLER( deco32_unknown_w );
