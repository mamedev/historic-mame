/*
 * sound server main code for X-Mame
 * by Juan Antonio Martinez <mame@drake.dit.upm.es>
 *
 */

#define __SNDSERVER_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <varargs.h>
#include <netdb.h>

#include "sndserver.h"

#ifndef TRUE
#define TRUE	(1)
#endif
#ifndef FALSE
#define FALSE	(0)
#endif

const char *idstring 	= "X-Mame sound server 1.0";
static int end_flag  	= 0;
extern int	errno;
#ifndef NETBSD
extern char	*sys_errlist[];
#endif

network_data * (*func[])(network_data *pt)= {
	nop_func,
	ack_func,
	nack_func,
	begin_func,
	end_func,
	start_func,
	stop_func,
	play_func,
	play_streamed_func,
	sync_func,
	set_vol_func,
	adjust_func
};

/* 
 * Network initialization code
 */

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
/*VARARGS1*/
int
errexit(format, va_alist)
char	*format;
va_dcl
{
	va_list	args;

	va_start(args);
	vfprintf(stderr,format, args);
	va_end(args);
	return(-1);
}

/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *------------------------------------------------------------------------
 */
int passivesock( service, protocol, qlen )
char	*service;	/* service associated with the desired port	*/
char	*protocol;	/* name of protocol to use ("tcp" or "udp")	*/
int	qlen;		/* maximum length of the server request queue	*/
{
	struct servent	*pse;	/* pointer to service information entry	*/
	struct protoent *ppe;	/* pointer to protocol information entry*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, type;	/* socket descriptor and socket type	*/

	memset((char *)&sin,0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

    /* Map service name to port number */
	if ( (pse=getservbyname(service, protocol) ) )
		sin.sin_port = htons(ntohs((u_short)pse->s_port));
	else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
		return errexit("can't get \"%s\" service entry\n", service);

    /* Map protocol name to protocol number */
	if ( (ppe = getprotobyname(protocol)) == 0)
		return errexit("can't get \"%s\" protocol entry\n", protocol);

    /* Use protocol to choose a socket type */
	if (strcmp(protocol, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

    /* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s < 0) return  errexit("can't create socket: %s\n", sys_errlist[errno]);

    /* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		return errexit("can't bind to %s port: %s\n", service, sys_errlist[errno]);
	if (type == SOCK_STREAM && listen(s, qlen) < 0)
		return errexit("can't listen on %s port: %s\n", service, sys_errlist[errno]);
	return s;
}

/* Network related function prototypes */
/* 
 * returns pt 		on success
 *         NULL		on fail
 *	   dummy_msg 	on success with reply
 */

/* 
 * NOP: just simply returns server identification string 
 */
network_data  *nop_func( network_data *pt){
	network_data *res;
	res=(network_data *)malloc(sizeof(network_data)+strlen(idstring)+1-4);
	if (! res) return (network_data *) NULL;
	memcpy(res,pt,sizeof(network_data));
	strcpy(res->data,idstring);
	res->length = htonl( strlen(idstring) );
	return dummy_msg;
}

/*
 * ACK: successfull confirm on last query
 */
network_data  *ack_func( network_data *pt){
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->netops = htonl( ACK );	
	dummy_msg->length = htonl( 4 );
	return dummy_msg;
}

/*
 * NACK: unsuccessfull notice on last query
 */
network_data  *nack_func( network_data *pt){
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->netops = htonl( NACK );	
	dummy_msg->length = htonl( 4 );
	return dummy_msg;
}

/*
 * BEGIN: initialize audio system
 * open audio resources and set device-specific parameters
 * returns ACK or NACK packet
 */
int sndserver_begin(network_data *pt) { return 0; }
int sndserver_end(network_data *pt) { return 0; }
int sndserver_start(network_data *pt) { return 0; }
int sndserver_stop(network_data *pt) { return 0; }
int sndserver_play(network_data *pt) { return 0; }
int sndserver_sync(network_data *pt) { return 0; }
int sndserver_set_vol(network_data *pt) { return 0; }
int sndserver_adjust(network_data *pt) { return 0; }

network_data  *begin_func( network_data *pt){
	if ( sndserver_begin(pt) ) return nack_func(pt);
	else 			   return ack_func(pt);;
}

/*
 * END: finish audio system
 * flushes all audio buffers and close audio devices
 *
 * Return ACK or NACK packet
 */

network_data  *end_func( network_data *pt){
	if ( sndserver_end(pt) ) return nack_func(pt);
	else 			   return ack_func(pt);;
}

/*
 * START: initialize audio buffers
 * 	  malloc audio_channels and inint audio structures
 * 
 * returns ACK or NACK
 */

network_data  *start_func( network_data *pt){
	if ( sndserver_start(pt) ) return nack_func(pt);
	else 			   return ack_func(pt);;
}

/*
 * STOP: flushes all audio buffers
 *       free audio channels and audio sample structures
 */

network_data  *stop_func( network_data *pt){
	if ( sndserver_stop(pt) ) return nack_func(pt);
	else 			  return ack_func(pt);;
}

/* 
 * get a sample data. try to optimize overload by just seeing if sample
 * is already available and stopping transfer
 *
 * returns ACK if already buffered to mark end of transmission
 *         NACK in case of trouble. received samples discarded
 *         PLAY to indicate expecting more data. offset indicates data requested
 */
network_data  *play_func( network_data *pt){
	int res = sndserver_play(pt);
	if (res<0) return nack_func(pt);	/* failed operation */
	if (res==0) return ack_func(pt);	/* just received    */
	/* else res marks next sample to receive. so indicate it    */
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->offset = htonl(res);
	dummy_msg->length = htonl( 4 );
	return dummy_msg;
}

/*
 * convenience function. ( in older versions mark play in non-buffered mode )
 * same funcionality than play
 *
 */

network_data  *play_streamed_func( network_data *pt){
	int res = sndserver_play(pt);
	if (res<0) return nack_func(pt);	/* failed operation */
	if (res==0) return ack_func(pt);	/* just received    */
	/* else res marks next sample to receive. so indicate it    */
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->offset = htonl(res);
	dummy_msg->length = htonl( 4 );
	return dummy_msg;
}

/* 
 * force to flush all buffers
 * Doesn't need return any data
 */

network_data  *sync_func( network_data *pt){
	sndserver_sync(pt);
	return (network_data *) NULL;
}

/*
 * Set master volume of audio mixer
 * Always returns NULL
 */

network_data  *set_vol_func( network_data *pt){
	sndserver_set_vol(pt);
	return (network_data *) NULL;
}

/* 
 * Set some specific parameters of a given channel
 * Returs Null
 */

network_data  *adjust_func( network_data *pt){
	sndserver_adjust(pt);
	return (network_data *) NULL;
}

/*
 * shows help message
 */

void sndserver_usage(void) {
	char msg[]="\n%s\n\
Usage: sndserver [ options ]\n\
Options:\n\
	-? | -h | --help 	Show this help message\n\
	-k | --kill		To send kill msg to an specific sndserver\n\
	-p | --port <port>	To specify port. default is %s\n\
";
	fprintf(stderr,msg,idstring,DEFAULT_PORT);

}
/*
 * caught a SIGINT signal to finish server and exit 
 */

void kaputt(int sig) {
	end_flag=TRUE;
}

/*
 * initialization tasks
 */

int init_server(void) {
	return 0;
}

/*
 * open a network connection in server mode
 * returns -1 if cannot open a socket
 */
int open_connection(char *port) {
	return passivesock(port,"udp",0);
}

int close_system(int s) {
	/* close audio system */
	/* close network connection */
	close(s);
	return 0;	
}

/*
 * main loop: waits until 
 * - Audio buffers emptied : evaluate new samples from mixer and play then
 *			     ( done by audio_timer procedure )
 * - Network data arrives  : process them
 */
int sndserver_loop(int sock ) {
	/* just simply wait messages from network connection  */
	int 			res;
	struct sockaddr_in 	fsin;
	char 			buffer[MAX_DATA_SIZE+sizeof(network_data)];
	static int		serial	=	0;
	int 			len	=	sizeof(fsin);
	network_data 		*pt	=	(network_data *) buffer;

	res=recvfrom(sock,pt,sizeof(network_data),0,(struct sockaddr *)&fsin,&len);
	if ( (res<0) && (errno != EINTR ) ) /* if alarm signal, just ignore */
		return errexit("RecvFrom: %s\n",sys_errlist[errno]);	
	if ( res<0 ) /* alarm timer */ return (0);
	/* evaluate length. if needed, read remainning data from network */
	if ( res != sizeof(network_data) ) { /* should never occurs */
		fprintf(stderr,"RecvFrom: Network Synchronization lost !!. Exitting..\n");
		end_flag=TRUE;
		return 1;
	}		
	if ( pt->length > 4 ) /* first 4 bytes comes with control packet */
		res = read(sock,pt+sizeof(network_data),pt->length-4);
		if ( res != pt->length -4 ){
		fprintf(stderr,"Read: Network Synchronization lost !!. Exitting..\n");
		end_flag=TRUE;
		return 1;
	}
	/*
	 *  now we have all data: check serial number and discard if older 
	 *  than last received one
         *  if everything ok, call involved network function
	 *  No matter if msg get repeated ( same serial ).just process twice ...
         */
	if (pt->msg_id < serial) return 0;
	serial=pt->msg_id;
	pt= (*func[pt->netops])(pt);
	/* check result and work according them */
	if (!pt) return 0; /* no answer needed */
	return sendto(sock,pt,sizeof(network_data)+(pt->length)-4,0,(struct sockaddr *)&fsin,len);
} 

void main(int argc, char *argv[] )
{
	int	i;
	int 	sock;
	char    *server_port = DEFAULT_PORT;
	int	kill_flag    = FALSE;
	
	/* parse command line */
	for(i=1; i<argc;i++) {
		if ( strcmp(argv[i],"-?") == 0 ) { sndserver_usage(); exit(0); }
		if ( strcmp(argv[i],"-h") == 0 ) { sndserver_usage(); exit(0); }
		if ( strcmp(argv[i],"--help") == 0 ) { sndserver_usage(); exit(0); }
		if ( strcmp(argv[i],"-k") == 0 ) { kill_flag = TRUE; continue; }
		if ( strcmp(argv[i],"--kill") == 0 ) { kill_flag = TRUE; continue; }
		if ( strcmp(argv[i],"-p") == 0 ) { server_port = argv[++i]; continue; }
		if ( strcmp(argv[i],"--port") == 0 ) { server_port= argv[++i]; continue; }
	}
	if ( ! strcmp(server_port,"") ){
		fprintf(stderr,"Illegal port value. Exitting...\n");
		exit(1);
	}
	printf("Sound Server is under development. Please be patient...\n");
	if (kill_flag == TRUE) {
		fprintf(stderr,"Sending STOP and END messages on port %s...\n",server_port);
		exit(0);
	}
	/* now make me a daemon .... not sure if really needed  */
	if ( init_server() ) {
		fprintf(stderr,"Failed to initialize server. Exitting...\n");
		exit(1);
	}
	if ( (sock=open_connection(server_port)) < 0) {
		fprintf(stderr,"Failed to open connection socket. Exitting...\n");
		exit(1);
	}
	end_flag = FALSE;	
	signal(SIGINT,kaputt);
	while (end_flag == FALSE ) sndserver_loop(sock);
        close_system(sock);	
	exit(0);
}
