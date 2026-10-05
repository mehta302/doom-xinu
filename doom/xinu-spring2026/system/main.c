/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
  char **argv = (char *[]){"doomgeneric"};
  doomgeneric_Create(1, argv);
  //while (1) {
  //  doomgeneric_Tick();
  //}
  kprintf("Exiting the main process");
  return OK;
 //    
 //    	kprintf("\nHello World!\n");
 //    	kprintf("\nI'm the first XINU app and running function main() in system/main.c.\n");
 //    	kprintf("\nI was created by nulluser() in system/initialize.c using create().\n");
 //    	kprintf("\nMy creator will turn itself into the do-nothing null process.\n");
 //    	kprintf("\nI will create a second XINU app that runs shell() in shell/shell.c as an example.\n");
 //    	kprintf("\nYou can do something else, or do nothing; it's completely up to you.\n");
 //    	kprintf("\n...creating a shell\n");
	//
	// /* Run the Xinu shell */
	//
	// recvclr();
	// resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	//
	// /* Wait for shell to exit and recreate it */
	//
	// while (TRUE) {
	// 	receive();
	// 	sleepms(200);
	// 	kprintf("\n\nMain process recreating shell\n\n");
	// 	resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	// }
	// return OK;
 //    
//  char fbuf[1024];
//  int32 fd;
//  int32 len;
//  uint32 ipaddr;
//  int32 retval;
//  int32 slot;
//  static int32 seq = 0;
//  char buf[56];
//  int32 i;
//  int32 nextval;
//
//  ipaddr = dnslookup("xinuserver.cs.purdue.edu");
//  slot = icmp_register(ipaddr);
//  if (slot == SYSERR) {
//    kprintf("ICMP registration failed\n");
//    return SYSERR;
//  }
//  nextval = seq;
//  for (i = 0; i < sizeof(buf); i++) {
//    buf[i] = 0xff & nextval++;
//  }
//  retval = icmp_send(ipaddr, ICMP_ECHOREQST, slot, seq++, buf, sizeof(buf));
//  if (retval == SYSERR) {
//    kprintf("No response from host\n");
//    icmp_release(slot);
//    return SYSERR;
//  }
//  retval = icmp_recv(slot, buf, sizeof(buf), 3000);
//  icmp_release(slot);
//  if (retval == TIMEOUT) {
//    kprintf("No response from host now\n");
//    return SYSERR;
//  }
//  if (retval != sizeof(buf)) {
//    kprintf("Expected %d bytes but got back %d\n", sizeof(buf), retval);
//    return SYSERR;
//  }
//  kprintf("Host is alive\n");
//
//  kprintf("OPENING FILE\n");
//  fd = open(RFILESYS, "test.txt", "ro");
//  if (fd == SYSERR) {
//    kprintf("FAILED TO OPEN\n");
//    return SYSERR;
//  }
//
//  len = 42;
//  kprintf("READING FILE\n");
//  read(fd, fbuf, len);
//  buf[42] = '\0';
//  kprintf("READ: %s\n", fbuf);
}
