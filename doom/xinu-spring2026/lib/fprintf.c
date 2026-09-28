/* fprintf.c - _fdoprnt */

#include <xinu.h>
#include <stdarg.h>

extern void _fdoprnt(char *, va_list, int (*)(did32, char), int);

//int fprntf(int dev, int c)
//{
//  return putc((did32) fd, (char) c);
//}
//
///*------------------------------------------------------------------------
// * vfprintf - write formatted output to a Xinu device using a va_list
// *
// * Standard prototype:
// *   int vfprintf(int fd, const char *fmt, va_list ap);
// *
// * In this port FILE is typedef'd to int32 (a Xinu did32), so 'fd' is
// * directly a device descriptor — exactly what _fdoprnt and putc expect.
// *
// * This is the correct base layer for the printf family in this port:
// *
// *   vfprintf(fd, fmt, ap)        <-- this function, calls _fdoprnt
// *       ^
// *   fprintf(fd, fmt, ...)        <-- already in Xinu; ideally rebuilt
// *       ^                             on top of vfprintf (see note)
// *   printf(fmt, ...)             <-- calls fprintf(CONSOLE, fmt, ...)
// *
// * Returns the number of characters written, or -1 on error.
// *------------------------------------------------------------------------
// */
//int vfprintf(int fd, const char *fmt, va_list ap)
//{
//    if (fmt == NULL)
//        return -1;
//
//    /*
//     * _fdoprnt drives the format loop, calling fprntf(fd, c) for each
//     * output character. The cast of fd to int is safe: did32 is int32,
//     * and _fdoprnt's opaque 'arg' parameter is typed as int throughout
//     * Xinu's source — this is the same cast Xinu uses internally.
//     */
//    _fdoprnt((char *)fmt, ap, fprntf, (int)fd);
//
//    /*
//     * _fdoprnt has no return value. We cannot count characters without
//     * wrapping the callback, which would add overhead for every call.
//     * Returning 0 on success is acceptable: Doom only checks vfprintf's
//     * return value for error (< 0), not for the exact byte count.
//     */
//    return 0;
//}

/*------------------------------------------------------------------------
 *  fprintf  -  Print a formatted message on specified device (file).
 *			Return 0 if the output was printed successfully,
 *			and -1 if an error occurred.
 *------------------------------------------------------------------------
 */
int	fprintf(
	  int		dev,		/* device to write to		*/
	  char		*fmt,		/* format string		*/
	  ...
	)
{
    va_list ap;
    int putc(did32, char);

    va_start(ap, fmt);
    _fdoprnt(fmt, ap, putc, dev);
    va_end(ap);

    return 0;
}
