/*
 * standalone MIPS disassembler by smf
 *
 * based on DIS68k by Aaron Giles
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osd_cpu.h"
#include "psx.h"
extern unsigned dasmr3k(char *,unsigned);
extern unsigned dasmmips3(char *,unsigned);

struct
{
	UINT8 id[ 8 ];
	UINT32 text;	/* SCE only */
	UINT32 data;	/* SCE only */
	UINT32 pc0;
	UINT32 gp0;		/* SCE only */
	UINT32 t_addr;
	UINT32 t_size;
	UINT32 d_addr;	/* SCE only */
	UINT32 d_size;	/* SCE only */
	UINT32 b_addr;	/* SCE only */
	UINT32 b_size;	/* SCE only */
	UINT32 s_addr;
	UINT32 s_size;
	UINT32 SavedSP;
	UINT32 SavedFP;
	UINT32 SavedGP;
	UINT32 SavedRA;
	UINT32 SavedS0;
	UINT8 dummy[ 0x800 - 76 ];
} m_psxexe_header;

#define FORMAT_BIN ( 0 )
#define FORMAT_PSX ( 1 )

#define CPU_PSX ( 0 )
#define CPU_R3000 ( 1 )
#define CPU_R4000 ( 2 )

static UINT8 *filebuf;
static UINT32 offset;
static UINT8 order[] = { 3, 2, 1, 0 };

offs_t			opcode_memory_min;			/* opcode memory minimum */
offs_t			opcode_memory_max;			/* opcode memory maximum */
offs_t			opcode_mask;				/* mask to apply to the opcode address */
UINT8 *			opcode_base;				/* opcode ROM base */

void memory_set_opbase(offs_t pc)
{
}

static char *Options[]=
{
	"begin", "end", "offset", "order", "format", "cpu", 0
};

static void usage (void)
{
	fprintf( stderr,
		"Usage: DISMIPS [options] <filename>\n\n"
		"Available options are:\n"
		" -begin  - Specify begin offset in file to disassemble in bytes [0]\n"
		" -end    - Specify end offset in file to disassemble in bytes [none]\n"
		" -offset - Specify address to load program in bytes [0]\n"
		" -order  - Specify byte order [3210]\n"
		" -format - Specify file format bin|psx [bin]\n"
		" -cpu    - Specify cpu psx|r3000|r4000 [psx]\n\n"
		"All values should be entered in hexadecimal\n" );
	exit( 1 );
}

int main( int argc, char *argv[] )
{
	FILE *f;
	UINT8 i;
	UINT8 j;
	UINT8 n;
	UINT8 p;
	UINT32 begin;
	UINT32 end;
	UINT32 filelen;
	UINT32 len;
	UINT32 pc;
	char buf[ 80 ];
	char *filename;
	UINT32 format;
	UINT32 cpu;

	filename = NULL;
	begin = 0;
	end = 0xffffffff;
	format = FORMAT_BIN;
	cpu = CPU_PSX;

	n = 0;
	for( i = 1; i < argc; i++ )
	{
		if( argv[ i ][ 0 ] != '-' )
		{
			switch( n )
			{
			case 0:
				filename = argv[ i ];
				break;
			default:
				usage();
				break;
			}
			n++;
		}
		else
		{
			for( j = 0; Options[ j ]; j++ )
			{
				if( strcmp( argv[ i ] + 1, Options[ j ] ) == 0 )
				{
					break;
				}
			}
			switch( j )
			{
			case 0:
				i++;
				if( i > argc )
				{
					usage();
				}
				begin = strtoul( argv[ i ], 0, 16 );
				break;
			case 1:
				i++;
				if( i > argc )
				{
					usage();
				}
				end = strtoul( argv[ i ], 0, 16 );
				break;
			case 2:
				i++;
				if( i > argc )
				{
					usage();
				}
				offset = strtoul( argv[ i ], 0, 16 );
				break;
			case 3:
				i++;
				if( i > argc )
				{
					usage();
				}
				if( strlen( argv[ i ] ) != 4 )
				{
					usage();
				}
				for( p = 0; p < 4; p++ )
				{
					if( argv[ i ][ p ] < '0' || argv[ i ][ p ] > '3' )
					{
						usage();
					}
					order[ p ] = argv[ i ][ p ] - '0';
				}
				break;
			case 4:
				i++;
				if( i > argc )
				{
					usage();
				}
				if( stricmp( argv[ i ], "bin" ) == 0 )
				{
					format = FORMAT_BIN;
				}
				else if( stricmp( argv[ i ], "psx" ) == 0 )
				{
					format = FORMAT_PSX;
				}
				else
				{
					usage();
				}
				break;
			case 5:
				i++;
				if( i > argc )
				{
					usage();
				}
				if( stricmp( argv[ i ], "psx" ) == 0 )
				{
					cpu = CPU_PSX;
				}
				else if( stricmp( argv[ i ], "r3000" ) == 0 )
				{
					cpu = CPU_R3000;
				}
				else if( stricmp( argv[ i ], "r4000" ) == 0 )
				{
					cpu = CPU_R4000;
				}
				else
				{
					usage();
				}
				break;
			default:
				usage();
				break;
			}
		}
	}

	if (!filename)
	{
		usage();
		return 1;
	}
	f=fopen (filename,"rb");
	if (!f)
	{
		printf ("Unable to open %s\n",filename);
		return 2;
	}
	fseek (f,0,SEEK_END);
	filelen=ftell (f);

	if( format == FORMAT_PSX )
	{
		fseek( f, 0, SEEK_SET );
		if( fread( &m_psxexe_header, 1, sizeof( m_psxexe_header ), f ) != sizeof( m_psxexe_header ) )
		{
			fprintf( stderr, "error reading ps-x exe header\n" );
			fclose( f );
			return 3;
		}
		if( memcmp( m_psxexe_header.id, "PS-X EXE", sizeof( m_psxexe_header.id ) ) != 0 )
		{
			fprintf( stderr, "invalid ps-x exe header\n" );
			fclose( f );
			return 3;
		}
		printf( "_start = $%08x\n\n", m_psxexe_header.pc0 );
		if( offset == 0 )
		{
			offset = m_psxexe_header.t_addr;
		}
		if( begin == 0 )
		{
			begin = sizeof( m_psxexe_header );
		}
		if( end == 0xffffffff )
		{
			end = sizeof( m_psxexe_header ) + m_psxexe_header.t_size;
		}
	}

	fseek (f,begin,SEEK_SET);
	len=(filelen>end)? (end-begin+1):(filelen-begin);
	filebuf=malloc(len+16);
	if (!filebuf)
	{
		printf ("Memory allocation error\n");
		fclose (f);
		return 3;
	}
	memset (filebuf,0,len+16);
	if (fread(filebuf,1,len,f)!=len)
	{
		printf ("Read error\n");
		fclose (f);
		free (filebuf);
		return 4;
	}
	fclose (f);

	pc = 0;
	while( pc < len )
	{
		UINT32 op;
		op = ( filebuf[ pc + order[ 0 ] ] << 24 ) |
			( filebuf[ pc + order[ 1 ] ] << 16 ) |
			( filebuf[ pc + order[ 2 ] ] << 8 ) |
			( filebuf[ pc + order[ 3 ] ] );
		*( (UINT32 *)&filebuf[ pc ] ) = op;
		pc += 4;
	}

	opcode_mask = 0xffffffff;
	opcode_base = filebuf - offset;
	opcode_memory_min = 0;
	opcode_memory_max = 0xffffffff;

	pc = 0;
	while( pc < len )
	{
		switch( cpu )
		{
		case CPU_PSX:
			i = DasmMIPS( buf, pc + offset );
			break;
		case CPU_R3000:
			i = dasmr3k( buf, pc + offset );
			break;
		case CPU_R4000:
			i = dasmmips3( buf, pc + offset );
			break;
		}

		printf( "%08x: ", pc + offset );
		for( j = 0; j < i; j++ )
		{
			printf( "%02x ", filebuf[ BYTE4_XOR_BE( pc ) ] );
			pc++;
		}
		while( j < 10 )
		{
			printf( "   " );
			j++;
		}
		printf( "%s\n", buf );
	}
	free (filebuf);
	return 0;
}
