/***************************************************************************
					ToaPlan  (1988-1991 hardware)
 ***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"



int  video_ofs3_r(int offset);
void video_ofs3_w(int offset, int data);
void toaplan1_videoram3_w(int offset, int data);

int toaplan1_coin_count; /* coin count increments on startup ? , so dont count it */
int toaplan1_hs_count;	 /* Some games write default HS to screen a few times, so overwrite it many times */
int toaplan1_hs_bank;	 /* MAME main RAM bank to check HS table in */
int toaplan1_hs_start;	 /* offset in main RAM bank to start of HS table */
int toaplan1_hs_end;	 /* offset in main RAM bank to  end  of HS table */
int toaplan1_hs_t_hi;	 /* first value at start of table to test */
int toaplan1_hs_t_lo;
int toaplan1_hs_t_e1;	 /* last value at end of table to test */
int toaplan1_hs_t_e2;
int toaplan1_hs_tile;	 /* tile data to 'or' new screen values with */
int toaplan1_hs_offs;	 /* Screen high-score RAM offset to the most significant digit that can possibly be shown */

int toaplan1_int_enable;
static int unk;
static int credits;
static int latch;

extern unsigned char *toaplan1_sharedram;



int rallybik_rom_r(int offset)
{
	unsigned char *m = Machine->memory_region[0];
	// return the actual rom values not the patched ones.
	if (offset == 0x18d4) return 0x6622 ;
	if (offset == 0x18f0) return 0x6606 ;

	return READ_WORD(m+offset) ;
}

int toaplan1_interrupt(void)
{
	if (toaplan1_int_enable)
		return MC68000_IRQ_4;
	return MC68000_INT_NONE;
}

void toaplan1_int_enable_w(int offset, int data)
{
	toaplan1_int_enable = data;
}

int toaplan1_unk_r(int offset)
{
	return unk ^= 1;
}

int vimana_input_port_4_r(int offset)
{
	int data, p;

	p = input_port_4_r(0);

	latch ^= p;
	data = (latch & p );

	/* simulate the mcu keeping track of credits */
	/* latch is so is doesn't add more than one */
	/* credit per keypress */

	if (data & 0x18)
	{
		credits++ ;
	}

	latch = p;

	return p;
}

int vimana_mcu_r(int offset)
{
	int data = 0 ;
	switch (offset >> 1)
	{
		case 0:
			data = 0xff;
			break;
		case 1:
			data = 0;
			break;
		case 2:
			data = credits;
			break;
	}
	return data;
}
void vimana_mcu_w(int offset, int data)
{
	switch (offset >> 1)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			credits = data;
			break;
	}
}

int toaplan1_shared_r(int offset)
{
	return toaplan1_sharedram[offset>>1];
}

void toaplan1_shared_w(int offset, int data)
{
	toaplan1_sharedram[offset>>1] = data;
}

void toaplan1_init_machine(void)
{
	toaplan1_int_enable = 0;
	unk = 0;
	credits = 0;
	latch = 0;
	toaplan1_hs_count = 0;
	toaplan1_coin_count = 0;
	coin_lockout_global_w(0,0);

	/*** Dirty the high score table ***/
	WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_start+4)]),0xffff);
	WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_start+6)]),0xffff);
	WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_end-4)]),0xffff);
	WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_end-2)]),0xffff);
}

void rallybik_init_machine(void)
{
	unsigned char *m = Machine->memory_region[0];
	WRITE_WORD(m+0x18d4,0x4e71); /* NOP */
	WRITE_WORD(m+0x18f0,0x4e71); /* NOP */

	toaplan1_hs_start = 0x01ac;	/* table starts at $801ac */
	toaplan1_hs_end  = 0x04d0;
	toaplan1_hs_bank = 1;
	toaplan1_hs_t_hi = 0x0004;	/* hs=40000 */
	toaplan1_hs_t_lo = 0x0000;
	toaplan1_hs_t_e1 = 0x0039;
	toaplan1_hs_t_e2 = 0x0040;
	toaplan1_hs_tile = 0x3000;
	toaplan1_hs_offs = 0x35a6;

	toaplan1_init_machine();
}

void truxton_init_machine(void)
{
	toaplan1_hs_start = 0x19de - 0x19d8;	/* table starts at $819de */
	toaplan1_hs_end  = 0x1b4a - 0x19d8;
	toaplan1_hs_bank = 2;
	toaplan1_hs_t_hi = 0x0000;	/* hs=50000 */
	toaplan1_hs_t_lo = 0x5000;
	toaplan1_hs_t_e1 = 0x000b;
	toaplan1_hs_t_e2 = 0x000b;
	toaplan1_hs_tile = 0x0000;
	toaplan1_hs_offs = 0x32a6;

	toaplan1_init_machine();
}

void hellfire_init_machine(void)
{
	toaplan1_hs_start = 0x2300 - 0x22f4;	/* table starts at $42300 */
	toaplan1_hs_end  = 0x23c2 - 0x22f4;
	toaplan1_hs_bank = 2;
	toaplan1_hs_t_hi = 0x0000;	/* hs=50000 */
	toaplan1_hs_t_lo = 0x5000;
	toaplan1_hs_t_e1 = 0x000b;
	toaplan1_hs_t_e2 = 0x0008;
	toaplan1_hs_tile = 0x0030;
	toaplan1_hs_offs = 0x304f;

	toaplan1_init_machine();
}

void zerowing_init_machine(void)
{
	toaplan1_hs_start = 0x1776 - 0x1770;	/* table starts at $81776 */
	toaplan1_hs_end  = 0x17de - 0x1770;
	toaplan1_hs_bank = 2;
	toaplan1_hs_t_hi = 0x0000;	/* hs=50000 */
	toaplan1_hs_t_lo = 0x5000;
	toaplan1_hs_t_e1 = 0x0011;
	toaplan1_hs_t_e2 = 0x0010;
	toaplan1_hs_tile = 0x0030;
	toaplan1_hs_offs = 0x3054;

	toaplan1_init_machine();
}

void demonwld_init_machine(void)
{
	unsigned char *m = Machine->memory_region[0];
	WRITE_WORD(m+0x1824,0x600a); /* change BEQ to BRA  */
	WRITE_WORD(m+0x181c,0x4e71); /* NOP  */

	toaplan1_hs_start = 0x01be - 0x01ae;	/* table starts at $c001be */
	toaplan1_hs_end  = 0x028a - 0x01ae;
	toaplan1_hs_bank = 2;
	toaplan1_hs_t_hi = 0x0004;	/* hs=40000 */
	toaplan1_hs_t_lo = 0x0000;
	toaplan1_hs_t_e1 = 0x002d;
	toaplan1_hs_t_e2 = 0x002d;
	toaplan1_hs_tile = 0x0080;
	toaplan1_hs_offs = 0x304f;

	toaplan1_init_machine();
}

void outzone_init_machine(void)
{
	toaplan1_hs_start = 0x01de;			/* table starts at $2401de */
	toaplan1_hs_end  = 0x0552;
	toaplan1_hs_bank = 1;
	toaplan1_hs_t_hi = 0x0020;	/* hs=200000 */
	toaplan1_hs_t_lo = 0x0000;
	toaplan1_hs_t_e1 = 0x003f;
	toaplan1_hs_t_e2 = 0x003f;
	toaplan1_hs_tile = 0x0030;
	toaplan1_hs_offs = 0x32a6;

	toaplan1_init_machine();
}

void vimana_init_machine(void)
{
	toaplan1_hs_start = 0x0198 - 0x0148;	/* table starts at $480198 */
	toaplan1_hs_end  = 0x028c - 0x0148;
	toaplan1_hs_bank = 2;
	toaplan1_hs_t_hi = 0x0002;	/* hs=200000 */
	toaplan1_hs_t_lo = 0x0000;
	toaplan1_hs_t_e1 = 0x004f;
	toaplan1_hs_t_e2 = 0x0041;
	toaplan1_hs_tile = 0x0030;
	toaplan1_hs_offs = 0x32a6;

	toaplan1_init_machine();
}


void rallybik_coin_w(int offset,int data)
{
	switch (data) {
		case 0x08: if (toaplan1_coin_count) { coin_counter_w(0,1); coin_counter_w(0,0); } break;
		case 0x09: if (toaplan1_coin_count) { coin_counter_w(2,1); coin_counter_w(2,0); } break;
		case 0x0a: if (toaplan1_coin_count) { coin_counter_w(1,1); coin_counter_w(1,0); } break;
		case 0x0b: if (toaplan1_coin_count) { coin_counter_w(3,1); coin_counter_w(3,0); } break;
		case 0x0c: coin_lockout_w(0,1); coin_lockout_w(2,1); break;
		case 0x0d: coin_lockout_w(0,0); coin_lockout_w(2,0); break;
		case 0x0e: coin_lockout_w(1,1); coin_lockout_w(3,1); break;
		case 0x0f: coin_lockout_w(1,0); coin_lockout_w(3,0); toaplan1_coin_count=1; break;
	}
}

void toaplan1_coin_w(int offset,int data)
{
	if (errorlog) fprintf(errorlog,"Z80 writing %02x to coin control\n",data);
	/* This still isnt too clear yet. */
	/* Coin C has no coin lock ? */
	/* Are some outputs for lights ? (no space on JAMMA for it though) */

	switch (data) {
		case 0xee: coin_counter_w(1,1); coin_counter_w(1,0); break; /* Count slot B */
		case 0xed: coin_counter_w(0,1); coin_counter_w(0,0); break; /* Count slot A */
	/* The following are coin counts after coin-lock active (faulty coin-lock ?) */
		case 0xe2: coin_counter_w(1,1); coin_counter_w(1,0); coin_lockout_w(1,1); break;
		case 0xe1: coin_counter_w(0,1); coin_counter_w(0,0); coin_lockout_w(0,1); break;

		case 0xec: coin_lockout_global_w(0,0); break;	/* ??? count games played */
		case 0xe8: break;	/* ??? Maximum credits reached with coin/credit ratio */
		case 0xe4: break;	/* ??? Reset coin system */

		case 0x0c: coin_lockout_global_w(0,0); break;	/* Unlock all coin slots */
		case 0x08: coin_lockout_w(2,0); break;	/* Unlock coin slot C */
		case 0x09: coin_lockout_w(0,0); break;	/* Unlock coin slot A */
		case 0x0a: coin_lockout_w(1,0); break;	/* Unlock coin slot B */

		case 0x02: coin_lockout_w(1,1); break;	/* Lock coin slot B */
		case 0x01: coin_lockout_w(0,1); break;	/* Lock coin slot A */
		case 0x00: coin_lockout_global_w(0,1); break;	/* Lock all coin slots */
	}
}


int toaplan1_hiload(void)
{
	/* check the start and end of hi score table */
	/* to make sure its already been initialised */

	if ((READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_start+4)]))==toaplan1_hs_t_hi) &&
		(READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_start+6)]))==toaplan1_hs_t_lo) &&
		(READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_end-4)]))==toaplan1_hs_t_e1) &&
		(READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][(toaplan1_hs_end-2)]))==toaplan1_hs_t_e2))
	{
		void *f;
		toaplan1_hs_count++ ;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			if (toaplan1_hs_count == 1)
			{
				osd_fread_msbfirst(f,
					&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start]),
					(toaplan1_hs_end - toaplan1_hs_start));
				osd_fclose(f);
			}
		}
	}
	if (toaplan1_hs_count)
	{
		/* Write HS to screen, but skip writing Most Significant zeros */
		int tp1_digit[8];
		int tp1_skip_zeros=0;
		int tp1_voffs3_org, tp1_vram_offs;
		int tp1_tmplo, tp1_tmphi, tp1_tmp_cnt;

		tp1_tmphi=READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start+4]));
		tp1_tmplo=READ_WORD(&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start+6]));

		WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start]),tp1_tmphi);
		WRITE_WORD(&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start+2]),tp1_tmplo);

		tp1_digit[0] = (tp1_tmphi >> 12) & 0x000f;
		tp1_digit[1] = (tp1_tmphi >>  8) & 0x000f;
		tp1_digit[2] = (tp1_tmphi >>  4) & 0x000f;
		tp1_digit[3] =  tp1_tmphi        & 0x000f;
		tp1_digit[4] = (tp1_tmplo >> 12) & 0x000f;
		tp1_digit[5] = (tp1_tmplo >>  8) & 0x000f;
		tp1_digit[6] = (tp1_tmplo >>  4) & 0x000f;
		tp1_digit[7] =  tp1_tmplo        & 0x000f;
		tp1_vram_offs = toaplan1_hs_offs;
		tp1_voffs3_org = video_ofs3_r(0);
		for (tp1_tmp_cnt = 0 ; tp1_tmp_cnt < 8 ; tp1_tmp_cnt++)
		{
			if ((tp1_digit[tp1_tmp_cnt] != 0) || tp1_skip_zeros)
			{
				video_ofs3_w(0, tp1_vram_offs);
				toaplan1_videoram3_w(2,(toaplan1_hs_tile + tp1_digit[tp1_tmp_cnt]));
				tp1_skip_zeros = 1;
			}
			tp1_vram_offs++ ;
			if ((Machine->gamedrv->flags & ORIENTATION_MASK) == ORIENTATION_ROTATE_270)
			{
				tp1_vram_offs += 0x3f;
			}
		}
		video_ofs3_w(0, tp1_voffs3_org);
		toaplan1_hs_count++ ;
		if (toaplan1_hs_count >= 0x18) return 1;
	}
	return 0;	/* can't or haven't finished loading hi scores yet */
}

void toaplan1_hisave(void)
{
	void *f;

	if (errorlog) fprintf(errorlog,"Saving the high scores. Count is %08x\n",toaplan1_hs_count);
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,
			&(cpu_bankbase[toaplan1_hs_bank][toaplan1_hs_start]),
			(toaplan1_hs_end - toaplan1_hs_start));
		osd_fclose(f);
	}
}

