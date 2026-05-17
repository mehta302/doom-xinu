/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
  uint32 ipaddr;
  int32 retval;
  int32 slot;
  static int32 seq = 0;
  char buf[56];
  int32 i;
  int32 nextval;

  ipaddr = dnslookup("xinuserver.cs.purdue.edu");
  slot = icmp_register(ipaddr);
  if (slot == SYSERR) {
    kprintf("ICMP registration failed\n");
    return SYSERR;
  }
  nextval = seq;
  for (i = 0; i < sizeof(buf); i++) {
    buf[i] = 0xff & nextval++;
  }
  retval = icmp_send(ipaddr, ICMP_ECHOREQST, slot, seq++, buf, sizeof(buf));
  if (retval == SYSERR) {
    kprintf("No response from host\n");
    icmp_release(slot);
    return SYSERR;
  }
  retval = icmp_recv(slot, buf, sizeof(buf), 3000);
  icmp_release(slot);
  if (retval == TIMEOUT) {
    kprintf("No response from host now\n");
    return SYSERR;
  }
  if (retval != sizeof(buf)) {
    kprintf("Expected %d bytes but got back %d\n", sizeof(buf), retval);
    return SYSERR;
  }
  kprintf("Host is alive\n");

  //resume(create(rendervideo, 8192, 50, "rendervideo", 0));
  resume(create(videostream, 8192, 50, "videostream", 0));
	//recvclr();
	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	
	/* Wait for shell to exit and recreate it */
	
	/*while (TRUE) {*/
		/*receive();*/
		/*sleepms(200);*/
		/*kprintf("\n\nMain process recreating shell\n\n");*/
		/*resume(create(shell, 4096, 20, "shell", 1, CONSOLE));*/
	/*}*/
	return OK;
}
