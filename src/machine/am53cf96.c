/*
 * am53cf96.c
 *
 * AMD/NCR/Symbios 53CF96 SCSI-2 controller.
 * Qlogic FAS-236 and Emulex ESP-236 are equivalents
 *
 * References:
 * AMD Am53CF96 manual
 *
 * Unimplented SCSI-2 commands used by KonamiGV:
 * 15 = MODE SELECT
 * 1A = MODE SENSE (6)
 * 43 = READ TOC
 * 48 = PLAY AUDIO TRACK/INDEX
 *
 * Incomplete SCSI-2 commands supported:
 * 12 = INQUIRY	(returns "SONY" for manufacturer, no other data)
 * 42 = READ SUB-CHANNEL (type 1 is fully implemented, others are not)
 *
 */

#include "driver.h"
#include "state.h"
#include "harddisk.h"
#include "cdrom.h"
#include "am53cf96.h"

data8_t scsi_regs[32], fifo[16], fptr = 0, last_cmd, xfer_state;
int lba, blocks, last_lba;
struct hard_disk_file *disk;
struct cdrom_file *cdrom;
static struct AM53CF96interface *intf;

// 53CF96 register set
enum
{
	REG_XFERCNTLOW = 0,	// read = current xfer count lo byte, write = set xfer count lo byte
	REG_XFERCNTMID,		// read = current xfer count mid byte, write = set xfer count mid byte
	REG_FIFO,		// read/write = FIFO
	REG_COMMAND,		// read/write = command

	REG_STATUS,		// read = status, write = destination SCSI ID (4)
	REG_IRQSTATE,		// read = IRQ status, write = timeout	      (5)
	REG_INTSTATE,		// read = internal state, write = sync xfer period (6)
	REG_FIFOSTATE,		// read = FIFO status, write = sync offset
	REG_CTRL1,		// read/write = control 1
	REG_CLOCKFCTR,		// clock factor (write only)
	REG_TESTMODE,		// test mode (write only)
	REG_CTRL2,		// read/write = control 2
	REG_CTRL3,		// read/write = control 3
	REG_CTRL4,		// read/write = control 4
	REG_XFERCNTHI,		// read = current xfer count hi byte, write = set xfer count hi byte
	REG_DATAALIGN		// data alignment (write only)
};

READ32_HANDLER( am53cf96_r )
{
	int reg, shift, rv;
	int states[] = { 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 0 };

	reg = offset * 2;
	if (mem_mask == 0xffffff00)
	{
		shift = 0;
	}
	else
	{
		reg++;
		shift = 16;
	}

	if (reg == REG_STATUS)
	{
		scsi_regs[REG_STATUS] &= ~0x7;
		scsi_regs[REG_STATUS] |= states[xfer_state];
		if (xfer_state < 10)
		{
			xfer_state++;
		}
	}

	rv = scsi_regs[reg]<<shift;

	if (reg == REG_FIFO)
	{
//		printf("53cf96: read FIFO PC=%x\n", activecpu_get_pc());
		return 0;
	}

//	logerror("53cf96: read reg %d = %x (PC=%x)\n", reg, rv>>shift, activecpu_get_pc());

	if (reg == REG_IRQSTATE)
	{
		scsi_regs[REG_STATUS] &= ~0x80;	// clear IRQ flag
	}

	return rv;
}

static void am53cf96_irq( int unused )
{
	scsi_regs[REG_IRQSTATE] = 8;	// indicate success
	scsi_regs[REG_STATUS] |= 0x80;	// indicate IRQ
	intf->irq_callback();
}

WRITE32_HANDLER( am53cf96_w )
{
	int reg, val, dma;

	reg = offset * 2;
	val = data;
	if (mem_mask == 0xffffff00)
	{
	}
	else
	{
		reg++;
		val >>= 16;
	}
	val &= 0xff;

//	logerror("53cf96: w %x to reg %d (ofs %02x data %08x mask %08x PC=%x)\n", val, reg, offset, data, mem_mask, activecpu_get_pc());

	if (reg == REG_XFERCNTLOW || reg == REG_XFERCNTMID || reg == REG_XFERCNTHI)
	{
		scsi_regs[REG_STATUS] &= ~0x10;	// clear CTZ bit
	}

	// FIFO
	if (reg == REG_FIFO)
	{
//		printf("%02x to FIFO @ %02d\n", val, fptr);
		fifo[fptr++] = val;
		if (fptr > 15)
		{
			fptr = 15;
		}
	}

	// command
	if (reg == REG_COMMAND)
	{
		dma = (val & 0x80) ? 1 : 0;
		fptr = 0;
		switch (val & 0x7f)
		{
			case 0:	// NOP
				scsi_regs[REG_IRQSTATE] = 8;	// indicate success
				xfer_state = 0;
				break;
			case 3:	// reset SCSI bus
				scsi_regs[REG_INTSTATE] = 4;	// command sent OK
				xfer_state = 0;
				timer_set( TIME_IN_HZ( 16384 ), 0, am53cf96_irq );
				break;
			case 0x42:    	// select with ATN steps
				timer_set( TIME_IN_HZ( 16384 ), 0, am53cf96_irq );
				last_cmd = fifo[1];
//				logerror("53cf96: executing SCSI command %x\n", last_cmd);
				if ((last_cmd == 0) || (last_cmd == 0x48) || (last_cmd == 0x4b))
				{
					scsi_regs[REG_INTSTATE] = 6;
				}
				else
				{
					scsi_regs[REG_INTSTATE] = 4;
				}

				switch (last_cmd)
				{
					case 0:		// TEST UNIT READY
					case 3: 	// REQUEST SENSE
						break;
					case 0x12:	// INQUIRY
						break;
					case 0x15:	// MODE SELECT (used to set CDDA volume)
						logerror("53cf96: MODE SELECT length %x control %x\n", fifo[5], fifo[6]);
						break;
					case 0x1a:	// MODE SENSE
						break;
					case 0x28: 	// READ (10 byte)
						lba = fifo[3]<<24 | fifo[4]<<16 | fifo[5]<<8 | fifo[6];
						blocks = fifo[8]<<8 | fifo[9];

						/* convert physical frame to CHD */
						if (cdrom)
						{
							lba = cdrom_phys_frame_to_chd(cdrom, lba);
							cdrom_stop_audio(cdrom);
						}

						logerror("53cf96: READ at LBA %x for %x blocks\n", lba, blocks);
						break;
					case 0x42:	// READ SUB-CHANNEL
//						logerror("53cf96: READ SUB-CHANNEL type %d\n", fifo[4]);
						break;
					case 0x43:	// READ TOC
						logerror("53cf96: READ TOC, starting track %d, length %x\n", fifo[7], fifo[8]<<8 | fifo[9]);
						break;
					case 0x48:	// PLAY AUDIO TRACK/INDEX
						// be careful: tracks here are zero-based, but the SCSI command
						// uses the real CD track number which is 1-based!
						lba = cdrom_get_chd_start_of_track(cdrom, fifo[5]-1);
						blocks = cdrom_get_chd_start_of_track(cdrom, fifo[8]-1) - lba;
						if (fifo[5] > fifo[8])
						{
							blocks = 0;
						}

						if (fifo[5] == fifo[8])
						{
							blocks = cdrom_get_chd_start_of_track(cdrom, fifo[5]) - lba;
						}

						if (blocks && cdrom)
						{
							cdrom_start_audio(cdrom, lba, blocks);
						}

						logerror("53cf96: PLAY AUDIO T/I: strk %d idx %d etrk %d idx %d frames %d\n", fifo[5], fifo[6], fifo[8], fifo[9], blocks);
						break;
					case 0x4b:	// PAUSE/RESUME
						if (cdrom)
						{
							cdrom_pause_audio(cdrom, (fifo[9] & 0x01) ^ 0x01);
						}

						logerror("53cf96: PAUSE/RESUME: %s\n", fifo[9]&1 ? "RESUME" : "PAUSE");
						break;
					default:
						logerror("53cf96: unknown SCSI command %x!\n", last_cmd);
						break;
				}
				xfer_state = 0;
				break;
			case 0x44:	// enable selection/reselection
				xfer_state = 0;
				break;
			case 0x10:	// information transfer (must not change xfer_state)
			case 0x11:	// second phase of information transfer
			case 0x12:	// message accepted
				timer_set( TIME_IN_HZ( 16384 ), 0, am53cf96_irq );
				scsi_regs[REG_INTSTATE] = 6;	// command sent OK
				break;
		}
	}

	// only update the register mirror if it's not a write-only reg
	if (reg != REG_STATUS && reg != REG_INTSTATE && reg != REG_IRQSTATE && reg != REG_FIFOSTATE)
	{
		scsi_regs[reg] = val;
	}
}

void am53cf96_init( struct AM53CF96interface *interface )
{
	const struct hard_disk_info *hdinfo;

	// save interface pointer for later
	intf = interface;

	memset(scsi_regs, 0, sizeof(scsi_regs));

	// try to open the disk
	if (interface->device == AM53CF96_DEVICE_HDD)
	{
		disk = hard_disk_open(get_disk_handle(0));
		if (!disk)
		{
			logerror("53cf96: no disk found!\n");
		}
		else
		{
			hdinfo = hard_disk_get_info(disk);
			if (hdinfo->sectorbytes != 512)
			{
				logerror("53cf96: Error!  invalid sector size %d\n", hdinfo->sectorbytes);
			}
		}
	}
	else if (interface->device == AM53CF96_DEVICE_CDROM)
	{
		cdrom = cdrom_open(get_disk_handle(0));
		if (!cdrom)
		{
			logerror("53cf96: no CD found!\n");
		}
	}
	else
	{
		logerror("53cf96: unknown device type!\n");
	}

	state_save_register_UINT8("53cf96", 0, "registers", scsi_regs, 32);
	state_save_register_UINT8("53cf96", 0, "fifo", fifo, 16);
	state_save_register_UINT8("53cf96", 0, "fifo pointer", &fptr, 1);
	state_save_register_UINT8("53cf96", 0, "last scsi-2 command", &last_cmd, 1);
	state_save_register_UINT8("53cf96", 0, "transfer state", &xfer_state, 1);
	state_save_register_int("53cf96", 0, "current lba", &lba);
	state_save_register_int("53cf96", 0, "blocks to read", &blocks);
	state_save_register_int("53cf96", 0, "read position", &last_lba);
}

// retrieve data from the SCSI controller
void am53cf96_read_data(int bytes, data8_t *pData)
{
	int i;
	UINT32 last_phys_frame;

	scsi_regs[REG_STATUS] |= 0x10;	// indicate DMA finished

	switch (last_cmd)
	{
		case 0x03:	// REQUEST SENSE
			pData[0] = 0x80;	// valid sense
			for (i = 1; i < 12; i++)
			{
				pData[i] = 0;
			}
			break;

		case 0x12:	// INQUIRY
			pData[8] = 'S';
			pData[9] = 'o';
			pData[10] = 'n';
			pData[11] = 'y';
			pData[12] = '\0';
			break;

		case 0x28:	// READ (10 byte)
			if ((disk) && (blocks))
			{
				while (bytes > 0)
				{
					if (!hard_disk_read(disk, lba, 1, pData))
					{
						logerror("53cf96: HD read error!\n");
					}
					lba++;
					last_lba = lba;
					blocks--;
					bytes -= 512;
					pData += 512;
				}
			}
			else if ((cdrom) && (blocks))
			{
				while (bytes > 0)
				{
					if (!cdrom_read_data(cdrom, lba, 1, pData, CD_TRACK_MODE1))
					{
						logerror("53cf96: CD read error!\n");
					}
					lba++;
					last_lba = lba;
					blocks--;
					bytes -= CD_FRAME_SIZE;
					pData += CD_FRAME_SIZE;
				}
			}
			break;

		case 0x42:	// READ SUB-CHANNEL
			switch (fifo[4])
			{
				case 1:	// return current position
					if (!cdrom)
					{
						return;
					}

					if (cdrom_audio_active(cdrom))
					{
						pData[1] = 0x11;		// audio in progress
					}
					else
					{
						if (cdrom_audio_ended(cdrom))
						{
							pData[1] = 0x13;	// ended successfully
						}
						else
						{
							pData[1] = 0x15;	// no audio status to report
						}
					}
					pData[2] = 0;
					pData[3] = 12;		// data length
					pData[4] = 0x01;	// sub-channel format code
					pData[5] = 0x10 | cdrom_audio_active(cdrom) ? 0 : 4;
					pData[6] = cdrom_get_track_phys(cdrom, last_lba);	// track
					pData[7] = 0;	// index

					// if audio is playing, get the latest LBA from the CDROM layer
					if (cdrom_audio_active(cdrom))
					{
						last_lba = cdrom_get_audio_lba(cdrom);
					}

					last_phys_frame = cdrom_chd_frame_to_phys(cdrom, last_lba);

					pData[8] = last_phys_frame>>24;
					pData[9] = (last_phys_frame>>16)&0xff;
					pData[10] = (last_phys_frame>>8)&0xff;
					pData[11] = last_phys_frame&0xff;

					last_phys_frame -= cdrom_get_phys_start_of_track(cdrom, pData[6]);

					pData[12] = last_phys_frame>>24;
					pData[13] = (last_phys_frame>>16)&0xff;
					pData[14] = (last_phys_frame>>8)&0xff;
					pData[15] = last_phys_frame&0xff;
					break;
				default:
					logerror("53cf96: Unknown subchannel type %d requested\n", fifo[4]);
			}
			break;

		case 0x43:	// READ TOC
			break;

		case 0x48:	// PLAY AUDIO TRACK/INDEX
			break;
	}
}

// write data to the SCSI controller
void am53cf96_write_data(int bytes, data8_t *pData)
{
//	int i;

	scsi_regs[REG_STATUS] |= 0x10;	// indicate DMA finished

	switch (last_cmd)
	{
		case 0x15:	// MODE SELECT
				// (sets volume on Sony CD-ROMs - bytes 21 and 23 are L & R volume)
			break;
	}
}

// get the device handle (HD or CD)
void *am53cf96_get_device(void)
{
	if (disk)
	{
		return disk;
	}

	if (cdrom)
	{
		return cdrom;
	}

	return NULL;
}
