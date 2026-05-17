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

static XINU_FILE file_pool[DOOM_MAX_FILES];
static int       file_pool_init = 0;

/*------------------------------------------------------------------------
 * pool_init - zero the file pool on first use
 *------------------------------------------------------------------------
 */
static void pool_init(void)
{
    int i;
    for (i = 0; i < DOOM_MAX_FILES; i++)
        file_pool[i].fd = SYSERR;
    file_pool_init = 1;
}

/*------------------------------------------------------------------------
 * pool_alloc - find a free slot in the file pool
 *------------------------------------------------------------------------
 */
static XINU_FILE *pool_alloc(void)
{
    int i;
    if (!file_pool_init)
        pool_init();
    for (i = 0; i < DOOM_MAX_FILES; i++) {
        if (file_pool[i].fd == SYSERR)
            return &file_pool[i];
    }
    return NULL;
}

/*------------------------------------------------------------------------
 * fopen  -  Open a remote file via the Xinu RFS
 *
 * @path:  Filename on the remote server. No leading '/' or '..' allowed.
 * @mode:  Standard mode string. Supported:
 *           "r"  / "rb"  - read-only,  file must exist
 *           "w"  / "wb"  - write-only, create or truncate
 *           "r+" / "rb+" - read-write, file must exist
 *
 * Returns a pointer to an XINU_FILE on success, NULL on failure.
 *
 * Internally calls Xinu's open(RFILESYS, path, xinu_mode) which routes
 * through rfsOpen() on the rfs master device and returns a did32
 * descriptor bound to an rfl pseudo-device slot.
 *------------------------------------------------------------------------
 */
XINU_FILE *fopen(const char *path, const char *mode)
{
    XINU_FILE  *fp;
    did32       fd;
    char        xinu_mode[4];  /* Xinu mode string: "r", "w", "r+" etc. */

    /* Map C mode string to Xinu RFS mode string.
     * Xinu's rfsOpen recognises: "r" (read, must exist),
     *                             "w" (write, create/truncate),
     *                             "r+" (read-write, must exist).
     * Binary flag ('b') is irrelevant on a remote text server; strip it. */
    if (mode[0] == 'r' && mode[1] == '+') {
        xinu_mode[0] = 'r'; xinu_mode[1] = '+'; xinu_mode[2] = '\0';
    } else if (mode[0] == 'w') {
        xinu_mode[0] = 'w'; xinu_mode[1] = '\0';
    } else if (mode[0] == 'r') {
        xinu_mode[0] = 'r'; xinu_mode[1] = '\0';
    } else {
        /* Unsupported mode (append, etc.) */
        return NULL;
    }

    fp = pool_alloc();
    if (fp == NULL)
        return NULL;    /* file pool exhausted */

    fd = open(RFILESYS, (char *)path, xinu_mode);
    if (fd == SYSERR)
        return NULL;

    fp->fd    = fd;
    fp->pos   = 0;
    fp->error = 0;
    return fp;
}

/*------------------------------------------------------------------------
 * fclose  -  Close a remote file
 *
 * Flushes any pending state and releases the rfl pseudo-device slot
 * back to the RFS via close(), which dispatches to rflClose().
 *
 * Returns 0 on success, EOF (-1) on error.
 *------------------------------------------------------------------------
 */
int fclose(XINU_FILE *fp)
{
    int ret;

    if (fp == NULL || fp->fd == SYSERR)
        return -1;  /* EOF */

    ret = close(fp->fd);

    /* Return the pool slot regardless of close result */
    fp->fd    = SYSERR;
    fp->pos   = 0;
    fp->error = 0;

    return (ret == SYSERR) ? -1 : 0;
}

/*------------------------------------------------------------------------
 * ftell  -  Return the current byte offset in the file
 *
 * Xinu has no ftell equivalent in the rfl layer; we maintain the
 * position ourselves in XINU_FILE.pos, updated after every read/write.
 *
 * Returns the current offset, or -1L on error.
 *------------------------------------------------------------------------
 */
long ftell(XINU_FILE *fp)
{
    if (fp == NULL || fp->fd == SYSERR)
        return -1L;
    return (long)fp->pos;
}

/*------------------------------------------------------------------------
 * fprintf  -  Format and write a string to a remote file
 *
 * Formats into a temporary stack buffer with snprintf (the shim we
 * already implemented), then writes it out via write(), which dispatches
 * through rflWrite().
 *
 * Returns the number of bytes written, or -1 on error.
 *------------------------------------------------------------------------
 */
int fprintf(XINU_FILE *fp, const char *fmt, ...)
{
    char    buf[FPRINTF_BUF_SIZE];
    va_list ap;
    int     len;
    int     written;

    if (fp == NULL || fp->fd == SYSERR)
        return -1;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len <= 0)
        return len;

    /* Clamp to buffer in case the format was truncated */
    if (len >= FPRINTF_BUF_SIZE)
        len = FPRINTF_BUF_SIZE - 1;

    written = write(fp->fd, buf, len);
    if (written == SYSERR) {
        fp->error = 1;
        return -1;
    }

    fp->pos += written;
    return written;
}

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
