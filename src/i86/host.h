/*****************************************************************************/
/* host dependent types                                                      */

#define BIGCASE

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
#ifdef linux_alpha
typedef signed int INT32;
typedef unsigned int UINT32;
#else
typedef signed long INT32;
typedef unsigned long UINT32;
#endif
typedef char BOOLEAN;

typedef UINT8 BYTE;
typedef UINT16 WORD;
typedef UINT32 DWORD;
