/***************************************************************************

 scsihd.c - Implementation of a SCSI hard disk drive

***************************************************************************/

#include "driver.h" 
#include "scsidev.h"
#include "harddisk.h"

typedef struct
{
	data32_t lba, blocks, last_lba;
	int last_command;
 	struct hard_disk_file *disk;
	data8_t last_packet[16];
} SCSIHd;


// scsihd_exec_command

int scsihd_exec_command(SCSIHd *our_this, data8_t *pCmdBuf)
{
	int retdata = 0;

	// remember the last command for the data transfer phase
	our_this->last_command = pCmdBuf[0] & 0x7f;

	// remember the actual command packet too
	memcpy(our_this->last_packet, pCmdBuf, 16);

	switch (our_this->last_command)
	{
		case 0:		// TEST UNIT READY
		case 3: 	// REQUEST SENSE
			break;
		case 0x12:	// INQUIRY
			break;
		case 0x15:	// MODE SELECT (used to set CDDA volume)
			logerror("SCSIHD: MODE SELECT length %x control %x\n", pCmdBuf[4], pCmdBuf[5]);
			break;
		case 0x1a:	// MODE SENSE
			break;
		case 0x28: 	// READ (10 byte)
			our_this->lba = pCmdBuf[2]<<24 | pCmdBuf[3]<<16 | pCmdBuf[4]<<8 | pCmdBuf[5];
			our_this->blocks = pCmdBuf[7]<<8 | pCmdBuf[8];

			logerror("SCSIHD: READ at LBA %x for %x blocks\n", our_this->lba, our_this->blocks);
			break;
		default:
			logerror("SCSIHD: unknown SCSI command %x!\n", our_this->last_command);
			break;
	}

	return retdata;
}

void scsihd_read_data(SCSIHd *our_this, int bytes, data8_t *pData)
{
	int i;

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

		case 0x28:	// READ (10 byte)
			if ((our_this->disk) && (our_this->blocks))
			{
				while (bytes > 0)
				{
					if (!hard_disk_read(our_this->disk, our_this->lba, 1, pData))
					{
						logerror("SCSIHD: HD read error!\n");
					}
					our_this->lba++;
					our_this->last_lba = our_this->lba;
					our_this->blocks--;
					bytes -= 512;
					pData += 512;
				}
			}
			break;

	}
}

void scsihd_write_data(SCSIHd *our_this, int bytes, data8_t *pData)
{
}

int scsihd_dispatch(int operation, void *file, INT64 intparm, data8_t *ptrparm)
{
	SCSIHd *instance, **result;
	struct hard_disk_file **devptr;

	switch (operation)
	{
		case SCSIOP_EXEC_COMMAND:
			return scsihd_exec_command((SCSIHd *)file, ptrparm);
			break;

		case SCSIOP_READ_DATA:
			scsihd_read_data((SCSIHd *)file, intparm, ptrparm);
			break;

		case SCSIOP_WRITE_DATA:
			scsihd_write_data((SCSIHd *)file, intparm, ptrparm);
			break;

		case SCSIOP_ALLOC_INSTANCE:
			instance = (SCSIHd *)malloc(sizeof(SCSIHd));

			instance->lba = 0;
			instance->blocks = 0;
			instance->disk = hard_disk_open(get_disk_handle(intparm));

			if (!instance->disk)
			{
				logerror("SCSIHD: no HD found!\n");
			}


			result = (SCSIHd **) file;
			*result = instance;
			break;

		case SCSIOP_DELETE_INSTANCE:
			break;

		case SCSIOP_GET_DEVICE:
			devptr = (struct hard_disk_file **)ptrparm;
			instance = (SCSIHd *)file;
			*devptr = instance->disk;
			break;
	}

	return 0;
}
