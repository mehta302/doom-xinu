// #include "doomgeneric.h"
// #include "xinu_shims.h"
// #include "doomkeys.h"
// #include <string.h>

// Xinu headers would be included here in a real environment
#include <xinu.h>

static int32_t s_Socket = -1;

#define KEYQUEUE_SIZE 16
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

void DG_Init() {
    memset(s_KeyQueue, 0, sizeof(s_KeyQueue));
    
    // Connect to the Unix server
    // s_Socket = open(TCP, SERVER_ADDR, "rw");
    // if (s_Socket < 0) {
        // Handle error - maybe print to Xinu console
    // }
}

void DG_DrawFrame() {
    // if (s_Socket >= 0) {
    //     // Send the entire screen buffer to the Unix server
    //     // Header: 'F' (Frame), followed by size
    //     uint32_t size = DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(pixel_t);
    //     char header[5] = {'F', 0, 0, 0, 0};
    //     memcpy(&header[1], &size, 4);
    //     
    //     write(s_Socket, header, 5);
    //     write(s_Socket, DG_ScreenBuffer, size);
    // }
    
    // Check for incoming key events from the server
    // We can use control() with TCP_AVAILABLE or similar if available, 
    // or just a non-blocking read if supported.
    // For now, assume we can do a check or the server sends it.
    
    // unsigned char key_packet[2];
    // In Xinu, we might need a way to check if data is available 
    // to avoid blocking DG_DrawFrame.
    // if (control(s_Socket, TCP_AVAIL, 0, 0) > 0)
    // {
    //     int32_t n = read(s_Socket, key_packet, 2);
    //     if (n == 2) {
    //         int pressed = key_packet[0];
    //         unsigned char key = key_packet[1];
    //         
    //         unsigned short keyData = (pressed << 8) | key;
    //         s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
    //         s_KeyQueueWriteIndex = (s_KeyQueueWriteIndex + 1) % KEYQUEUE_SIZE;
    //     }
    // }
}

void DG_SleepMs(uint32_t ms) {
    // Already shimmed in xinu_shims.c as usleep or via direct call
    sleepms(ms);
}

uint32_t DG_GetTicksMs() {
    return clktime * 1000; // Very coarse resolution
}

int DG_GetKey(int* pressed, unsigned char* key) {
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
        return 0;
    }
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex = (s_KeyQueueReadIndex + 1) % KEYQUEUE_SIZE;
    
    *pressed = keyData >> 8;
    *key = keyData & 0xFF;
    return 1;
}

void DG_SetWindowTitle(const char * title) {
}

// int main(int argc, char **argv) {
//     doomgeneric_Create(argc, argv);
//     while(1) {
//         doomgeneric_Tick();
//     }
//     return 0;
// }
