#include <xinu.h>
#include <stdarg.h>
//
// // Xinu external functions (assumed)
// extern void* getmem(uint32_t nbytes);
// extern int32_t freemem(void* pmem, uint32_t nbytes);
// #define SYSERR (-1)
//
// // --- Memory Shims ---
//
void* malloc(size_t size) {
    if (size == 0) return NULL;
    // We need to store the size to use with freemem later
    uint32_t total_size = size + sizeof(uint32_t);
    uint32_t* ptr = (uint32_t*)getmem(total_size);
    if ((intptr_t)ptr == SYSERR) return NULL;
    *ptr = total_size;
    return (void*)(ptr + 1);
}

void free(void* ptr) {
    if (ptr == NULL) return;
    uint32_t* size_ptr = (uint32_t*)ptr - 1;
    freemem((void*)size_ptr, *size_ptr);
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (ptr == NULL) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    uint32_t old_total_size = *((uint32_t*)ptr - 1);
    uint32_t old_size = old_total_size - sizeof(uint32_t);
    if (size <= old_size) return ptr;

    void* new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size);
        free(ptr);
    }
    return new_ptr;
}

void* memmove(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

//
// // --- String Shims ---
//
char *strdup(const char *s)
{
    size_t len;
    char *copy;

    if (s == NULL)
        return NULL;

    len = strlen(s) + 1;        /* +1 for the null terminator */
    copy = (char *)malloc(len);

    if (copy == NULL)
        return NULL;

    memcpy(copy, s, len);       /* Copy including the null terminator */
    return copy;
}

int strcmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 && (*p1 == *p2))
    {
        p1++;
        p2++;
    }

    return *p1 - *p2;
}

int isspace(int c)
{
    return (c == ' '  ||
            c == '\t' ||
            c == '\n' ||
            c == '\v' ||
            c == '\f' ||
            c == '\r');
}

int snprintf(
      char         *str,        /* output buffer                */
      size_t        size,       /* max bytes including '\0'     */
      const char   *fmt,        /* format string                */
      ...
    )
{
    va_list ap;
    snprntf_state state;

    state.ptr   = str;
    state.end   = (size > 0) ? (str + size - 1) : str;  /* reserve 1 byte for '\0' */
    state.count = 0;

    va_start(ap, fmt);
    _fdoprnt((char *)fmt, ap, snprntf, (int)&state);
    va_end(ap);

    /* Always null-terminate as long as size > 0 */
    if (size > 0)
        *state.end = '\0';      /* safe sentinel in case we hit the limit */
    if (state.ptr < state.end)
        *state.ptr = '\0';      /* normal case: terminate at current position */

    return state.count;         /* return total that would have been written */
}

static int snprntf(
             int    astate,     /* pointer to snprntf_state, passed as int  */
             int    ac          /* character to write                        */
           )
{
    snprntf_state *state = (snprntf_state *)astate;
    char c = (char)ac;

    state->count++;

    if (state->ptr < state->end)
        *state->ptr++ = c;

    return c;
}

/*------------------------------------------------------------------------
 *  vsnprintf  -  Format arguments and place output in a bounded string.
 *                Returns the number of characters that would have
 *                been written had size been unlimited (not counting
 *                the null byte) -- the standard C99 contract.
 *------------------------------------------------------------------------
 */
int     vsnprintf(
          char          *str,           /* output buffer                */
          size_t        size,           /* size of output buffer, bytes */
          char          *fmt,           /* format string                */
          va_list       ap              /* caller's argument list       */
        )
{
    struct vsnf_ctx ctx;

    ctx.bufp  = str;
    ctx.left  = (size > 0) ? (size - 1) : 0;
    ctx.total = 0;

    _fdoprnt(fmt, ap, vsnprntf, (int)&ctx);

    if (size > 0)
    {
        *ctx.bufp = '\0';
    }

    return ((int)ctx.total);
}

/*------------------------------------------------------------------------
 *  vsnprntf  -  Routine called by _fdoprnt to handle each character.
 *               Only writes while room remains, but always counts,
 *               so the truncated-length return value stays correct.
 *------------------------------------------------------------------------
 */
static int      vsnprntf(
                  int           actx,
                  int           ac
                )
{
    struct vsnf_ctx *ctx = (struct vsnf_ctx *)actx;
    char c = (char)ac;

    ctx->total++;
    if (ctx->left > 0)
    {
        *ctx->bufp++ = c;
        ctx->left--;
    }
    return ((int)c);
}

void puts(const char *msg)
{
  char *ptr = msg;
  while (ptr != '\0')
    putchar(*(ptr++));
}

int toupper(int ch)
{
  if (c >= 'a' && c <= 'z')
    return c - ('a' - 'A');
  return c;
}

float my_atof(const char *str)
{
    float result = 0.0;
    float sign = 1.0;

    /* Skip leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    /* Handle optional sign */
    if (*str == '-') {
        sign = -1.0;
        str++;
    } else if (*str == '+') {
        str++;
    }

    /* Parse integer part */
    while (isdigit((unsigned char)*str)) {
        result = result * 10.0 + (*str - '0');
        str++;
    }

    /* Parse fractional part */
    if (*str == '.') {
        float frac = 0.0;
        float scale = 1.0;
        str++;
        while (isdigit((unsigned char)*str)) {
            frac = frac * 10.0 + (*str - '0');
            scale *= 10.0;
            str++;
        }
        result += frac / scale;
    }

    result *= sign;

    /* Parse exponent part (e.g. 1.5e10) */
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        int exponent = 0;

        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        while (isdigit((unsigned char)*str)) {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        float factor = 1.0;
        for (int i = 0; i < exponent; i++) {
            factor *= 10.0;
        }

        if (exp_sign == 1) {
            result *= factor;
        } else {
            result /= factor;
        }
    }

    return result;
}

//
// // --- IO Shims ---
//
int system(const char *s)
{
  kprintf("system() not supported\n");
  return 0;
}

int fflush(int fs)
{
  kprintf("fflush() not supported\n");
}


///*------------------------------------------------------------------------
// * doom_fopen - open a file on the Xinu remote filesystem
// *
// * Returns a FILE (did32) on success, DOOM_INVALID_FILE on failure.
// *------------------------------------------------------------------------
// */
//FILE doom_fopen(const char *path, const char *mode)
//{
//    char   xinu_mode[4];
//    did32  fd;
//
//    /* Map C mode to Xinu RFS mode string, stripping 'b' (binary) flag */
//    if (mode[0] == 'r' && mode[1] == '+') {
//        xinu_mode[0] = 'r'; xinu_mode[1] = '+'; xinu_mode[2] = '\0';
//    } else if (mode[0] == 'w') {
//        xinu_mode[0] = 'w'; xinu_mode[1] = '\0';
//    } else if (mode[0] == 'r') {
//        xinu_mode[0] = 'r'; xinu_mode[1] = '\0';
//    } else {
//        return DOOM_INVALID_FILE;
//    }
//
//    fd = open(RFILESYS, (char *)path, xinu_mode);
//    return (fd == SYSERR) ? DOOM_INVALID_FILE : (FILE)fd;
//}
//
///*------------------------------------------------------------------------
// * doom_ftell - return the current byte offset in an open file
// *
// * Xinu has no native ftell. We use control() on the rfl device to
// * query the current position via RFS_CTL_GETPOS if your Xinu version
// * supports it, otherwise we track it via a small static table keyed on
// * the device descriptor.
// *------------------------------------------------------------------------
// */
//
//#define FTELL_MAX_FDS 16
//
//static long ftell_pos[FTELL_MAX_FDS];
//
//long doom_ftell(FILE *fp)
//{
//    if (*fp < 0 || *fp >= FTELL_MAX_FDS)
//        return -1L;
//    return ftell_pos[(int)(*fp)];
//}
//
///*
// * doom_ftell_update - called internally after any read/write to keep
// * the position table current. Wire this into your fread/fwrite shims
// * if you need ftell accuracy after reads, e.g.:
// *
// *   n = read(fp, buf, len);
// *   doom_ftell_update(fp, n);
// */
//void doom_ftell_update(FILE fp, int32 delta)
//{
//    if (fp >= 0 && fp < FTELL_MAX_FDS)
//        ftell_pos[(int)fp] += delta;
//}
//
//#define RENAME_BUF_SIZE 512
//
///*
// * doom_remove - implemented via rfsControl if your Xinu version wires
// * RFS_CTL_UNLINK, otherwise falls back to opening and truncating.
// *
// * Check your rfsControl.c: if it handles a delete control code, use:
// *   control(RFILESYS, RFS_CTL_UNLINK, (int32)path, 0)
// *
// * If not wired, the best client-only option is to open the file for
// * write (which truncates it to zero on the server) and close it,
// * leaving a zero-byte file. This is the safest no-server fallback.
// */
//int doom_remove(const char *path)
//{
//    FILE fd;
//
//    /* Attempt via control() first — works if rfserver supports it */
//    if (control(RFILESYS, RFS_CTL_UNLINK, (int32)path, 0) != SYSERR)
//        return 0;
//
//    /*
//     * Fallback: truncate to zero. Not a true delete, but prevents
//     * Doom from reading stale save data on the next load.
//     */
//    fd = open(RFILESYS, (char *)path, "w");
//    if (fd == SYSERR)
//        return -1;
//    close(fd);
//    return 0;
//}
//
///*
// * doom_rename - copy oldpath to newpath byte-by-byte, then remove oldpath.
// *
// * This is not atomic: a crash between the write and remove leaves both
// * files. Acceptable for Doom save games — the worst case is a duplicate.
// */
//int doom_rename(const char *oldpath, const char *newpath)
//{
//    FILE   src, dst;
//    char   buf[RENAME_BUF_SIZE];
//    int32  n;
//    int    ret = 0;
//
//    src = open(RFILESYS, (char *)oldpath, "r");
//    if (src == SYSERR)
//        return -1;
//
//    dst = open(RFILESYS, (char *)newpath, "w");
//    if (dst == SYSERR) {
//        close(src);
//        return -1;
//    }
//
//    /* Copy contents */
//    while ((n = read(src, buf, sizeof(buf))) > 0) {
//        if (write(dst, buf, n) != n) {
//            ret = -1;
//            break;
//        }
//    }
//
//    close(src);
//    close(dst);
//
//    if (ret == 0)
//        remove(oldpath);    /* best-effort; ignore failure */
//
//    return ret;
//}
//
//void mkdir(const char *s)
//{
//  kprintf("mkdir() not supported\n");
//}

//
// // --- String Shims ---
//
// void* memmove(void* dest, const void* src, size_t n) {
//     char* d = (char*)dest;
//     const char* s = (const char*)src;
//     if (d < s) {
//         while (n--) *d++ = *s++;
//     } else {
//         d += n;
//         s += n;
//         while (n--) *--d = *--s;
//     }
//     return dest;
// }
//
// static inline int tolower(int c) {
//     if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
//     return c;
// }
//
// int strcasecmp(const char* s1, const char* s2) {
//     while (*s1 && (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))) {
//         s1++;
//         s2++;
//     }
//     return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
// }
//
// int strncasecmp(const char* s1, const char* s2, size_t n) {
//     if (n == 0) return 0;
//     while (n-- > 0 && *s1 && (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))) {
//         if (n == 0) return 0;
//         s1++;
//         s2++;
//     }
//     return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
// }
//
// // --- File I/O Shims ---
// // Assuming Xinu remote filesystem uses open/read/write/close/seek on a specific device.
//
// extern int32_t open(int32_t dev, const char* name, const char* mode);
// extern int32_t close(int32_t dev);
// extern int32_t read(int32_t dev, void* buf, uint32_t len);
// extern int32_t write(int32_t dev, const void* buf, uint32_t len);
// extern int32_t seek(int32_t dev, uint32_t pos);
//
// #define RFILESYS 0 // Placeholder for remote filesystem device ID
//
// FILE* fopen(const char* filename, const char* mode) {
//     int32_t fd = open(RFILESYS, filename, mode);
//     if (fd == SYSERR) return NULL;
//     return (FILE*)(intptr_t)fd;
// }
//
// int fclose(FILE* stream) {
//     return close((int32_t)(intptr_t)stream);
// }
//
// size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
//     int32_t res = read((int32_t)(intptr_t)stream, ptr, size * nmemb);
//     if (res == SYSERR) return 0;
//     return res / size;
// }
//
// size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
//     int32_t res = write((int32_t)(intptr_t)stream, ptr, size * nmemb);
//     if (res == SYSERR) return 0;
//     return res / size;
// }
//
// int fseek(FILE* stream, long offset, int whence) {
//     if (whence == SEEK_SET) {
//         return (seek((int32_t)(intptr_t)stream, offset) == SYSERR) ? -1 : 0;
//     }
//     return -1;
// }
//
// long ftell(FILE* stream) {
//     return -1;
// }
//
// // --- Process ---
// extern void kill(int32_t pid);
// extern int32_t getpid(void);
//
// void exit(int status) {
//     kill(getpid());
// }
//
// double fabs(double x) {
//     return (x < 0) ? -x : x;
// }
//
// int errno = 0;
//
// // --- Time ---
// extern uint32_t clktime;
// extern void sleepms(uint32_t ms);
//
// int gettimeofday(struct timeval* tv, struct timezone* tz) {
//     if (tv) {
//         tv->tv_sec = clktime;
//         tv->tv_usec = 0;
//     }
//     return 0;
// }
//
// unsigned int usleep(unsigned int useconds) {
//     sleepms(useconds / 1000);
//     return 0;
// }
