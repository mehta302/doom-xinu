/*
 * sdl_receiver.c
 *
 * Receives UDP packets from a Xinu/Galileo board and renders each
 * 160x120 8bpp grayscale frame using SDL 1.2.
 *
 * Transport : Plain UDP socket
 * Interface : eth0.1427 (10.168.14.69) facing the Galileo
 * Display   : SDL 1.2 surface, X11-forwarded via SSH -Y
 *
 * Build:
 *   gcc sdl_reciever.c `sdl-config --cflags --libs` -lX11 -o sdl_reciever
 *
 * Run (no root required):
 *   ./sdl_receiver
 *
 * X11 forwarding: connect via  ssh -Y user@server  before running.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

/* UDP socket headers */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* SDL 1.2 */
#include <SDL/SDL.h>

/* -----------------------------------------------------------------------
 * Constants — must match videosend.c on the Xinu side
 * ---------------------------------------------------------------------- */
#define VIDEO_LISTEN_IP     "10.168.14.69"  /* Bind to this local IP    */
#define VIDEO_PORT          9000            /* UDP port to listen on    */

#define VIDEO_FRAME_W       160
#define VIDEO_FRAME_H       120
#define VIDEO_FRAME_SIZE    (VIDEO_FRAME_W * VIDEO_FRAME_H)  /* 19200  */

#define VIDEO_PAYLOAD_SIZE  1400
#define VIDEO_CHUNK_HDR_SIZE    6
#define VIDEO_PIXEL_SIZE    (VIDEO_PAYLOAD_SIZE - VIDEO_CHUNK_HDR_SIZE)
#define VIDEO_TOTAL_CHUNKS  \
        ((VIDEO_FRAME_SIZE + VIDEO_PIXEL_SIZE - 1) / VIDEO_PIXEL_SIZE)

/* Galileo's IP — packets from any other source are ignored             */
#define GALILEO_IP          "10.168.14.119"

/* -----------------------------------------------------------------------
 * Chunk header parsing helpers
 * ---------------------------------------------------------------------- */
static inline uint16_t read16be(const uint8_t *p)
{
        return (uint16_t)((p[0] << 8) | p[1]);
}

/* -----------------------------------------------------------------------
 * Reassembly state
 * ---------------------------------------------------------------------- */
typedef struct {
        uint8_t  pixels[VIDEO_FRAME_SIZE];      /* Assembled frame buf  */
        uint8_t  received[VIDEO_TOTAL_CHUNKS];  /* Per-chunk recv flag  */
        uint16_t frame_id;                      /* Current frame id     */
        int      chunks_received;               /* Count received so far*/
        int      initialized;                   /* Seen first frame?    */
} FrameAssembler;

/* -----------------------------------------------------------------------
 * Globals
 * ---------------------------------------------------------------------- */
static volatile int g_running = 1;

static void handle_sigint(int sig)
{
        (void)sig;
        g_running = 0;
}

/* -----------------------------------------------------------------------
 * open_udp_socket  -  Bind a UDP socket to VIDEO_LISTEN_IP:VIDEO_PORT
 *
 * Returns socket fd on success, -1 on failure.
 * ---------------------------------------------------------------------- */
static int open_udp_socket(void)
{
        int sock;
        struct sockaddr_in addr;
        int reuse = 1;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
                printf("Cannot open socket\n");
                perror("socket(UDP)");
                return -1;
        }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(VIDEO_PORT);
        addr.sin_addr.s_addr = inet_addr(VIDEO_LISTEN_IP);

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                printf("Cannot bind socket\n");
                perror("bind(UDP)");
                close(sock);
                return -1;
        }

        printf("Listening on UDP %s:%d\n", VIDEO_LISTEN_IP, VIDEO_PORT);
        return sock;
}

/* -----------------------------------------------------------------------
 * init_sdl  -  Open a 160x120 32bpp SDL window
 *
 * We use 32bpp and convert 8bpp grayscale manually — more reliable
 * with X11 forwarding than SDL palette surfaces.
 *
 * Returns SDL_Surface* on success, NULL on failure.
 * ---------------------------------------------------------------------- */
static SDL_Surface *init_sdl(void)
{
        SDL_Surface *screen;

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
                return NULL;
        }

        screen = SDL_SetVideoMode(VIDEO_FRAME_W, VIDEO_FRAME_H, 32,
                                  SDL_SWSURFACE);
        if (!screen) {
                fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
                SDL_Quit();
                return NULL;
        }

        SDL_WM_SetCaption("Xinu Video Stream - 10.168.14.119", NULL);
        printf("SDL window opened: %dx%d\n", VIDEO_FRAME_W, VIDEO_FRAME_H);
        return screen;
}

/* -----------------------------------------------------------------------
 * render_frame  -  Convert 8bpp grayscale buffer to 32bpp and blit
 * ---------------------------------------------------------------------- */
static void render_frame(SDL_Surface *screen, const uint8_t *pixels)
{
        uint32_t *dst;
        int       i;
        uint8_t   g;

        if (SDL_LockSurface(screen) < 0) {
                fprintf(stderr, "SDL_LockSurface: %s\n", SDL_GetError());
                return;
        }

        dst = (uint32_t *)screen->pixels;
        for (i = 0; i < VIDEO_FRAME_W * VIDEO_FRAME_H; i++) {
                g = pixels[i];
                dst[i] = SDL_MapRGB(screen->format, g, g, g);
        }

        SDL_UnlockSurface(screen);
        SDL_Flip(screen);
}

/* -----------------------------------------------------------------------
 * reset_assembler  -  Clear state for a new incoming frame
 * ---------------------------------------------------------------------- */
static void reset_assembler(FrameAssembler *fa, uint16_t new_frame_id)
{
        memset(fa->received, 0, sizeof(fa->received));
        fa->frame_id        = new_frame_id;
        fa->chunks_received = 0;
}

/* -----------------------------------------------------------------------
 * process_packet  -  Parse one UDP payload and update frame assembler
 *
 * Returns 1 if a complete frame was assembled and rendered, 0 otherwise.
 * ---------------------------------------------------------------------- */
static int process_packet(const uint8_t *buf, int len,
                           FrameAssembler *fa, SDL_Surface *screen)
{
        uint16_t frame_id, chunk_index, total_chunks;
        uint16_t pixel_bytes;
        uint32_t pixel_offset;

        /* Minimum: 6-byte header + at least 1 pixel byte              */
        if (len < VIDEO_CHUNK_HDR_SIZE + 1) {
                printf("Packet too short: %d bytes\n", len);
                return 0;
        }

        /* Parse chunk header                                           */
        frame_id     = read16be(buf + 0);
        chunk_index  = read16be(buf + 2);
        total_chunks = read16be(buf + 4);
        pixel_bytes  = (uint16_t)(len - VIDEO_CHUNK_HDR_SIZE);

        /* Sanity checks                                                */
        if (total_chunks != VIDEO_TOTAL_CHUNKS) {
                printf("Bad total_chunks: %u (expected %u)\n",
                        total_chunks, VIDEO_TOTAL_CHUNKS);
                return 0;
        }
        if (chunk_index >= VIDEO_TOTAL_CHUNKS) {
                printf("chunk_index %u out of range\n", chunk_index);
                return 0;
        }

        /* New frame id — reset assembler                               */
        if (!fa->initialized || frame_id != fa->frame_id) {
                if (fa->initialized && fa->chunks_received > 0) {
                        printf("Frame %u incomplete (%d/%d), new frame %u\n",
                                fa->frame_id, fa->chunks_received,
                                VIDEO_TOTAL_CHUNKS, frame_id);
                }
                reset_assembler(fa, frame_id);
                fa->initialized = 1;
        }

        /* Skip duplicates                                              */
        if (fa->received[chunk_index]) {
                return 0;
        }

        /* Write pixel data into assembler buffer                       */
        pixel_offset = (uint32_t)chunk_index * VIDEO_PIXEL_SIZE;
        if (pixel_offset + pixel_bytes > VIDEO_FRAME_SIZE) {
                printf("Chunk overflow at index %u\n", chunk_index);
                return 0;
        }

        memcpy(fa->pixels + pixel_offset, buf + VIDEO_CHUNK_HDR_SIZE,
               pixel_bytes);
        fa->received[chunk_index] = 1;
        fa->chunks_received++;

        /* Frame complete — render it                                   */
        if (fa->chunks_received == VIDEO_TOTAL_CHUNKS) {
                render_frame(screen, fa->pixels);
                reset_assembler(fa, (uint16_t)(frame_id + 1));
                return 1;
        }

        return 0;
}

/* -----------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
        int             sock;
        SDL_Surface    *screen;
        FrameAssembler  fa;
        uint8_t         buf[VIDEO_PAYLOAD_SIZE];
        struct sockaddr_in sender;
        socklen_t       sender_len;
        int             pkt_len;
        uint64_t        frames_rendered = 0;
        SDL_Event       event;
        uint32_t        galileo_ip;

        signal(SIGINT, handle_sigint);

        galileo_ip = inet_addr(GALILEO_IP);

        sock = open_udp_socket();
        if (sock < 0) return 1;

        screen = init_sdl();
        if (!screen) { close(sock); return 1; }

        memset(&fa, 0, sizeof(fa));

        printf("Waiting for stream from Galileo (%s)...\n", GALILEO_IP);
        printf("Press Ctrl+C or close window to quit.\n");

        while (g_running) {

                /* Poll SDL events to keep window responsive            */
                while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT)
                                g_running = 0;
                        if (event.type == SDL_KEYDOWN &&
                            event.key.keysym.sym == SDLK_ESCAPE)
                                g_running = 0;
                }

                /* Non-blocking receive                                 */
                sender_len = sizeof(sender);
                pkt_len = recvfrom(sock, buf, sizeof(buf), MSG_DONTWAIT,
                                   (struct sockaddr *)&sender, &sender_len);

                if (pkt_len < 0) {
                  if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    SDL_Delay(1);
                    continue;
                  }
                  printf("Error recieving\n");
                  perror("recvfrom");
                  break;
                }

                /* Ignore packets not from the Galileo                  */
                if (sender.sin_addr.s_addr != galileo_ip) {
                  printf("Packet from another ip\n");
                  continue;
                }

                if (process_packet(buf, pkt_len, &fa, screen)) {
                  frames_rendered++;
                  if (frames_rendered % 30 == 0) {
                    printf("Frames rendered: %lu\n",
                           (unsigned long)frames_rendered);
                  }
                }
        }

        printf("\nDone. Total frames rendered: %lu\n",
               (unsigned long)frames_rendered);

        close(sock);
        SDL_Quit();
        return 0;
}
