/* videosend.c - videosend, videostream, get_next_frame */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * Video definitions
 *------------------------------------------------------------------------
 */
extern unsigned char _binary_badapple_raw_start[];
uint32 frames = 600;
byte *buf = (byte*) _binary_badapple_raw_start;

/*------------------------------------------------------------------------
 * Network configuration
 *------------------------------------------------------------------------
 */
#define VIDEO_SERVER_IP     "10.168.14.69"  /* Unix server IPv4 address */
#define VIDEO_SERVER_PORT   9000            /* UDP port on Unix server  */
#define VIDEO_LOCAL_PORT    9001            /* UDP port on Galileo      */

/*------------------------------------------------------------------------
 * Video format
 *------------------------------------------------------------------------
 */
#define VIDEO_FRAME_W       160
#define VIDEO_FRAME_H       120
#define VIDEO_FRAME_SIZE    (VIDEO_FRAME_W * VIDEO_FRAME_H)  /* 19200  */

/*
 * Maximum UDP payload per packet.
 * net_udpdata is 1500-28 = 1472 bytes, but we use 1400 to stay
 * comfortably within any intermediate MTU and leave room for our
 * small chunk header prepended inside the payload.
 */
#define VIDEO_PAYLOAD_SIZE  1400

/*
 * Chunk header layout (prepended to every UDP payload, 6 bytes total):
 *
 *   bytes 0-1 : frame_id      (uint16, big-endian) — frame counter
 *   bytes 2-3 : chunk_index   (uint16, big-endian) — 0-based chunk number
 *   bytes 4-5 : total_chunks  (uint16, big-endian) — chunks per frame
 *
 * Pixel data follows immediately after the 6-byte header.
 */
#define VIDEO_CHUNK_HDR_SIZE    6
#define VIDEO_PIXEL_SIZE        (VIDEO_PAYLOAD_SIZE - VIDEO_CHUNK_HDR_SIZE)

/* Total chunks needed per frame (ceiling division)                     */
#define VIDEO_TOTAL_CHUNKS  \
        ((VIDEO_FRAME_SIZE + VIDEO_PIXEL_SIZE - 1) / VIDEO_PIXEL_SIZE)

/*------------------------------------------------------------------------
 * put16be  -  Write a uint16 in big-endian order into a byte buffer
 *------------------------------------------------------------------------
 */
static inline void put16be(byte *buf, uint16 val)
{
  buf[0] = (byte)(val >> 8);
  buf[1] = (byte)(val & 0xFF);
}

/*------------------------------------------------------------------------
 * videosend  -  Send one 160x120 8bpp grayscale frame via UDP
 *
 *   framebuf : pointer to VIDEO_FRAME_SIZE bytes of grayscale pixels
 *   frame_id : monotonically increasing frame counter
 *
 * Returns OK on success, SYSERR on failure.
 *------------------------------------------------------------------------
 */
int32   videosend(
  byte    *framebuf,    /* Pointer to raw 160x120 pixel buffer  */
  uint16   frame_id     /* Frame sequence number                */
) {
  uid32   slot;                   /* UDP table slot               */
  uint32  serverip;               /* Server IP as uint32          */
  byte    pktbuf[VIDEO_PAYLOAD_SIZE]; /* Outgoing packet buffer   */
  uint16  chunk_index;            /* Current chunk (0-based)      */
  uint32  offset;                 /* Byte offset into framebuf    */
  uint16  pixel_bytes;            /* Pixel bytes in this chunk    */
  uint32  remaining;              /* Bytes left to send           */
  status  retval;                 /* Return from udp_sendto       */

  if (framebuf == NULL) {
    return SYSERR;
  }

  if (dot2ip(VIDEO_SERVER_IP, &serverip) == SYSERR) {
    return SYSERR;
  }

  /* Register a UDP slot: remip/remport fixed, send-only          */
  slot = udp_register(serverip, VIDEO_SERVER_PORT, VIDEO_LOCAL_PORT);
  if ((int32)slot == SYSERR) {
    kprintf("videosend: udp_register failed\n");
    return SYSERR;
  }

  chunk_index = 0;
  offset      = 0;
  remaining   = VIDEO_FRAME_SIZE;

  while (remaining > 0) {

    /* How many pixel bytes go in this chunk                */
    pixel_bytes = (remaining > VIDEO_PIXEL_SIZE)
                    ? (uint16)VIDEO_PIXEL_SIZE
                    : (uint16)remaining;

    /* Build 6-byte chunk header                            */
    put16be(pktbuf + 0, frame_id);
    put16be(pktbuf + 2, chunk_index);
    put16be(pktbuf + 4, (uint16)VIDEO_TOTAL_CHUNKS);

    /* Copy pixel data after header                         */
    memcpy(pktbuf + VIDEO_CHUNK_HDR_SIZE,
           framebuf + offset,
           pixel_bytes);

    /* Send via Xinu UDP stack                              */
    retval = udp_sendto(slot,
                        serverip,
                        VIDEO_SERVER_PORT,
                        (char *)pktbuf,
                        VIDEO_CHUNK_HDR_SIZE + pixel_bytes);

    if (retval == SYSERR) {
      kprintf("videosend: udp_sendto failed chunk %d\n",
              chunk_index);
      udp_release(slot);
      return SYSERR;
    }

    offset      += pixel_bytes;
    remaining   -= pixel_bytes;
    chunk_index++;
  }

  udp_release(slot);
  return OK;
}

/*------------------------------------------------------------------------
 * videostream  -  Xinu process: continuously stream frames over UDP
 *------------------------------------------------------------------------
 */
void    videostream(void)
{
  uint16  frame_id = 0;
  byte   *framebuf;

  kprintf("videostream: starting\n");
  kprintf("  destination : %s:%d\n", VIDEO_SERVER_IP, VIDEO_SERVER_PORT);
  kprintf("  frame size  : %d bytes (%dx%d 8bpp)\n",
          VIDEO_FRAME_SIZE, VIDEO_FRAME_W, VIDEO_FRAME_H);
  kprintf("  chunks/frame: %d x %d pixel bytes\n",
          VIDEO_TOTAL_CHUNKS, VIDEO_PIXEL_SIZE);

  while (1) {

    framebuf = get_next_frame();

    if (framebuf == NULL) {
      kprintf("videostream: no frame, stopping\n");
      break;
    }

    if (videosend(framebuf, frame_id) == SYSERR) {
      kprintf("videostream: send error frame %d\n",
              frame_id);
    }

    kprintf("Sent Frame %d\n", frame_id++);     /* Wraps at 65535 -> 0 naturally        */
  }

  kprintf("videostream: done\n");
}

/*------------------------------------------------------------------------
 * get_next_frame  -  Xinu process: get the next frame stored in buf
 * returns NULL if no more frames (streaming at 10 fps)
 *------------------------------------------------------------------------
 */
byte *get_next_frame(void)
{
  byte *retbuf;

  if (frames--) {
    retbuf = buf;
    buf += VIDEO_FRAME_SIZE;
    sleepms(100);
    return retbuf;
  }
  else {
    return NULL;
  }
}
