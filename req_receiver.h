#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <string.h>
#include <stdlib.h>

#define BUFF_SIZE 1024

void receive_req(int _switch_size);
