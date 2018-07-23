#include "req_receiver.h"
#include "packets.h"

using namespace scheduler_packets;
// test t
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
		host_voqs_all[i] = (uint32_t*)malloc(_host_voqs_size);
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


#define TOTAL_VOQ_BYTES 180 
                       
#define NUM_OF_FRAME_TO_START_SCHED 90
#define TIME_OUT_TO_NEXT_FRAME_MS 1

				// 10:bf:48:80:66:c5
				
				if( buffer[6] == 0x10 && buffer[7] == 0xbf &&
					buffer[8] == 0x48 && buffer[9] == 0x80 &&
					buffer[10] == 0x66 && buffer[11] == 0xc5 ) {

					printf("receiving packet...\n");
					upstream_raw_packet packet;
					memset(&packet, 0, sizeof(upstream_raw_packet));

					parse_up_packet(buffer, &packet);


                                        uint16_t s_pfwi_id = 0;
                                        memcpy(&s_pfwi_id, packet.s_pfwi_id, 2);
                                        printf("s_pfwi_id: %d\n", s_pfwi_id);

/*
					uint8_t _pwia_id[2] = {0};
					//memcpy(&_pwia_id, buffer + OFFSET_S_PFWI_ID, 2);
					//_pwia_id = _pwia_id >> 4;

					memcpy(&_pwia_id[1], buffer + OFFSET_S_PFWI_ID, 1);
					memcpy(&_pwia_id[0], buffer + OFFSET_S_PFWI_ID+1, 1);
					_pwia_id[1] = _pwia_id[1] & 15;

					uint16_t tmp = 0;
					memcpy(&tmp, _pwia_id, 2);

					printf("--pwia_id: %d\n", tmp);					

					memcpy(host_voqs_all[tmp-1], &buffer[OFFSET_D_VOQ], TOTAL_VOQ_BYTES);

					//TODO: need to verify if all bits are correctly stored in host_voq_all

					FRAME_COUNT = FRAME_COUNT+1;

					//TODO: need to accumulate other packets on the same time slot 
					//	(up to how many? => 90) before starting scheduling

					if (FRAME_COUNT == NUM_OF_FRAME_TO_START_SCHED) {
						//TODO: need to start scheduling
					}
*/
				}
			}
		}
	}
}

int main() {
	receive_req(90);

	return 0;
}
