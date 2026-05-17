#ifndef XINU_SHIM_H
#define XINU_SHIMS_H

void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);

char* strdup(const char *s);
int strcmp(const char *s1, const char *s2);
int isspace(int c);
int snprntf(int astate, int ac);
int snprintf(char *str, size_t size, const char *fmt, ...);

typedef struct {
    char   *ptr;        /* current write position           */
    char   *end;        /* one past the last writable byte  */
    int     count;      /* total chars that would be written */
} snprntf_state;

typedef struct xinu_file {
    did32   fd;         /* Xinu device descriptor from open()   */
    int32   pos;        /* current byte offset into the file    */
    int32   error;      /* non-zero if an error has occurred    */
} XINU_FILE;

#define FILE XINU_FILE
#define stderr 

#define DOOM_MAX_FILES  8
#define FPRINTF_BUF_SIZE 1024

int *fopen(const char *path, const char *mode);
int fclose(int fp);
long ftell(int fp);

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
