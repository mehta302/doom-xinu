#ifndef XINU_SHIM_H
#define XINU_SHIMS_H

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void* memmove(void* dest, const void* src, size_t n);

char* strdup(const char *s);
int snprntf(int astate, int ac);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsnprntf(int actx, int ac);
int vsnprintf(char *str, size_t size, char *fmt, va_list ap);
void puts(const char *msg);
float atof(const char *value);
float fabs(float x);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

typedef struct {
    char   *ptr;        /* current write position           */
    char   *end;        /* one past the last writable byte  */
    int     count;      /* total chars that would be written */
} snprntf_state;

struct vsnf_ctx
{
    char    *bufp;    /* next slot to fill, or (once out of room) the */
                       /* slot the terminating '\0' will land in      */
    size_t  left;      /* real characters still fitting (excludes the */
                        /* one byte reserved for the terminator)      */
    size_t  total;      /* count of ALL characters the format would   */
                         /* produce, truncated or not (return value)  */
};

int system(const char *s);
int fflush(int fs);

//typedef int32 FILE;
//#define DOOM_INVALID_FILE ((FILE)-1)
//#define FTELL_MAX_FDS 16
//
//FILE doom_fopen(const char *path, const char *mode);
//#define fopen(path, mode) doom_fopen(path, mode)
//long doom_ftell(FILE *fp);
//#define ftell(fp) doom_ftell(fp)
//#define fclose(fp) close((did32)(fp))
//#define fread(buf, sz, count, fp)  read((did32)(fp),  (char*)(buf), (int32)((sz)*(count)))
//#define fwrite(buf, sz, count, fp) write((did32)(fp), (char*)(buf), (int32)((sz)*(count)))
//#define SEEK_SET 0
//#define SEEK_CUR 1
//#define SEEK_END 2
//#define fseek(fp, offset, whence) seek((did32)(fp), (uint32)(offset))
//int doom_remove(const char *path);
//#define remove(path) doom_remove(path)
//int doom_rename(const char *oldpath, const char *newpath);
//#define rename(oldpath, newpath) doom_rename(oldpath, newpath)
//void doom_fflush(FILE fp);
//#define fflush(fp) doom_fflush(fp)
//void mkdir(const char *s);


#endif // XINU_SHIMS_H*/

/*#ifndef XINU_SHIMS_H
#define XINU_SHIMS_H

#include <stdint.h>
#include <stddef.h>

// Memory allocation
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

// String/Memory functions missing from lib.md
void* memmove(void* dest, const void* src, size_t n);
int strcasecmp(const char* s1, const char* s2);
int strncasecmp(const char* s1, const char* s2, size_t n);

// File I/O (to be implemented via Xinu remote filesystem)
// Assuming FILE is already defined in Xinu if fprintf is available.
// If not, we define it here as a placeholder.
#ifndef _FILE_DEFINED
typedef void FILE;
#define _FILE_DEFINED
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE* fopen(const char* filename, const char* mode);
int fclose(int stream);
size_t fread(void* ptr, size_t size, size_t nmemb, int stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, int stream);
int fseek(int stream, long offset, int whence);
long ftell(int stream);

double fabs(double x);

// Errno
extern int errno;
#define EISDIR 21

// Time
struct timeval {
    long tv_sec;
    long tv_usec;
};
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
int gettimeofday(struct timeval* tv, struct timezone* tz);
unsigned int usleep(unsigned int useconds);

#endif // XINU_SHIMS_H*/
