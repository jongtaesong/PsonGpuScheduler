/*
 * send_grant.cpp
 *
 *  Created on: Aug 6, 2018
 *      Author: jsong
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
//#include <linux/if_arp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <string.h>
#include <errno.h>
#include "scheduler.h"


typedef struct grant_raw_packet {
    uint8_t sfd;
    uint8_t da[6];
    uint8_t sa[6];
    uint16_t tp_id;
    uint8_t user_pri;
    uint16_t vid;
    uint16_t length;
    uint16_t version_type  :4;
    uint16_t ts_id         :12;
    uint32_t granted_port[360];
} grant_raw_packet;



#define OFFSET_VID OFFSET_DA + 14
#define OFFSET_LENGTH OFFSET_DA + 16
#define OFFSET_VERSION_TYPE OFFSET_DA + 18
#define OFFSET_TS_ID OFFSET_DA + 18
#define OFFSET_BP OFFSET_DA + 20
#define OFFSET_S_PFWI_ID OFFSET_DA + 20
#define OFFSET_D_VOQ OFFSET_DA + 22

int sock = 0;
grant_raw_packet grant_frame;
struct sockaddr_ll dest;


#define GRANT_BUFFER_SIZE 2462

void send_grant_frame(int ts_id, int sw_size, uint32_t * granted_output) {


	struct sockaddr_ll dest;
	int if_index = if_nametoindex("enp2s0");
	struct ether_header eh;
	char buffer[GRANT_BUFFER_SIZE] = {0};

	if (sock == 0)
	{
		sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

		//may return -1 when executing user is not super-user
		if( sock == -1 ) {
			printf("socket creation error\n");
			close(sock);
			return;
		}
		printf("socket creation ok\n");
		dest.sll_family = AF_PACKET;

		dest.sll_protocol = htons(ETH_P_ALL);
		dest.sll_ifindex = if_index;

		//10-BF-48-7D-0D-04 // my desktop
		//10:bf:48:80:66:c5 // my ubuntu
		//1C:1B:0D:94:B1:9D // GPU server

		// broadcast frame
		memset (&dest.sll_addr[0], 0xff, ETH_ALEN);
		//dest.sll_addr[0] = 0xFF;
		//dest.sll_addr[1] = 0xFF;
		//dest.sll_addr[2] = 0xFF;
		//dest.sll_addr[3] = 0xFF;
		//dest.sll_addr[4] = 0xFF;
		//dest.sll_addr[5] = 0xFF;
		dest.sll_halen = ETH_ALEN; // 6

		memset(&grant_frame, 0, sizeof(grant_raw_packet));

		// broadcast frame
		memset (&dest.sll_addr[0], 0xff, ETH_ALEN);
		//grant_frame.da[0] = 0xFF;
		//grant_frame.da[1] = 0xFF;
		//grant_frame.da[2] = 0xFF;
		//grant_frame.da[3] = 0xFF;
		//grant_frame.da[4] = 0xFF;
		//grant_frame.da[5] = 0xFF;

		grant_frame.sa[0] = 0x1C;
		grant_frame.sa[1] = 0x1B;
		grant_frame.sa[2] = 0x0D;
		grant_frame.sa[3] = 0x94;
		grant_frame.sa[4] = 0xB1;
		grant_frame.sa[5] = 0x9D;

		uint16_t tp_id = 0x8100;
		grant_frame.tp_id = htons(tp_id);

		grant_frame.user_pri = 7;

		grant_frame.vid = htons(0);
		grant_frame.length = htonl(0x005e); // TODO check the length of grant
		grant_frame.version_type = 0;

	}


	grant_frame.ts_id = htons(ts_id);

	memcpy(&grant_frame.granted_port[0], granted_output, sizeof(uint32_t)*sw_size);
	int length = sendto(sock, &grant_frame, sizeof(grant_raw_packet), 0, (struct sockaddr*)&dest, sizeof(dest));
	if( length == -1) {
		printf("Error: %s (%d)\n", strerror(errno), errno);
	}

	return;
}
