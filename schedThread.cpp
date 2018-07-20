/*
 * schedThread.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: jsong
 */



#include "scheduler.h"
extern void copyMsgVoQToDevice (msgRequest_t * req);
extern int pefrom_scheduling (int ts);

ReqMsgQueue_c ReqMsgQueue;
SynchMsgQueue_c SynchMsgQueue;


//## operation MsgProcUE(void *)
void * ReqMsgProc (void * arg) {
    //#[ operation MsgProcUE(void *)

    void * p_UE;


    std::list<msgRequest_t *>::iterator it;

    for (;;)
    {
        if (ReqMsgQueue.size() > 0 )
        {
                // Get the first Message From Queue
        	    msgRequest_t * msg;
                it = ReqMsgQueue.begin();
                msg =  (* it);

                copyMsgVoQToDevice (msg);

                ReqMsgQueue.erase(it);

                delete msg;

                //usleep (50);
        }
    }
    printf("Thread Broken [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);

}


//## operation MsgProcUE(void *)
void * SynchMsgProc (void * arg) {
    //#[ operation MsgProcUE(void *)

    void * p_UE;


    std::list<msgSynch_t*>::iterator it;

    for (;;)
    {
        if (SynchMsgQueue.size() > 0 )
        {
                // Get the first Message From Queue
        	    msgSynch_t * msg;
                it = SynchMsgQueue.begin();
                msg =  (* it);
                pefrom_scheduling(msg->ts_id);

                SynchMsgQueue.erase(it);

                delete msg;

                //usleep (50);
        }
    }
    printf("Thread Broken [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);

}






/*

//## operation init_softnet_socket_thread_cp(void *)
void * init_scheduler_socket_thread_cp(void * socket_ip_address) {
    //#[ operation init_softnet_socket_thread_cp(void *)
    ////////////////////////////////////////////////////////
    ///////////////// Copy from Internet code - Start




        int recvSockfd = -1;
        int len;
        const int on = 1;
        struct sockaddr_in servaddr, cliaddr;
        int count;
        char recvbuf[2000];
        //char *sockptr;
        int nready = 0;
        fd_set readFds;
        struct timeval timeOut;

        printf("Function Start [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);

        memset (&servaddr, sizeof(servaddr), 0);

        // IP Socket Address Initializing
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //inaddr_any;
        //servaddr.sin_port = htons (flow_conf.ubs_port); // TBD
        structIpAddress_t * ip_address = (structIpAddress_t*) socket_ip_address;
        std::cout << " Input PORT = "<< ip_address->port << std::endl;
        servaddr.sin_port = htons (ip_address->port); // TBD

        printf(" UDP PORT = %d [%s, %d, %s]\n",
                servaddr.sin_port ,  __FILE__, __LINE__, __FUNCTION__);


        // socket creation
        recvSockfd = socket(AF_INET, SOCK_DGRAM, 0);


        printf("recvSockfd = %d\n", recvSockfd);

        if(recvSockfd < 0)
        {
        	printf("[Exception] Socket creation fail...[%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
		   printf("PackProcess thread creation fail...[%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
		   return 0;
	}

	// socket bind
	if(bind(recvSockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		   printf("[Exception] Socket bind fail...[%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
		   printf("PackProcess thread creation fail...[%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
		   return 0;
	}

	setsockopt(recvSockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	len = sizeof(cliaddr);

	// init_ue_message_thread_cp(p_UE);

	for (;;)
	{
		memset((void *)recvbuf, 0x00, MAX_PACKET_BUFF_SIZE);
		FD_ZERO(&readFds);
		FD_SET(recvSockfd, &readFds);
		timeOut.tv_sec = 10;
		timeOut.tv_usec = 0;
		nready= select(recvSockfd+1, &readFds, (fd_set*) NULL, (fd_set*)NULL, &timeOut);

		if(nready <= 0)
		{
			 continue;
		}
		if(FD_ISSET(recvSockfd, &readFds))
		{
			 len = sizeof(struct sockaddr_in6);
			 count = recvfrom(recvSockfd, recvbuf, sizeof(msgUnion_t), 0, (struct sockaddr *)&cliaddr, (socklen_t *)&len);

			 if(count >  0)
			 {
	///////////////// Copy from Inernet - End
	 ///////////////////////////////////////////////////////////////////
				msgUnion_t * msg = new msgUnion_t;
				memcpy (msg, recvbuf, sizeof(msgUnion_t));

				std::cout <<std::endl<< " ----- Receiving Message Header" << std::endl;


				ReqMsgQueue.push_back(msg);
				printf(" receive packet [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);

			 }else perror("packet receive error\n");
		}
	}
	printf("Thread Broken [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
	//#]
}
*/

void init_scheduler_message_thread_cp(void * p_UE) {

    pthread_t       ReqMsgProc_threadID;
    pthread_t       SynchMsgProc_threadID;
    pthread_t       SchPktProc_threadID;


    // create UEMsgProc Thread
    if (pthread_create(&ReqMsgProc_threadID, NULL, ReqMsgProc,(void *) p_UE) != 0)
    {
        printf("Scheduler ReqMsgProc Failed [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
    } else
    {
        printf("Scheduler ReqMsgProc is started [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
        //rcfbMsgProc_Status = 1;
    }

    if (pthread_create(&SynchMsgProc_threadID, NULL, SynchMsgProc,(void *) p_UE) != 0)
    {
        printf("Scheduler SynchMsgProc Failed [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
        //rcfbMsgProc_Status = 0;
    } else
    {
        printf("Scheduler SynchMsgProc is started [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
        //rcfbMsgProc_Status = 1;
    }
/*
    // create UEMsgProc Thread
    if (pthread_create(&SchPktProc_threadID, NULL, init_scheduler_socket_thread_cp, (void*) p_UE) != 0)
    {
        printf("Scheduler PacketProc Failed [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
        //rcfbMsgProc_Status = 0;
    }
    else
    {
        printf("UE PacketProc is started [%s, %d, %s]\n", __FILE__, __LINE__, __FUNCTION__);
        //rcfbMsgProc_Status = 1;
    }
*/
    return;

}
