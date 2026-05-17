/* ethwrite.c - ethwrite */

#include <xinu.h>

#define VLAN_ID 1427
#define VLAN_TPID_HI 0x81
#define VLAN_TPID_LO 0x00
#define VLAN_TCI_HI ((0 << 5) | (0 << 4) | ((VLAN_ID >> 8) & 0x0F))
#define VLAN_TCI_LO (VLAN_ID & 0xFF)

/*------------------------------------------------------------------------
 * ethwrite  -  enqueue packet for transmission on Intel Quark Ethernet
 *------------------------------------------------------------------------
 */
devcall	ethwrite	(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buf,			/* Buffer that hols a packet	*/
	  int32	len 			/* Length of the packet		*/
	)
{
	struct	ethcblk *ethptr;	/* Pointer to control block	*/
	struct	eth_q_csreg *csrptr;	/* Address of device CSRs	*/
	volatile struct	eth_q_tx_desc *descptr; /* Ptr to descriptor	*/
	uint32 i;			/* Counts bytes during copy	*/
  /*char *txbuf;*/
  /*int32 taggedlen;*/

	ethptr = &ethertab[devptr->dvminor];

	csrptr = (struct eth_q_csreg *)ethptr->csr;

	/* Wait for an empty slot in the transmit descriptor ring */

	wait(ethptr->osem);

	/* Point to the tail of the descriptor ring */

	descptr = (struct eth_q_tx_desc *)ethptr->txRing + ethptr->txTail;

	/* Increment the tail index and wrap, if needed */

	ethptr->txTail += 1;
	if(ethptr->txTail >= ethptr->txRingSize) {
		ethptr->txTail = 0;
	}

	/* Add packet length to the descriptor */

	descptr->buf1size = len;

	/* Copy packet into the buffer associated with the descriptor	*/

	for(i = 0; i < len; i++) {
		*((char *)descptr->buffer1 + i) = *((char *)buf + i);
	}

  /*txbuf = (char*) descptr->buffer1;*/
  /*taggedlen = len + VLAN_TAG_LEN;*/
  /*memcpy(txbuf, buf, ETH_DST_SRC_LEN);*/

  /*txbuf[ETH_DST_SRC_LEN] = (char) VLAN_TPID_HI;*/
  /*txbuf[ETH_DST_SRC_LEN+1] = (char) VLAN_TPID_LO;*/
  /*txbuf[ETH_DST_SRC_LEN+2] = (char) VLAN_TCI_HI;*/
  /*txbuf[ETH_DST_SRC_LEN+3] = (char) VLAN_TCI_LO;*/

  /*memcpy(txbuf+ETH_DST_SRC_LEN+VLAN_TAG_LEN, buf+ETH_DST_SRC_LEN, len-ETH_DST_SRC_LEN);*/
  /*descptr->buf1size = taggedlen;*/

	/* Mark the descriptor if we are at the end of the ring */

	if(ethptr->txTail == 0) {
		descptr->ctrlstat = ETH_QUARK_TDCS_TER;
	} else {
		descptr->ctrlstat = 0;
	}

	/* Initialize the descriptor */

	descptr->ctrlstat |=
		(ETH_QUARK_TDCS_OWN | /* The desc is owned by DMA	*/
		 ETH_QUARK_TDCS_IC  | /* Interrupt after transfer	*/
		 ETH_QUARK_TDCS_LS  | /* Last segment of packet		*/
		 ETH_QUARK_TDCS_FS);  /* First segment of packet	*/

	/* Un-suspend DMA on the device */

	csrptr->tpdr = 1;

	return OK;
}
