/*************************************************************************

	Driver for Atari polygon racer games

**************************************************************************/

/*----------- defined in machine/harddriv.c -----------*/

extern data16_t *hdmsp_ram;
extern data16_t *hd68k_slapstic_base;
extern data16_t *hddsk_ram;
extern data16_t *hddsk_rom;
extern data16_t *hddsk_zram;

extern data16_t *hdgsp_speedup_addr[2];
extern offs_t hdgsp_speedup_pc;

extern data16_t *hdmsp_speedup_addr;
extern offs_t hdmsp_speedup_pc;

extern offs_t hdadsp_speedup_pc;

MACHINE_INIT( harddriv );

INTERRUPT_GEN( hd68k_vblank_gen );
INTERRUPT_GEN( hd68k_irq_gen );
WRITE16_HANDLER( hd68k_irq_ack_w );
void hdgsp_irq_gen(int state);
void hdmsp_irq_gen(int state);

READ16_HANDLER( hd68k_port0_r );
WRITE16_HANDLER( hdgsp_io_w );

READ16_HANDLER( hd68k_gsp_io_r );
WRITE16_HANDLER( hd68k_gsp_io_w );

READ16_HANDLER( hd68k_msp_io_r );
WRITE16_HANDLER( hd68k_msp_io_w );

READ16_HANDLER( hd68k_adsp_program_r );
WRITE16_HANDLER( hd68k_adsp_program_w );
READ16_HANDLER( hd68k_adsp_data_r );
WRITE16_HANDLER( hd68k_adsp_data_w );
READ16_HANDLER( hd68k_adsp_buffer_r );
WRITE16_HANDLER( hd68k_adsp_buffer_w );
WRITE16_HANDLER( hd68k_adsp_irq_clear_w );
WRITE16_HANDLER( hd68k_adsp_control_w );
READ16_HANDLER( hd68k_adsp_irq_state_r );

WRITE16_HANDLER( hd68k_ds3_control_w );
READ16_HANDLER( hd68k_ds3_irq_state_r );
READ16_HANDLER( hdds3_special_r );
WRITE16_HANDLER( hdds3_special_w );
READ16_HANDLER( hd68k_ds3_program_r );
WRITE16_HANDLER( hd68k_ds3_program_w );

READ16_HANDLER( hd68k_sound_reset_r );
WRITE16_HANDLER( hd68k_nwr_w );

READ16_HANDLER( hd68k_adc8_r );
READ16_HANDLER( hd68k_adc12_r );

READ16_HANDLER( hd68k_zram_r );
WRITE16_HANDLER( hd68k_zram_w );

READ16_HANDLER( hdadsp_special_r );
WRITE16_HANDLER( hdadsp_special_w );

READ16_HANDLER( hd68k_duart_r );
WRITE16_HANDLER( hd68k_duart_w );

WRITE16_HANDLER( hd68k_wr0_write );
WRITE16_HANDLER( hd68k_wr1_write );
WRITE16_HANDLER( hd68k_wr2_write );
WRITE16_HANDLER( hd68k_adc_control_w );

READ16_HANDLER( hddsk_ram_r );
WRITE16_HANDLER( hddsk_ram_w );
READ16_HANDLER( hddsk_zram_r );
WRITE16_HANDLER( hddsk_zram_w );
READ16_HANDLER( hddsk_rom_r );

WRITE16_HANDLER( racedriv_68k_slapstic_w );
READ16_HANDLER( racedriv_68k_slapstic_r );

WRITE16_HANDLER( racedriv_asic65_w );
READ16_HANDLER( racedriv_asic65_r );
READ16_HANDLER( racedriv_asic65_io_r );

WRITE16_HANDLER( racedriv_asic61_w );
READ16_HANDLER( racedriv_asic61_r );

WRITE16_HANDLER( steeltal_68k_slapstic_w );
READ16_HANDLER( steeltal_68k_slapstic_r );

READ16_HANDLER( hdadsp_speedup_r );
READ16_HANDLER( hdadsp_speedup2_r );
WRITE16_HANDLER( hdadsp_speedup2_w );

READ16_HANDLER( hdgsp_speedup_r );
WRITE16_HANDLER( hdgsp_speedup1_w );
WRITE16_HANDLER( hdgsp_speedup2_w );

READ16_HANDLER( hdmsp_speedup_r );
WRITE16_HANDLER( hdmsp_speedup_w );


/*----------- defined in sndhrdw/harddriv.c -----------*/

READ_HANDLER( hdsnd_68k_r );
WRITE_HANDLER( hdsnd_68k_w );

READ_HANDLER( hdsnd_data_r );
WRITE_HANDLER( hdsnd_data_w );

READ_HANDLER( hdsnd_320port_r );
READ_HANDLER( hdsnd_switches_r );
READ_HANDLER( hdsnd_status_r );
READ_HANDLER( hdsnd_320ram_r );
READ_HANDLER( hdsnd_320com_r );

WRITE_HANDLER( hdsnd_latches_w );
WRITE_HANDLER( hdsnd_speech_w );
WRITE_HANDLER( hdsnd_irqclr_w );
WRITE_HANDLER( hdsnd_320ram_w );
WRITE_HANDLER( hdsnd_320com_w );


/*----------- defined in vidhrdw/harddriv.c -----------*/

extern UINT8 hdgsp_multisync;
extern UINT8 *hdgsp_vram;
extern data16_t *hdgsp_vram_2bpp;
extern data16_t *hdgsp_control_lo;
extern data16_t *hdgsp_control_hi;
extern data16_t *hdgsp_paletteram_lo;
extern data16_t *hdgsp_paletteram_hi;
extern size_t hdgsp_vram_size;

VIDEO_START( harddriv );
void hdgsp_write_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void hdgsp_read_from_shiftreg(UINT32 address, UINT16 *shiftreg);
void hdgsp_display_update(UINT32 offs, int rowbytes, int scanline);

READ16_HANDLER( hdgsp_control_lo_r );
WRITE16_HANDLER( hdgsp_control_lo_w );
READ16_HANDLER( hdgsp_control_hi_r );
WRITE16_HANDLER( hdgsp_control_hi_w );

READ16_HANDLER( hdgsp_vram_2bpp_r );
WRITE16_HANDLER( hdgsp_vram_1bpp_w );
WRITE16_HANDLER( hdgsp_vram_2bpp_w );

READ16_HANDLER( hdgsp_paletteram_lo_r );
WRITE16_HANDLER( hdgsp_paletteram_lo_w );
READ16_HANDLER( hdgsp_paletteram_hi_r );
WRITE16_HANDLER( hdgsp_paletteram_hi_w );

VIDEO_EOF( harddriv );
VIDEO_UPDATE( harddriv );
