#include "req_receiver.h"

void receive_req(int _switch_size) {
        int _num_bits_per_voq = 4;
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


	uint32_t *host_voqs_all[90];

	for (int i=0; i<90; i++) {
		host_voqs_all[i] = (uint32_t)malloc(_host_voqs_size);
	}
/*
	// when wanting to bind a specific NIC I/F to the socket

	const char *opt = "eth0";
	const int len = strnlen(opt, IFNAMSIZ);
	if (len == 16) {
		printf("Too long iface name => socket creation error.");
		return;
	}
	setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, opt, len);
*/
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

                                printf("receiving packet...\n");

#define TOTAL_VOQ_BYTES 180 
                       
#define OFFSET_TS_ID 18
#define OFFSET_D_VOQ 22

#define NUM_OF_FRAME_TO_START_SCHED 90
#define TIME_OUT_TO_NEXT_FRAME_MS 1

				uint16_t _time_slot = 0;
				memcpy(&_time_slot, buffer + OFFSET_TS_ID, 2);
				_time_slot = _time_slot << 4;
				
				memcpy(host_voqs_all[FRAME_COUNT], &buffer[OFFSET_D_VOQ], TOTAL_VOQ_BYTES);
	
				//TODO: need to verify if all bits are correctly stored in host_voq_all
			
				FRAME_COUNT = FRAME_COUNT+1;

                                //TODO: need to accumulate other packets on the same time slot 
				//	(up to how many? => 90) before starting scheduling

				if (FRAME_COUNT == NUM_OF_FRAME_TO_START_SCHED) {
					//TODO: need to start scheduling
				}
			}
		}
	}
}
