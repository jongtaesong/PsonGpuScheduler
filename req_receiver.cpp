#include "req_receiver.h"
#include "packets.h"
#include "scheduler.h"

using namespace scheduler_packets;
extern int pefrom_scheduling (int ts);
extern void send_grant_frame(int ts, int sw_size, uint32_t * granted_ouput);
extern uint32_t * host_granted_output;
extern int _switch_size;


// test
void receive_req(int _switch_size) {
        int _num_bits_per_voq = 8;
        int _num_req_per_uint32 = 32/_num_bits_per_voq;
        int _block_size = _switch_size/_num_req_per_uint32;
        int _host_voqs_size = sizeof(uint32_t) * _switch_size * _block_size;

        int 	req_ts = 0;
        int     packet_count = 0;

        int     s;
        int     length = 0;
        struct  sockaddr_ll dest;
        uint8_t buffer[BUFF_SIZE] = {0};

        memset(buffer, 0, BUFF_SIZE);
        struct  ether_header *eh = (struct ether_header *) buffer;


	uint32_t *host_voqs_all[360];

	for (int i=0; i<90; i++) {
		host_voqs_all[i] = (uint32_t*)malloc(_host_voqs_size);
	}


	// when wanting to bind a specific NIC I/F to the socket

	const char *opt = "enp8s0";
	const int len = strnlen(opt, IFNAMSIZ);
	if (len == 16) {
		printf("Too long iface name => socket creation error.");
		return;
	}
	setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, opt, len);

        s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

        if( s == -1 ) {
        	printf("socket creation error.\n");
            close(s);
        }
        else {
        	printf("socket is successfully created.\n");


        	int FRAME_COUNT = 0;

			for(;;) {
				length = recvfrom(s, buffer, BUFF_SIZE, 0, NULL, NULL);

				if (length == -1) {
					//errorhandling ....
					printf("error while handling socket connection.\n");
				}
				else {
					//TODO: need to filter hosts by MAC address


#define TOTAL_VOQ_BYTES 180 
                       
#define NUM_OF_FRAME_TO_START_SCHED 90
#define TIME_OUT_TO_NEXT_FRAME_MS 1

#define LENGTH_OFFSET 	12
#define TS_OFFSET 	14
#define PWIA_ID_OFFSET 	16
#define VOQ_OFFSET	18
			
					// TEST HOST SERVER / 10:bf:48:80:66:c5
					if( buffer[6] == 0x10 && buffer[7] == 0xbf &&
						buffer[8] == 0x48 && buffer[9] == 0x80 &&
						buffer[10] == 0x66 && buffer[11] == 0xc5 ) {

						printf("receiving packet...\n");
						//////////////////////////////////////////////
						//
						//   |ver/type|        TS_ID       |
						//   |  4bit  |        12 bit      |
						//   [      8bit    ] --> buffer[TS_OFFSET]
						//    - version/type == 0 --> synch
						//    - version/type == 1 or 2 --> 90x90 or 360x360
						//
						/////////////////////////////////////////////////
						if (buffer[TS_OFFSET] > 0xf) // Request message

						{
							// Copy the VoQ info to the host voq info
							msgRequest_t  req_msg;
							memcpy (&req_msg, &buffer[TS_OFFSET], 4+_switch_size);
							memcpy (host_voqs_all[req_msg.s_pfwi_id],&req_msg.voq_info, _switch_size);
						}

						else // Synch message
						{
							msgSynch_t  synch_msg;
							memcpy (&synch_msg, &buffer[TS_OFFSET], sizeof(msgSynch_t));
							pefrom_scheduling ( synch_msg.ts_id+1); // Scheduling 1 TS ahead
							send_grant_frame(synch_msg.ts_id+1, _switch_size, host_granted_output);
						}
/*
						int NUM_OF_VOQS = -1;

						// LENGTH == 94
						if( buffer[LENGTH_OFFSET] == 0x00 &&
							buffer[LENGTH_OFFSET+1] == 0x5E ) {
							printf("NUMBER OF VALID VOQ FIELD: 90\n");
							NUM_OF_VOQS = 90;
						}
						// LENGTH == 364
						else if( buffer[LENGTH_OFFSET] == 0x01 &&
							 buffer[LENGTH_OFFSET+1] == 0x30) {
							printf("NUMBER OF VALID VOQ FIELD: 360\n");
							NUM_OF_VOQS = 360;
						}

						// UNKNOWN PACKET HANDLING
						if( NUM_OF_VOQS == -1 ) {
							continue;
						}

						uint16_t pwia_id = 0;
						memcpy(&pwia_id+1, buffer + PWIA_ID_OFFSET  , 1);
						memcpy(&pwia_id  , buffer + PWIA_ID_OFFSET+1, 1);

						printf("pwia_id: %d\n", pwia_id);
						memcpy(host_voqs_all[pwia_id], buffer + VOQ_OFFSET, NUM_OF_VOQS);
						printf("-- all voq information is copied into memory space\n");

						FRAME_COUNT = FRAME_COUNT+1;


						if( FRAME_COUNT == NUM_OF_FRAME_TO_START_SCHED ) {
							uint16_t time_slot;
							memcpy(&time_slot+1, buffer + PWIA_ID_OFFSET-2, 1);
							memcpy(&time_slot, buffer + PWIA_ID_OFFSET-1, 1);
							time_slot = time_slot&0xfff;
							pefrom_scheduling ( time_slot);
						}
*/
				}
			}
		}
	}
}
/*
int main() {
	receive_req(90);

	return 0;
}
*/
