#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rfilesys.h"

static open_file_t  open_files[MAX_OPEN_FILES];
static volatile int g_running = 1;

static void handle_sigint(int sig) { (void)sig; g_running = 0; }

/* -----------------------------------------------------------------------
 * get_file  —  Return cached FILE* for name, opening it if needed.
 *              mode_bits are the Xinu F_MODE_* flags.
 *              Returns NULL on error.
 * ---------------------------------------------------------------------- */
static FILE *get_file(const char *name, int32_t mode_bits)
{
        int   i, free_slot = -1;
        FILE *fp;
        const char *fmode;

        /* Safety: reject absolute paths and directory traversal        */
        if (name[0] == '/' || strstr(name, "..") != NULL) {
                fprintf(stderr, "[RFS] Rejected unsafe path: '%s'\n", name);
                return NULL;
        }

        /* Check cache                                                  */
        for (i = 0; i < MAX_OPEN_FILES; i++) {
                if (open_files[i].name[0] != '\0' &&
                    strcmp(open_files[i].name, name) == 0) {
                        return open_files[i].fp;
                }
                if (open_files[i].name[0] == '\0' && free_slot < 0) {
                        free_slot = i;
                }
        }

        /* Not cached — open the file                                   */
        if (free_slot < 0) {
                fprintf(stderr, "[RFS] No free file slots for '%s'\n", name);
                return NULL;
        }

        /* Determine fopen mode from Xinu mode bits                     */
        if ((mode_bits & F_MODE_W) && (mode_bits & F_MODE_N)) {
                fmode = "w+b";          /* Create/truncate, read+write  */
        } else if ((mode_bits & F_MODE_W) && (mode_bits & F_MODE_R)) {
                fmode = "r+b";          /* Read+write, file must exist  */
        } else if (mode_bits & F_MODE_W) {
                fmode = "r+b";          /* Write (treat as r+b)         */
        } else {
                fmode = "rb";           /* Read only (default)          */
        }

        fp = fopen(name, fmode);
        if (fp == NULL) {
                fprintf(stderr, "[RFS] fopen('%s', '%s') failed: %s\n",
                        name, fmode, strerror(errno));
                return NULL;
        }

        strncpy(open_files[free_slot].name, name, RF_NAMLEN - 1);
        open_files[free_slot].name[RF_NAMLEN - 1] = '\0';
        open_files[free_slot].fp = fp;
        printf("[OPEN] '%s' (mode=0x%x, fmode='%s') cached at slot %d\n",
               name, (unsigned)mode_bits, fmode, free_slot);
        return fp;
}

/* -----------------------------------------------------------------------
 * close_file  —  Remove file from cache and close it
 * ---------------------------------------------------------------------- */
static void close_file(const char *name)
{
        int i;
        for (i = 0; i < MAX_OPEN_FILES; i++) {
                if (open_files[i].name[0] != '\0' &&
                    strcmp(open_files[i].name, name) == 0) {
                        fclose(open_files[i].fp);
                        open_files[i].fp = NULL;
                        open_files[i].name[0] = '\0';
                        printf("[CLOSE] '%s'\n", name);
                        return;
                }
        }
        fprintf(stderr, "[CLOSE] '%s' not found in cache\n", name);
}

/* -----------------------------------------------------------------------
 * fill_hdr  —  Fill common response header fields
 * ---------------------------------------------------------------------- */
static void fill_hdr(struct rf_msg_hdr *resp, uint16_t type,
                     uint16_t status, uint32_t seq, const char *name)
{
        resp->rf_type   = htons(type);
        resp->rf_status = htons(status);
        resp->rf_seq    = htonl(seq);
        memset(resp->rf_name, 0, RF_NAMLEN);
        strncpy(resp->rf_name, name, RF_NAMLEN - 1);
}

/* -----------------------------------------------------------------------
 * handle_open
 * ---------------------------------------------------------------------- */
static int handle_open(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_oreq *r = (const struct rf_msg_oreq *)req;
        struct rf_msg_ores       *s = (struct rf_msg_ores *)resp;
        uint32_t seq      = ntohl(r->rf_seq);
        int32_t  mode     = (int32_t)ntohl((uint32_t)r->rf_mode);
        char     name[RF_NAMLEN + 1];
        FILE    *fp;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        fp = get_file(name, mode);

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_ORES,
                 fp ? RF_STATUS_OK : RF_STATUS_ERR, seq, name);
        s->rf_mode = htonl((uint32_t)mode);
        *resplen = sizeof(struct rf_msg_ores);
        return 0;
}

/* -----------------------------------------------------------------------
 * handle_close
 * ---------------------------------------------------------------------- */
static int handle_close(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_creq *r = (const struct rf_msg_creq *)req;
        struct rf_msg_cres       *s = (struct rf_msg_cres *)resp;
        uint32_t seq = ntohl(r->rf_seq);
        char     name[RF_NAMLEN + 1];

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        close_file(name);

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_CRES,
                 RF_STATUS_OK, seq, name);
        *resplen = sizeof(struct rf_msg_cres);
        return 0;
}

/* -----------------------------------------------------------------------
 * handle_read
 *
 * Xinu passes rf_pos (absolute byte offset) and rf_len (bytes wanted).
 * We seek to rf_pos and read rf_len bytes, returning them in rf_data.
 * ---------------------------------------------------------------------- */
static int handle_read(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_rreq *r = (const struct rf_msg_rreq *)req;
        struct rf_msg_rres       *s = (struct rf_msg_rres *)resp;
        uint32_t seq     = ntohl(r->rf_seq);
        uint32_t pos     = ntohl(r->rf_pos);
        uint32_t req_len = ntohl(r->rf_len);
        char     name[RF_NAMLEN + 1];
        FILE    *fp;
        size_t   nread;
        uint32_t to_read;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        fp = get_file(name, F_MODE_R | F_MODE_O);
        if (fp == NULL) {
                fill_hdr((struct rf_msg_hdr *)s, RF_MSG_RRES,
                         RF_STATUS_ERR, seq, name);
                s->rf_pos = htonl(pos);
                s->rf_len = htonl(0);
                *resplen = (int)(sizeof(struct rf_msg_rres) - RF_DATALEN);
                return 0;
        }

        /* Cap read at RF_DATALEN                                       */
        to_read = (req_len > RF_DATALEN) ? RF_DATALEN : req_len;

        if (fseek(fp, (long)pos, SEEK_SET) != 0) {
                fprintf(stderr, "[READ] fseek('%s', %u) failed: %s\n",
                        name, pos, strerror(errno));
                fill_hdr((struct rf_msg_hdr *)s, RF_MSG_RRES,
                         RF_STATUS_ERR, seq, name);
                s->rf_pos = htonl(pos);
                s->rf_len = htonl(0);
                *resplen = (int)(sizeof(struct rf_msg_rres) - RF_DATALEN);
                return 0;
        }

        nread = fread(s->rf_data, 1, to_read, fp);

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_RRES,
                 RF_STATUS_OK, seq, name);
        s->rf_pos = htonl(pos);
        s->rf_len = htonl((uint32_t)nread);

        /* Only send the header + actual bytes read, not full RF_DATALEN*/
        *resplen = (int)(  sizeof(struct rf_msg_hdr)
                         + sizeof(uint32_t)         /* rf_pos           */
                         + sizeof(uint32_t)         /* rf_len           */
                         + nread);

        if (nread > 0) {
                printf("[READ] '%s' pos=%u len=%u -> %zu bytes\n",
                       name, pos, to_read, nread);
        } else {
                printf("[READ] '%s' pos=%u -> EOF\n", name, pos);
        }

        return 0;
}

/* -----------------------------------------------------------------------
 * handle_write
 * ---------------------------------------------------------------------- */
static int handle_write(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_wreq *r = (const struct rf_msg_wreq *)req;
        struct rf_msg_wres       *s = (struct rf_msg_wres *)resp;
        uint32_t seq     = ntohl(r->rf_seq);
        uint32_t pos     = ntohl(r->rf_pos);
        uint32_t req_len = ntohl(r->rf_len);
        char     name[RF_NAMLEN + 1];
        FILE    *fp;
        size_t   nwritten;
        uint32_t to_write;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        fp = get_file(name, F_MODE_W | F_MODE_O);
        if (fp == NULL) {
                fill_hdr((struct rf_msg_hdr *)s, RF_MSG_WRES,
                         RF_STATUS_ERR, seq, name);
                s->rf_pos = htonl(pos);
                s->rf_len = htonl(0);
                *resplen = sizeof(struct rf_msg_wres);
                return 0;
        }

        to_write = (req_len > RF_DATALEN) ? RF_DATALEN : req_len;

        if (fseek(fp, (long)pos, SEEK_SET) != 0) {
                fill_hdr((struct rf_msg_hdr *)s, RF_MSG_WRES,
                         RF_STATUS_ERR, seq, name);
                s->rf_pos = htonl(pos);
                s->rf_len = htonl(0);
                *resplen = sizeof(struct rf_msg_wres);
                return 0;
        }

        nwritten = fwrite(r->rf_data, 1, to_write, fp);
        fflush(fp);

        printf("[WRITE] '%s' pos=%u len=%u -> %zu bytes\n",
               name, pos, to_write, nwritten);

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_WRES,
                 RF_STATUS_OK, seq, name);
        s->rf_pos = htonl(pos);
        s->rf_len = htonl((uint32_t)nwritten);
        *resplen = sizeof(struct rf_msg_wres);
        return 0;
}

/* -----------------------------------------------------------------------
 * handle_size
 * ---------------------------------------------------------------------- */
static int handle_size(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_sreq *r = (const struct rf_msg_sreq *)req;
        struct rf_msg_sres       *s = (struct rf_msg_sres *)resp;
        uint32_t seq = ntohl(r->rf_seq);
        char     name[RF_NAMLEN + 1];
        FILE    *fp;
        long     size;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        fp = get_file(name, F_MODE_R | F_MODE_O);
        if (fp == NULL) {
                fill_hdr((struct rf_msg_hdr *)s, RF_MSG_SRES,
                         RF_STATUS_ERR, seq, name);
                s->rf_size = htonl(0);
                *resplen = sizeof(struct rf_msg_sres);
                return 0;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);

        printf("[SIZE] '%s' = %ld bytes\n", name, size);

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_SRES,
                 RF_STATUS_OK, seq, name);
        s->rf_size = htonl((uint32_t)size);
        *resplen = sizeof(struct rf_msg_sres);
        return 0;
}

/* -----------------------------------------------------------------------
 * handle_delete
 * ---------------------------------------------------------------------- */
static int handle_delete(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_dreq *r = (const struct rf_msg_dreq *)req;
        struct rf_msg_dres       *s = (struct rf_msg_dres *)resp;
        uint32_t seq = ntohl(r->rf_seq);
        char     name[RF_NAMLEN + 1];
        int      rc;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        /* Close if cached before deleting                              */
        close_file(name);
        rc = remove(name);

        printf("[DELETE] '%s' -> %s\n", name, rc == 0 ? "OK" : "ERR");

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_DRES,
                 rc == 0 ? RF_STATUS_OK : RF_STATUS_ERR, seq, name);
        *resplen = sizeof(struct rf_msg_dres);
        return 0;
}

/* -----------------------------------------------------------------------
 * handle_truncate
 * ---------------------------------------------------------------------- */
static int handle_truncate(const uint8_t *req, uint8_t *resp, int *resplen)
{
        const struct rf_msg_treq *r = (const struct rf_msg_treq *)req;
        struct rf_msg_tres       *s = (struct rf_msg_tres *)resp;
        uint32_t seq = ntohl(r->rf_seq);
        char     name[RF_NAMLEN + 1];
        FILE    *fp;

        memset(name, 0, sizeof(name));
        memcpy(name, r->rf_name, RF_NAMLEN);

        /* Truncate by closing cached handle and reopening with "wb"    */
        close_file(name);
        fp = fopen(name, "wb");
        if (fp != NULL) fclose(fp);

        printf("[TRUNC] '%s' -> %s\n", name, fp ? "OK" : "ERR");

        fill_hdr((struct rf_msg_hdr *)s, RF_MSG_TRES,
                 fp ? RF_STATUS_OK : RF_STATUS_ERR, seq, name);
        *resplen = sizeof(struct rf_msg_tres);
        return 0;
}

/* -----------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
        int                sock;
        struct sockaddr_in addr, client;
        socklen_t          client_len;
        uint8_t            reqbuf[sizeof(struct rf_msg_wreq)];
        uint8_t            respbuf[sizeof(struct rf_msg_rres)];
        int                reqlen, resplen;
        struct rf_msg_hdr *hdr;
        uint16_t           msg_type;
        int                reuse = 1;
        char               cwd[512];

        signal(SIGINT,  handle_sigint);
        signal(SIGTERM, handle_sigint);

        memset(open_files, 0, sizeof(open_files));

        if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("[RFS] Serving files from: %s\n", cwd);

        printf("[RFS] Listening on UDP port %d\n", RF_SERVER_PORT);
        printf("[RFS] RF_NAMLEN=%d  RF_DATALEN=%d\n",
               RF_NAMLEN, RF_DATALEN);
        printf("[RFS] Largest message (rres): %zu bytes\n",
               sizeof(struct rf_msg_rres));
        printf("[RFS] Waiting for Xinu...\n\n");

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) { perror("socket"); return 1; }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(RF_SERVER_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                perror("bind");
                close(sock);
                return 1;
        }

        while (g_running) {

                client_len = sizeof(client);
                reqlen = recvfrom(sock, reqbuf, sizeof(reqbuf), 0,
                                  (struct sockaddr *)&client, &client_len);
                if (reqlen < 0) {
                        if (errno == EINTR) break;
                        perror("recvfrom");
                        continue;
                }

                if (reqlen < (int)sizeof(struct rf_msg_hdr)) {
                        fprintf(stderr, "[RFS] Runt packet (%d bytes) "
                                "from %s — ignored\n",
                                reqlen, inet_ntoa(client.sin_addr));
                        continue;
                }

                hdr      = (struct rf_msg_hdr *)reqbuf;
                msg_type = ntohs(hdr->rf_type);
                resplen  = 0;

                switch (msg_type) {
                case RF_MSG_OREQ:  handle_open     (reqbuf,respbuf,&resplen); break;
                case RF_MSG_CREQ:  handle_close    (reqbuf,respbuf,&resplen); break;
                case RF_MSG_RREQ:  handle_read     (reqbuf,respbuf,&resplen); break;
                case RF_MSG_WREQ:  handle_write    (reqbuf,respbuf,&resplen); break;
                case RF_MSG_SREQ:  handle_size     (reqbuf,respbuf,&resplen); break;
                case RF_MSG_DREQ:  handle_delete   (reqbuf,respbuf,&resplen); break;
                case RF_MSG_TREQ:  handle_truncate (reqbuf,respbuf,&resplen); break;
                case RF_MSG_MREQ:  /* mkdir: not supported, return error */
                case RF_MSG_XREQ:  /* rmdir: not supported, return error */
                        fill_hdr((struct rf_msg_hdr *)respbuf,
                                 msg_type | RF_MSG_RESPONSE,
                                 RF_STATUS_ERR,
                                 ntohl(hdr->rf_seq),
                                 hdr->rf_name);
                        resplen = sizeof(struct rf_msg_hdr);
                        break;
                default:
                        fprintf(stderr, "[RFS] Unknown type 0x%04x "
                                "from %s — ignored\n",
                                msg_type, inet_ntoa(client.sin_addr));
                        continue;
                }

                if (resplen > 0) {
                        sendto(sock, respbuf, (size_t)resplen, 0,
                               (struct sockaddr *)&client, client_len);
                }
        }

        printf("\n[RFS] Shutting down.\n");
        {
                int i;
                for (i = 0; i < MAX_OPEN_FILES; i++)
                        if (open_files[i].fp != NULL)
                                fclose(open_files[i].fp);
        }
        close(sock);
        return 0;
}

