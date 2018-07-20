/*
 * scheduler.h
 *
 *  Created on: Jun 21, 2018
 *      Author: jsong
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_




#include <iostream>
#include <numeric>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <list>
#include <pthread.h>
#include <assert.h>

#include "sys/socket.h"
#include "sys/types.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "netinet/ip.h"
#include "netinet/tcp.h"
#include "pthread.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/time.h"
#include "time.h"
#include "sys/select.h"
#include "sys/ioctl.h"
#include "net/if.h"
#include "netinet/in.h"

#include "cuda_profiler_api.h"
#include "schedulerConst.h"



struct structIpAddress_t {
    unsigned char addr[4];              //## attribute addr
    char char_addr[16];         //## attribute char_addr
    unsigned short port;                //## attribute port
};

/*
typedef struct InputArbiter {

    bool  is_granted;		//## attribute is_granted

    int arbiter_pointer;		//## attribute arbiter_pointer

    int index;		//## attribute index

    int requested_arbiter_pointer;		//## attribute requested_arbiter_pointer

    int requested_priority;		//## attribute requested_priority

    bool is_requested[NUM_SWITCH_SIZE];		//## attribute is_requested

    int granted_port;		//## attribute granted_port

    int voq_state[NUM_SWITCH_SIZE];		//## attribute voq_state

    int highest_priority;		//## attribute highest_priority

} InputArbiter_t;
*/

/*
typedef struct OutputArbiter {

    int index;		//## attribute index

    bool is_granted;		//## attribute is_granted

    int arbiter_pointer;		//## attribute arbiter_pointer

    int granted_arbiter_pointer;		//## attribute granted_arbiter_pointer

    int received_highest_priority;		//## attribute received_highest_priority

    int received_req_priority[NUM_SWITCH_SIZE];		//## attribute received_req_priority

} OutputArbiter_t;

typedef struct VoQPerInput_t
{
	uint32_t voq_info [NUM_REQ_SIZE_UINT32]; // [input][output] 0:no packet, 1: packet exist
} VoQPerInput_t;


typedef struct VoQAllInput_t
{
	VoQPerInput_t input[NUM_SWITCH_SIZE];

} VoQAllInput_t;

typedef struct RRPointerAll_t
{
	short rr_pointer[NUM_SWITCH_SIZE];
}RRPointerAll_t;

typedef struct GrantMap_t
{
	short granted_input[NUM_SWITCH_SIZE];
	short granted_output[NUM_SWITCH_SIZE];
}GrantMap_t;

 // [input][output] 0: no request, 1: request exist
typedef struct ReqMap_t
{
	uint8_t req[NUM_SWITCH_SIZE][NUM_SWITCH_SIZE];
}ReqMap_t;

*/



typedef struct msgRequest_t
{
	uint16_t version_type  :4;
	uint16_t ts_id         :12;
	uint16_t back_pressure :4;
	uint16_t s_pfwi_id     :12;
	uint32_t voq_info [MAX_NUM_SWITCH_SIZE];  // SWITCH_SIZE must be multiple of 4
}msgRequest_t;

typedef struct msgSynch_t {
	uint16_t version_type  :4;
	uint16_t ts_id         :12;
} msgSynch_t;

typedef struct msgPacketRequest_t {
    int input_idx;		//## attribute input_idx
    int output_idx;		//## attribute output_idx
    int time_slot;		//## attribute time_slot
} msgPacketRequest_t;


/*typedef struct RRPointer_t {
    uint16_t pointer[NUM_SWITCH_SIZE][NUM_RR_SEQ_SIZE];
} RRPointer_t;
*/




union msgUnion_t {
	msgRequest_t msg_req;
	msgSynch_t msg_synch;
};

typedef std::list<msgRequest_t *> ReqMsgQueue_c;

typedef std::list<msgSynch_t *> SynchMsgQueue_c;

extern ReqMsgQueue_c ReqMsgQueue;
extern SynchMsgQueue_c SynchMsgQueue;





#endif /* SCHEDULER_H_ */
