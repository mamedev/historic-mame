#include <stdio.h>
#include <stdlib.h>

#define __SNDSERVER_C_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "sndserver.h"

const char *idstring="X-Mame sound server 1.0";

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
	memcpy(dummy_msg,pt,sizeof(network_data));
	strcpy(dummy_msg->data,idstring);
	dummy_msg->length = htonl( strlen(idstring) );
	return dummy_msg;
}

network_data  *ack_func( network_data *pt){
	return pt;
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->netops = htonl( ACK );	
	return dummy_msg;
}

network_data  *nack_func( network_data *pt){
	return pt;
	memcpy(dummy_msg,pt,sizeof(network_data));
	dummy_msg->netops = htonl( NACK );	
	return dummy_msg;
}

network_data  *begin_func( network_data *pt){
	return pt;
}

network_data  *end_func( network_data *pt){
	return pt;
}

network_data  *start_func( network_data *pt){
	return pt;
}

network_data  *stop_func( network_data *pt){
	return pt;
}

network_data  *play_func( network_data *pt){
	return pt;
}

network_data  *play_streamed_func( network_data *pt){
	return pt;
}

network_data  *sync_func( network_data *pt){
	return pt;
}

network_data  *set_vol_func( network_data *pt){
	return pt;
}

network_data  *adjust_func( network_data *pt){
	return pt;
}

	
void main(int argc, char *argv )
{
	printf("Sound Server is under development. Please be patient...\n");
	exit(0);
}
