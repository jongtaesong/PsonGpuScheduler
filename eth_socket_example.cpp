/*
 * eth_socket_example.cpp
 *
 *  Created on: Jul 5, 2018
 *      Author: jsong
 */




#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>

#define ETHER_TYPE 0x8100 /* custom type */

#define BUF_SIZE (ETH_FRAME_LEN)

// Add more line for GITHUB
static char broadcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static void
print_usage (const char *progname)
{
  fprintf (stderr, "usage: %s [-l] [-i device] [-d dest-addr] msg\n", progname);
}

int
init_eth_socket (int   argc,
      char **argv)
{
  char *if_name;
  int listen;
  char *msg;
  int sock;
  int if_index;
  uint8_t if_addr[ETH_ALEN];
  uint8_t dest_addr[ETH_ALEN];
  size_t send_len;
  char buf[BUF_SIZE];
  int i;

  //if_name = "eth0";
  if_name = "enp0s31f6";
  printf("if_name %s \n", if_name);

    memcpy (dest_addr, broadcast_addr, ETH_ALEN);
  listen = 0;
  msg = "Hello";

  {
    int opt;

    while ((opt = getopt (argc, argv, "li:d:")) != -1)
      {
        // printf("opt %s %d \n", opt, opt);

        switch (opt)
          {
          case 'l':
        	printf("case l \n");
            listen = 1;
            break;
          case 'i':
          	printf("case i \n");
            if_name = optarg;
            break;
          case 'd':
          	printf("case d \n");
            {
              int mac[ETH_ALEN];

              if (ETH_ALEN != sscanf (optarg,
                                      "%02x:%02x:%02x:%02x:%02x:%02x",
                                      &mac[0],
                                      &mac[1],
                                      &mac[2],
                                      &mac[3],
                                      &mac[4],
                                      &mac[5]))
                {
                  print_usage (argv[0]);
                  return EXIT_FAILURE;
                }
              for (i = 0; i < ETH_ALEN; i++)
                dest_addr[i] = mac[i];
            }
            break;
          default: /* '?' */
          	printf("case default \n");
            print_usage (argv[0]);
            return EXIT_FAILURE;
          }
      }

    if (optind < argc)
      msg = argv[optind];
  }

  /* Create the AF_PACKET socket. */
  sock = socket (AF_PACKET, SOCK_RAW, htons (ETHER_TYPE));
  if (sock < 0)
    perror ("socket()");

  /* Get the index number and MAC address of ethernet interface. */
  {
    struct ifreq ifr;

    memset (&ifr, 0, sizeof (ifr));
    strncpy (ifr.ifr_name, if_name, IFNAMSIZ - 1);

    if (ioctl (sock, SIOCGIFINDEX, &ifr) < 0)
      perror ("SIOCGIFINDEX");
    if_index = ifr.ifr_ifindex;

    if (ioctl (sock, SIOCGIFHWADDR, &ifr) < 0)
      perror ("SIOCGIFHWADDR");
    memcpy (if_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
  }

  if (listen)
    {
	  printf("start listening \n");

	  struct ifreq ifr;
      int s;

      memset (&ifr, 0, sizeof (ifr));
      strncpy (ifr.ifr_name, if_name, IFNAMSIZ - 1);

      /* Set interface to promiscuous mode. */
      if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
        perror ("SIOCGIFFLAGS");
      ifr.ifr_flags |= IFF_PROMISC;
      if (ioctl (sock, SIOCSIFFLAGS, &ifr) < 0)
        perror ("SIOCSIFFLAGS");

      /* Allow the socket to be reused. */
      s = 1;
      if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &s, sizeof (s)) < 0)
        {
          perror ("SO_REUSEADDR");
          close (sock);
          return EXIT_FAILURE;
        }

      /* Bind to device. */
      if (setsockopt (sock, SOL_SOCKET, SO_BINDTODEVICE, if_name, IFNAMSIZ - 1) < 0)
        {
          perror ("SO_BINDTODEVICE");
          close (sock);
          return EXIT_FAILURE;
        }

      while (1)
        {
          printf("while loop \n");
          struct ether_header *eh = (struct ether_header *) buf;
          ssize_t received;
          char *p;

          printf("before recvfrom \n");

          received = recvfrom (sock, buf, BUF_SIZE, 0, NULL, NULL);
          printf("after recvfrom \n");
          if (received <= 0)
          {
          	printf("break \n");

            break;
          }
          /* Receive only destination address is broadcast or me. */
          if (memcmp (eh->ether_dhost, if_addr, ETH_ALEN) != 0 &&
              memcmp (eh->ether_dhost, broadcast_addr, ETH_ALEN) != 0)
          {
        	printf("memcmp loop \n");

            continue;
          }

          fprintf (stdout,
                   "%02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x ",
                   eh->ether_shost[0],
                   eh->ether_shost[1],
                   eh->ether_shost[2],
                   eh->ether_shost[3],
                   eh->ether_shost[4],
                   eh->ether_shost[5],
                   eh->ether_dhost[0],
                   eh->ether_dhost[1],
                   eh->ether_dhost[2],
                   eh->ether_dhost[3],
                   eh->ether_dhost[4],
                   eh->ether_dhost[5]);
          printf ( "%02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x ",
                   eh->ether_shost[0],
                   eh->ether_shost[1],
                   eh->ether_shost[2],
                   eh->ether_shost[3],
                   eh->ether_shost[4],
                   eh->ether_shost[5],
                   eh->ether_dhost[0],
                   eh->ether_dhost[1],
                   eh->ether_dhost[2],
                   eh->ether_dhost[3],
                   eh->ether_dhost[4],
                   eh->ether_dhost[5]);

          received -= sizeof (*eh);
          p = buf + sizeof (*eh);
          for (i = 0; i < received; i++)
            fputc (p[i], stdout);

          fputc ('\n', stdout);
        }

      close (sock);

      return 0;
    }

  memset (buf, 0, BUF_SIZE);

  /* Construct ehternet header. */
  {
    struct ether_header *eh;

    /* Ethernet header */
    eh = (struct ether_header *) buf;
    memcpy (eh->ether_shost, if_addr, ETH_ALEN);
    memcpy (eh->ether_dhost, dest_addr, ETH_ALEN);
    eh->ether_type = htons (ETHER_TYPE);

    send_len = sizeof (*eh);
  }

  /* Fill the packet data. */
  for (i = 0; msg[i] != '\0'; i++)
    buf[send_len++] = msg[i];

  /* Fill the destination address and send it. */
  {
    struct sockaddr_ll sock_addr;

    sock_addr.sll_ifindex = if_index;
    sock_addr.sll_halen = ETH_ALEN;
    memcpy (sock_addr.sll_addr, dest_addr, ETH_ALEN);

    if (sendto (sock, buf, send_len, 0,
                (struct sockaddr *) &sock_addr, sizeof (sock_addr)) < 0)
      perror ("sendto()");
  }

  close (sock);

  return 0;
}
