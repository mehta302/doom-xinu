/* -----------------------------------------------------------------------
 * Constants — copied directly from rfilesys.h
 * ---------------------------------------------------------------------- */
#define RF_NAMLEN       128
#define RF_DATALEN      1024
#define RF_SERVER_PORT  9000
 
/* Status values used in rf_status field of responses                   */
#define RF_STATUS_OK    0               /* Success                      */
#define RF_STATUS_ERR   0xFFFF          /* Failure (maps to SYSERR)     */
 
/* Message type codes — must match rfilesys.h exactly                   */
#define RF_MSG_RESPONSE 0x0100
 
#define RF_MSG_RREQ     0x0001
#define RF_MSG_RRES     (RF_MSG_RREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_WREQ     0x0002
#define RF_MSG_WRES     (RF_MSG_WREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_OREQ     0x0003
#define RF_MSG_ORES     (RF_MSG_OREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_DREQ     0x0004
#define RF_MSG_DRES     (RF_MSG_DREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_TREQ     0x0005
#define RF_MSG_TRES     (RF_MSG_TREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_SREQ     0x0006
#define RF_MSG_SRES     (RF_MSG_SREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_MREQ     0x0007
#define RF_MSG_MRES     (RF_MSG_MREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_XREQ     0x0008
#define RF_MSG_XRES     (RF_MSG_XREQ | RF_MSG_RESPONSE)
 
#define RF_MSG_CREQ     0x0009
#define RF_MSG_CRES     (RF_MSG_CREQ | RF_MSG_RESPONSE)
 
/* -----------------------------------------------------------------------
 * Message structs — mirror rfilesys.h exactly with #pragma pack(2)
 *
 * RF_MSG_HDR macro expands to the common header fields present in
 * every message. We replicate it here as a concrete struct and also
 * inline it into each operation struct exactly as Xinu does.
 * ---------------------------------------------------------------------- */
 
#pragma pack(2)
 
/* Common header — present at the start of every message                */
struct rf_msg_hdr {
        uint16_t  rf_type;              /* Message type                 */
        uint16_t  rf_status;            /* 0 in req, status in response */
        uint32_t  rf_seq;               /* Sequence number              */
        char      rf_name[RF_NAMLEN];   /* Null-terminated file name    */
};
 
/* Read request (Xinu → server)                                         */
struct rf_msg_rreq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        uint32_t  rf_pos;              /* Byte offset to read from      */
        uint32_t  rf_len;              /* Bytes requested (1..1024)     */
};
 
/* Read response (server → Xinu)                                        */
struct rf_msg_rres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        uint32_t  rf_pos;              /* Position read from            */
        uint32_t  rf_len;              /* Bytes returned (0 = EOF)      */
        char      rf_data[RF_DATALEN]; /* Payload                       */
};
 
/* Write request (Xinu → server)                                        */
struct rf_msg_wreq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        uint32_t  rf_pos;              /* Byte offset to write at       */
        uint32_t  rf_len;              /* Bytes to write                */
        char      rf_data[RF_DATALEN]; /* Data to write                 */
};
 
/* Write response (server → Xinu)                                       */
struct rf_msg_wres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        uint32_t  rf_pos;              /* Position written at           */
        uint32_t  rf_len;              /* Bytes actually written        */
};
 
/* Open request (Xinu → server)                                         */
struct rf_msg_oreq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        int32_t   rf_mode;             /* Xinu mode bits (F_MODE_*)     */
};
 
/* Open response (server → Xinu)                                        */
struct rf_msg_ores {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        int32_t   rf_mode;             /* Echo of mode bits             */
};
 
/* Close request — header only                                          */
struct rf_msg_creq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
 
/* Close response — header only                                         */
struct rf_msg_cres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
 
/* Size request — header only                                           */
struct rf_msg_sreq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
 
/* Size response                                                        */
struct rf_msg_sres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
        uint32_t  rf_size;             /* File size in bytes            */
};
 
/* Delete request/response — header only                                */
struct rf_msg_dreq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
struct rf_msg_dres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
 
/* Truncate request/response — header only                              */
struct rf_msg_treq {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
struct rf_msg_tres {
        uint16_t  rf_type;
        uint16_t  rf_status;
        uint32_t  rf_seq;
        char      rf_name[RF_NAMLEN];
};
 
#pragma pack()
 
/* -----------------------------------------------------------------------
 * Maximum UDP message size — must fit the largest struct
 * rf_msg_rres is the largest: HDR(138) + pos(4) + len(4) + data(1024)
 * ---------------------------------------------------------------------- */
#define RF_MAX_UDP  (sizeof(struct rf_msg_rres))
 
/* -----------------------------------------------------------------------
 * Xinu mode bit values (from Xinu's file.h / rfilesys.h references)
 * ---------------------------------------------------------------------- */
#define F_MODE_R    0x01                /* Read access                  */
#define F_MODE_W    0x02                /* Write access                 */
#define F_MODE_RW   0x03               /* Read+write                   */
#define F_MODE_N    0x04                /* New file (create)            */
#define F_MODE_O    0x08                /* Old file (must exist)        */
 
/* -----------------------------------------------------------------------
 * Server state
 *
 * Xinu's RFS design is stateless from the server's perspective:
 * every read/write carries the filename and absolute position.
 * We still cache open FILE* handles keyed by filename for performance
 * — avoids reopening the file for every read request.
 * ---------------------------------------------------------------------- */
#define MAX_OPEN_FILES  16
 
typedef struct {
        char  name[RF_NAMLEN];   /* Empty string = slot is free         */
        FILE *fp;
} open_file_t;
