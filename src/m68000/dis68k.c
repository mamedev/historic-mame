#include "m68kdasm.c"

static char *Options[]=
{
 "begin","end","offset",NULL
};

static void usage (void)
{
 printf ("Usage: DIS68K [options] <filename>\n"
         "Available options are:\n"
         " -begin  - Specify begin offset in file to disassemble [0]\n"
         " -end    - Specify end offset in file to disassemble [none]\n"
         " -offset - Specify address to load program [0]\n"
         "All values should be entered in hexadecimal\n");
 exit (1);
}

int main (int argc,char *argv[])
{
 int i,j,n;
 char *filename=NULL,buf[40];
 unsigned char *filebuf;
 FILE *f;
 unsigned begin=0,end=(unsigned)-1,offset=0,filelen,len,pc;
 printf ("DIS68K\n"
         "Copyright (C) Aaron Giles 1997\n");
 for (i=1,n=0;i<argc;++i)
 {
  if (argv[i][0]!='-')
  {
   switch (++n)
   {
    case 1:  filename=argv[i];
             break;
    default: usage();
   }
  }
  else
  {
   for (j=0;Options[j];++j)
    if (!strcmp(argv[i]+1,Options[j])) break;
   switch (j)
   {
    case 0:  ++i; if (i>argc) usage();
             begin=strtoul(argv[i],NULL,16);
             break;
    case 1:  ++i; if (i>argc) usage();
             end=strtoul(argv[i],NULL,16);
             break;
    case 2:  ++i; if (i>argc) usage();
             offset=strtoul(argv[i],NULL,16);
             break;
    default: usage();
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
 pc=0;
 while (pc<len-1)
 {
  i=Dasm68000 (filebuf+pc,buf,pc+offset);
  for (j=strlen(buf);j<39;++j) buf[j]=' ';
  buf[39]='\0';
  printf ("    %s    ; %06X ",buf,pc+offset);
  for (j=0;j<i;++j) printf("%02X ",filebuf[pc+j]);
  printf ("\n");
  pc+=i;
 }
 free (filebuf);
 return 0;
}
