extern data8_t *news_fgram;
extern data8_t *news_bgram;

WRITE8_HANDLER( news_fgram_w );
WRITE8_HANDLER( news_bgram_w );
WRITE8_HANDLER( news_bgpic_w );
VIDEO_START( news );
VIDEO_UPDATE( news );
