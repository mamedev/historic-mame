/***************************************************************************

 scsicd.c - Implementation of a SCSI CD-ROM device, using MAME's cdrom.c primitives

***************************************************************************/

#include "driver.h" 
#include "scsidev.h"
#include "cdrom.h"

typedef struct
{
	data32_t lba, blocks, last_lba;
	int last_command;
 	struct cdrom_file *cdrom;
	data8_t last_packet[16];
} SCSICd;


// scsicd_exec_command
//
// Execute a SCSI command passed in via pCmdBuf.

int scsicd_exec_command(SCSICd *our_this, data8_t *pCmdBuf)
{
	struct cdrom_file *cdrom = our_this->cdrom;
	int retval = 12, trk;

	// remember the last command for the data transfer phase
	our_this->last_command = pCmdBuf[0];

	// remember the actual command packet too
	memcpy(our_this->last_packet, pCmdBuf, 16);

	switch (our_this->last_command)
	{
			case 0:		// TEST UNIT READY
			retval = 12;
			break;
		case 3: 	// REQUEST SENSE
			retval = 16;
			break;
		case 0x12:	// INQUIRY
			break;
		case 0x1a:	// MODE SENSE
			retval = 8;
			break;
		case 0x25:	// READ CD-ROM CAPACITY
			retval = 8;
			break;
		case 0x28: 	// READ (10 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			retval = our_this->blocks * 2048;

			/* convert physical frame to CHD */
			if (cdrom)
			{
				our_this->lba = cdrom_phys_frame_to_chd(cdrom, our_this->lba);
				cdrom_stop_audio(cdrom);
			}

			logerror("SCSICD: READ (10) at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);
			break;
		case 0x42:	// READ SUB-CHANNEL
	//						logerror("SCSICD: READ SUB-CHANNEL type %d\n", pCmdBuf[3]);
			retval = 16;
			break;
		case 0x43:	// READ TOC
			break;
		case 0x45:	// PLAY AUDIO  (10 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			logerror("SCSICD: PLAY AUDIO (10) at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);

			trk = cdrom_get_track_chd(cdrom, our_this->lba);

			if (cdrom_get_track_type(cdrom, trk))
			{
				logerror("SCSICD: track is audio\n");
			}
			else
			{
				logerror("SCSICD: track is NOT audio!\n");
				retval = -1;
			}
			break;
		case 0x48:	// PLAY AUDIO TRACK/INDEX
			// be careful: tracks here are zero-based, but the SCSI command
			// uses the real CD track number which is 1-based!
			our_this->lba = cdrom_get_chd_start_of_track(cdrom, pCmdBuf[4]-1);
			our_this->blocks = cdrom_get_chd_start_of_track(cdrom, pCmdBuf[7]-1) - our_this->lba;
			if (pCmdBuf[4] > pCmdBuf[7])
			{
				our_this->blocks = 0;
			}

			if (pCmdBuf[4] == pCmdBuf[7])
			{
				our_this->blocks = cdrom_get_chd_start_of_track(cdrom, pCmdBuf[4]) - our_this->lba;
			}

			if (our_this->blocks && cdrom)
			{
				cdrom_start_audio(cdrom, our_this->lba, our_this->blocks);
			}

			logerror("SCSICD: PLAY AUDIO T/I: strk %d idx %d etrk %d idx %d frames %d\n", pCmdBuf[4], pCmdBuf[5], pCmdBuf[7], pCmdBuf[8], our_this->blocks);
			break;
		case 0x4b:	// PAUSE/RESUME
			if (cdrom)
			{
				cdrom_pause_audio(cdrom, (pCmdBuf[8] & 0x01) ^ 0x01);
			}

			logerror("SCSICD: PAUSE/RESUME: %s\n", pCmdBuf[8]&1 ? "RESUME" : "PAUSE");
			break;
		case 0x55:	// MODE SELECT
			logerror("SCSICD: MODE SELECT length %x control %x\n", pCmdBuf[7]<<8 | pCmdBuf[8], pCmdBuf[1]);
			retval = 0x18;
			break;
		case 0x5a:	// MODE SENSE
			retval = 0x18;
			break;
		case 0xa8: 	// READ (12 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<16 | pCmdBuf[8]<<8 | pCmdBuf[9];

			retval = our_this->blocks * 2048;

			/* convert physical frame to CHD */
			if (cdrom)
			{
				our_this->lba = cdrom_phys_frame_to_chd(cdrom, our_this->lba);
				cdrom_stop_audio(cdrom);
			}

			logerror("SCSICD: READ (12) at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);
			break;
		default:
			logerror("SCSICD: unknown SCSI command %x!\n", our_this->last_command);
			break;
	}

	return retval;
}

// scsicd_read_data
//
// Read data from the device resulting from the execution of a command

void scsicd_read_data(SCSICd *our_this, int bytes, data8_t *pData)
{
	int i;
	UINT32 last_phys_frame;
	data8_t *fifo = our_this->last_packet;
	struct cdrom_file *cdrom = our_this->cdrom;
	data32_t temp;

	switch (our_this->last_command)
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

		case 0x25:	// READ CAPACITY
			logerror("SCSICD: READ CAPACITY\n");

			temp = cdrom_get_track_start(cdrom, 0xaa, 0);
			temp--;	// return the last used block on the disc

			pData[0] = (temp>>24) & 0xff;
			pData[1] = (temp>>16) & 0xff;
			pData[2] = (temp>>8) & 0xff;
			pData[3] = (temp & 0xff);
			pData[4] = 0;	// block size is always 2048 (0x0800)
			pData[5] = 0;
			pData[6] = 0x08;
			pData[7] = 0;
			break;

		case 0x28:	// READ (10 byte)
		case 0xa8:	// READ (12 byte)
			if ((our_this->cdrom) && (our_this->blocks))
			{
				while (bytes > 0)
				{
					if (!cdrom_read_data(our_this->cdrom, our_this->lba, 1, pData, CD_TRACK_MODE1))
					{
						logerror("SCSICD: CD read error!\n");
					}
					our_this->lba++;
					our_this->last_lba = our_this->lba;
					our_this->blocks--;
					bytes -= CD_FRAME_SIZE;
					pData += CD_FRAME_SIZE;
				}
			}
			break;

		case 0x42:	// READ SUB-CHANNEL
			switch (fifo[3])
			{
				case 1:	// return current position
					if (!cdrom)
					{
						return;
					}

					logerror("SCSICD: READ SUB-CHANNEL Time = %x, SUBQ = %x\n", fifo[1], fifo[2]);	

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
					pData[6] = cdrom_get_track_phys(cdrom, our_this->last_lba);	// track
					pData[7] = 0;	// index

					// if audio is playing, get the latest LBA from the CDROM layer
					if (cdrom_audio_active(cdrom))
					{
						our_this->last_lba = cdrom_get_audio_lba(cdrom);
					}

					last_phys_frame = cdrom_chd_frame_to_phys(cdrom, our_this->last_lba);

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
					logerror("SCSICD: Unknown subchannel type %d requested\n", fifo[3]);
			}
			break;

		case 0x43:	// READ TOC
			logerror("SCSICD: READ TOC, format = %d\n", fifo[2]&0xf);
			switch (fifo[2] & 0x0f)
			{
				case 0:		// normal
					{
						data8_t start_trk = fifo[6];
						int trks, len, in_len, dptr;
						data32_t tstart;

						in_len = fifo[7]<<8 | fifo[8];

						trks = cdrom_get_last_track(cdrom);
						len = (trks * 8) + 2;

						if (start_trk == 0xaa)	// special hack
						{
							len = 8 + 2;
						}

						if (len > in_len)
						{
							len = in_len;
						}

						pData[0] = (in_len>>8) & 0xff;
						pData[1] = (in_len & 0xff);
						pData[2] = start_trk;
						pData[3] = trks;

						dptr = (start_trk * 8) + 4;
						if (start_trk == 0xaa)
						{
							dptr = 4;
							trks = 1;
						}

						for (i = start_trk; i < start_trk + trks; i++)
						{
							pData[dptr++] = 0;
							pData[dptr++] = cdrom_get_adr_control(cdrom, i);
							pData[dptr++] = i;
							pData[dptr++] = 0;
							
							tstart = cdrom_get_track_start(cdrom, i, (fifo[1]&2)>>1);

							pData[dptr++] = (tstart>>24) & 0xff;
							pData[dptr++] = (tstart>>16) & 0xff;
							pData[dptr++] = (tstart>>8) & 0xff;
							pData[dptr++] = (tstart & 0xff);
						}
					}
					break;
				default:
					logerror("SCSICD: Unhandled READ TOC format %d\n", fifo[2]&0xf);
					break;
			}
			break;

		case 0x48:	// PLAY AUDIO TRACK/INDEX
			break;

		case 0x5a:	// MODE SENSE
			logerror("SCSICD: MODE SENSE page code = %x, PC = %x\n", fifo[2] & 0x3f, (fifo[2]&0xc0)>>6);

			switch (fifo[2] & 0x3f)
			{
				case 0xe:	// CD Audio control page
					pData[0] = 0x8e;	// page E, parameter is savable
					pData[1] = 0x0e;	// page length
					pData[2] = 0x04;	// IMMED = 1, SOTC = 0
					pData[3] = pData[4] = pData[5] = pData[6] = pData[7] = 0; // reserved

					// connect each audio channel to 1 output port	
					pData[8] = 1;
					pData[10] = 2;
					pData[12] = 4;
					pData[14] = 8;

					// indicate max volume
					pData[9] = pData[11] = pData[13] = pData[15] = 0xff;
					break;

				default:
					logerror("SCSICD: MODE SENSE unknown page %x\n", fifo[2] & 0x3f);
					break;
			}
			break;
	}
}

// scsicd_write_data
//
// Write data to the CD-ROM device as part of the execution of a command

void scsicd_write_data(SCSICd *our_this, int bytes, data8_t *pData)
{
	switch (our_this->last_command)
	{
		case 0x55:	// MODE SELECT
			logerror("SCSICD: MODE SELECT page %x\n", pData[0] & 0x3f);

			switch (pData[0] & 0x3f)
			{
				case 0xe:	// audio page
					logerror("Ch 0 route: %x vol: %x\n", pData[8], pData[9]);
					logerror("Ch 1 route: %x vol: %x\n", pData[10], pData[11]);
					logerror("Ch 2 route: %x vol: %x\n", pData[12], pData[13]);
					logerror("Ch 3 route: %x vol: %x\n", pData[14], pData[15]);
					break;
			}
			break;
	}
}

int scsicd_dispatch(int operation, void *file, INT64 intparm, data8_t *ptrparm)
{
	SCSICd *instance, **result;
	struct cdrom_file **devptr;

	switch (operation)
	{
		case SCSIOP_EXEC_COMMAND:
			return scsicd_exec_command((SCSICd *)file, ptrparm);
			break;

		case SCSIOP_READ_DATA:
			scsicd_read_data((SCSICd *)file, intparm, ptrparm);
			break;

		case SCSIOP_WRITE_DATA:
			scsicd_write_data((SCSICd *)file, intparm, ptrparm);
			break;

		case SCSIOP_ALLOC_INSTANCE:
			instance = (SCSICd *)malloc(sizeof(SCSICd));

			instance->lba = 0;
			instance->blocks = 0;
			instance->cdrom = cdrom_open(get_disk_handle(intparm));

			if (!instance->cdrom)
			{
				logerror("SCSICD: no CD found!\n");
			}


			result = (SCSICd **) file;
			*result = instance;
			break;

		case SCSIOP_DELETE_INSTANCE:
			break;

		case SCSIOP_GET_DEVICE:
			devptr = (struct cdrom_file **)ptrparm;
			instance = (SCSICd *)file;
			*devptr = instance->cdrom;
			break;
	}

	return 0;
}
