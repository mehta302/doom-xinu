/* rendervideo.c - rendervideo, renderframe */

#include <xinu.h>
#include <stdio.h>

extern unsigned char _binary_badapple_raw_start[];
static const char ramp[] = "@%#*+=-:. ";
#define RAMP_LEN (sizeof(ramp) - 1)
#define FRAME_H 34
#define FRAME_W 138
#define FRAME_SIZE (FRAME_H * FRAME_W)
#define FRAMES 3287

// void renderframe(int frame)
// {
//     unsigned char *base = _binary_badapple_raw_start;
//     unsigned char *img = base + frame * FRAME_SIZE;
//
//     int x, y;
//     for (y = 0; y < FRAME_H; y++) {
//         for (x = 0; x < FRAME_W; x++) {
//             unsigned char p = img[y * FRAME_W + x];
//             int idx = (p * (RAMP_LEN - 1)) / 255;
//             kprintf("%c", ramp[idx]);
//         }
//         kprintf("\n");
//     }
// }

void renderframe(int frame)
{
    unsigned char *base = _binary_badapple_raw_start;
    unsigned char *img  = base + frame * FRAME_SIZE;

    static char outbuf[(FRAME_W + 1) * FRAME_H + 16];
    int pos = 0;

    int x, y;
    for (y = 0; y < FRAME_H; y++) {
        for (x = 0; x < FRAME_W; x++) {
            unsigned char p = img[y * FRAME_W + x];
            int idx = (p * (RAMP_LEN - 1)) / 255;
            outbuf[pos++] = ramp[idx];
        }
        outbuf[pos++] = '\n';
    }

    /* Move cursor to top-left (avoid full clear) */
    outbuf[pos++] = '\033';
    outbuf[pos++] = '[';
    outbuf[pos++] = 'H';

    write(CONSOLE, outbuf, pos);
}

void rendervideo(void)
{
    int frames = FRAMES;
    int i;

    for (i = 0; i < frames; i++) {
        kprintf("\033[H\033[J");  // ANSI clear screen
        renderframe(i);
        sleepms(330);  // ~3 FPS
    }
}

