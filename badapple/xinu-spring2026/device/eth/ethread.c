/* ethread.c - ethread */

#include <xinu.h>

#define VLAN_TPID 0x8100
#define ETH_TPID_OFFSET 12

/*------------------------------------------------------------------------
 * ethread  -  Read an incoming packet on Intel Quark Ethernet
 *------------------------------------------------------------------------
 */
devcall	ethread	(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buf,			/* Buffer for the packet	*/
	  int32	len 			/* Size of the buffer		*/
	)
{
	struct	ethcblk *ethptr;	/* Ethertab entry pointer	*/
	struct	eth_q_rx_desc *rdescptr;/* Pointer to the descriptor	*/
	struct	netpacket *pktptr;	/* Pointer to packet		*/
	uint32	framelen = 0;		/* Length of the incoming frame	*/
  /*uint16 tpid;*/
	bool8	valid_addr;
	int32	i;

	ethptr = &ethertab[devptr->dvminor];

	while(1) {

		/* Wait until there is a packet in the receive queue */

		wait(ethptr->isem);

		/* Point to the head of the descriptor list */

		rdescptr = (struct eth_q_rx_desc *)ethptr->rxRing +
							ethptr->rxHead;
		pktptr = (struct netpacket*)rdescptr->buffer1;

		/* See if destination address is our unicast address */

		if(!memcmp(pktptr->net_ethdst, ethptr->devAddress, 6)) {
			valid_addr = TRUE;

		/* See if destination address is the broadcast address */

		} else if(!memcmp(pktptr->net_ethdst,
                                    NetData.ethbcast,6)) {
            		valid_addr = TRUE;

		/* For multicast addresses, see if we should accept */

    		} else {
			valid_addr = FALSE;
			for(i = 0; i < (ethptr->ed_mcc); i++) {
				if(memcmp(pktptr->net_ethdst,
					ethptr->ed_mca[i], 6) == 0){
					valid_addr = TRUE;
					break;
				}
		        }
		}

		if(valid_addr == TRUE){ /* Accept this packet */

			/* Get the length of the frame */

			framelen = (rdescptr->status >> 16) & 0x00003FFF;

      /*tpid = (uint16) (*/
        /*(((uint8) ((char*) rdescptr->buffer1)[ETH_TPID_OFFSET]) << 8) |*/
        /*((uint8) ((char*) rdescptr->buffer1)[ETH_TPID_OFFSET+1])*/
      /*);*/

      /*if (tpid == VLAN_TPID) {*/
        /*char *rawbuf = (char*) rdescptr->buffer1;*/
        /*uint32 remaining = framelen - ETH_DST_SRC_LEN - VLAN_TAG_LEN;*/
        /*memmove(rawbuf+ETH_DST_SRC_LEN, rawbuf+ETH_DST_SRC_LEN+VLAN_TAG_LEN, remaining);*/
        /*for (i = 0; i < remaining; i++) {*/
          /**(rawbuf+ETH_DST_SRC_LEN+i) = *(rawbuf+ETH_DST_SRC_LEN+VLAN_TAG_LEN+i);*/
        /*}*/
        /*framelen -= VLAN_TAG_LEN;*/
      /*}*/

			/* Only return len characters to caller */

			if(framelen > len) {
				framelen = len;
			}

			/* Copy the packet into the caller's buffer */

			memcpy(buf, (void*)rdescptr->buffer1, framelen);
		}

        	/* Increment the head of the descriptor list */

		ethptr->rxHead += 1;
		if(ethptr->rxHead >= ETH_QUARK_RX_RING_SIZE) {
			ethptr->rxHead = 0;
		}

		/* Reset the descriptor to max possible frame len */

		rdescptr->buf1size = sizeof(struct netpacket);

		/* If we reach the end of the ring, mark the descriptor	*/

		if(ethptr->rxHead == 0) {
			rdescptr->rdctl1 |= (ETH_QUARK_RDCTL1_RER);
		}

		/* Indicate that the descriptor is ready for DMA input */

		rdescptr->status = ETH_QUARK_RDST_OWN;

		if(valid_addr == TRUE) {
			break;
		}
	}

	/* Return the number of bytes returned from the packet */

	return framelen;

}
