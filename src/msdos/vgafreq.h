/* Tweaked modes definitions - V.V
** You should be able to find perfect settings
** for your video card by making subtle changes
** to the provided values */

/*#define S3*/
#define SYNC


#if defined S3
/* Tested by V.V on a Diamond Stealth 3240 - S3 Vision 968 */

/* 224x288 noscanlines - 59.99 Hz at vgafreq 1 */
#define SY_FREQ224NH 0x5f
#define SY_FREQ224NV 0x53
/* 256x256 noscanlines - 59.98 Hz at vgafreq 1 */
#define SY_FREQ256NH 0x61
#define SY_FREQ256NV 0x47
/* 288x224 noscanlines - 60.00 Hz at vgafreq 0 */
#define SY_FREQ288NH 0x5f
#define SY_FREQ288NV 0x08

/* 224x288 scanlines - vgafreq 2 not working here */
#define SY_FREQ224SH 0x5f
#define SY_FREQ224SV 0x43
/* 256x256 scanlines - 120.17 Hz at vgafreq 1 */
#define SY_FREQ256SH 0x61
#define SY_FREQ256SV 0x22
/* 288x224 scanlines - 119.99 Hz at vgafreq 0 */
#define SY_FREQ288SH 0x5f
#define SY_FREQ288SV 0x03


#elif defined SYNC
/* Tested by V.V on a Diamond Stealth 3240 - S3 Vision 968
** Original synced modes, sync is approx. accurate */

/* 224x288 noscanlines - 59.75 Hz at vgafreq 1 */
#define SY_FREQ224NH 0x5f
#define SY_FREQ224NV 0x55
/* 256x256 noscanlines - 59.91 Hz at vgafreq 1 */
#define SY_FREQ256NH 0x62
#define SY_FREQ256NV 0x42
/* 288x224 noscanlines - 59.67 Hz at vgafreq 0 */
#define SY_FREQ288NH 0x5f
#define SY_FREQ288NV 0x08

/* 224x288 scanlines - vgafreq 2 not working here */
#define SY_FREQ224SH 0x5f
#define SY_FREQ224SV 0x43
/* 256x256 scanlines - 122.18 Hz at vgafreq 1 */
#define SY_FREQ256SH 0x5f
#define SY_FREQ256SV 0x23
/* 288x224 scanlines - 116.44 Hz at vgafreq 0 */
#define SY_FREQ288SH 0x5f
#define SY_FREQ288SV 0x0b

#endif


/* Tested by V.V on a Diamond Stealth 3240 - S3 Vision 968
** Original modes, sync is NOT accurate */

/* 224x288 noscanlines - 59.75 Hz at vgafreq 1 */
#define OR_FREQ224NH 0x5f
#define OR_FREQ224NV 0x55
/* 256x256 noscanlines - 57.05 Hz at vgafreq 0 */
#define OR_FREQ256NH 0x5f
#define OR_FREQ256NV 0x23
/* 288x224 noscanlines - 59.67 Hz at vgafreq 0 */
#define OR_FREQ288NH 0x5f
#define OR_FREQ288NV 0x0b

/* 224x288 scanlines - vgafreq 2 not working here */
#define OR_FREQ224SH 0x5f
#define OR_FREQ224SV 0x43
/* 256x256 scanlines - 122.18 Hz at vgafreq 1 */
#define OR_FREQ256SH 0x5f
#define OR_FREQ256SV 0x23
/* 288x224 scanlines - 116.44 Hz at vgafreq 0 */
#define OR_FREQ288SH 0x5f
#define OR_FREQ288SV 0x0b
