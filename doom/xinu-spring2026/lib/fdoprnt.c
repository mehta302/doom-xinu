/* fdoprnt.c - _fdoprnt, _prtu, _prtf */

#include <stdarg.h>

#define MAXSTR  80
#ifndef NULL
#define NULL    0
#endif

/*
 * Define FDOPRNT_FLOAT (e.g. CFLAGS += -DFDOPRNT_FLOAT) to enable %f / %F.
 * Off by default: it needs a working FPU (or soft-float + libgcc) in the
 * kernel, and DOOM itself is fixed-point.  Without it, %f prints literally.
 */

typedef unsigned long long      ullong;

/* Length modifiers */
#define LM_NONE 0               /* (none)                               */
#define LM_HH   1               /* hh      char                         */
#define LM_H    2               /* h       short                        */
#define LM_L    3               /* l z t   long / size_t / ptrdiff_t    */
#define LM_LL   4               /* ll j    long long / intmax_t         */

static int _prtu(ullong num, int base, int upper, char *str);
#ifdef FDOPRNT_FLOAT
static int _prtf(double val, int prec, int alt, char *str);
#endif

/*------------------------------------------------------------------------
 *  _fdoprnt  -  Format and write output using 'func' to write characters.
 *
 *      Conversions:  c s d i u o x X b p            (f F if FDOPRNT_FLOAT)
 *      Flags:        -  0  +  <space>  #            (any order)
 *      Width:        N or *
 *      Precision:    .N or .*   (min digits for integers, max chars for s)
 *      Length:       hh h l ll z t j
 *
 *      Widths/precisions larger than MAXSTR are ignored.
 *      Unknown conversions are echoed literally as "%<char>".
 *------------------------------------------------------------------------
 */
void    _fdoprnt(
          char          *fmt,                   /* format string        */
          va_list       ap,                     /* ap list of values    */
          int           (*func)(int, int),      /* char output function */
          int           farg                    /* arg for char output  */
        )
{
    int c;
    int i;
    int f;                      /* The format character (comes after %) */
    char *str;                  /* Points at the text to be output      */
    char string[MAXSTR + 24];   /* Scratch for converted numbers        */

    int length;                 /* Length of string "str"               */
    char fill;                  /* Fill character (' ' or '0')          */
    int leftjust;               /* 0 = right-justified, else left-just  */
    int plus, space, alt;       /* '+', ' ' and '#' flags               */
    int fmin;                   /* Minimum field width                  */
    int prec, hasprec;          /* Precision, and whether one was given */
    int lmod;                   /* Length modifier (LM_*)               */
    int isnum;                  /* Numeric conversion (precision=digits)*/
    int zeros;                  /* Leading zeros demanded by precision  */
    int pad;                    /* Number of fill characters to emit    */
    char pfx[3];                /* Sign and/or "0x" style prefix        */
    int pfxlen;
    int base;
    long long sv;
    ullong uv;

    for (;;)
    {
        /* Echo characters until '%' or end of fmt string */
        while ((c = *fmt++) != '%')
        {
            if (c == '\0')
            {
                return;
            }
            (*func) (farg, c);
        }
        /* Echo "...%%..." as '%' */
        if (*fmt == '%')
        {
            (*func) (farg, *fmt++);
            continue;
        }

        /* Flags, in any order */
        leftjust = plus = space = alt = 0;
        fill = ' ';
        for (;; fmt++)
        {
            if (*fmt == '-')
            {
                leftjust = 1;
            }
            else if (*fmt == '0')
            {
                fill = '0';
            }
            else if (*fmt == '+')
            {
                plus = 1;
            }
            else if (*fmt == ' ')
            {
                space = 1;
            }
            else if (*fmt == '#')
            {
                alt = 1;
            }
            else
            {
                break;
            }
        }

        /* Minimum field width; "*" takes it from the arguments */
        fmin = 0;
        if (*fmt == '*')
        {
            fmin = va_arg(ap, int);
            ++fmt;
            if (fmin < 0)               /* negative width means "-" */
            {
                leftjust = 1;
                fmin = -fmin;
            }
        }
        else
        {
            while ('0' <= *fmt && *fmt <= '9')
            {
                fmin = fmin * 10 + *fmt++ - '0';
            }
        }

        /* Precision */
        prec = 0;
        hasprec = 0;
        if (*fmt == '.')
        {
            hasprec = 1;
            if (*(++fmt) == '*')
            {
                prec = va_arg(ap, int);
                ++fmt;
            }
            else
            {
                while ('0' <= *fmt && *fmt <= '9')
                {
                    prec = prec * 10 + *fmt++ - '0';
                }
            }
            if (prec < 0 || prec > MAXSTR)
            {
                hasprec = 0;
            }
        }

        /* Length modifiers */
        lmod = LM_NONE;
        for (;; fmt++)
        {
            if (*fmt == 'h')
            {
                lmod = (lmod == LM_H) ? LM_HH : LM_H;
            }
            else if (*fmt == 'l')
            {
                lmod = (lmod == LM_L) ? LM_LL : LM_L;
            }
            else if (*fmt == 'z' || *fmt == 't')
            {
                lmod = LM_L;
            }
            else if (*fmt == 'j')
            {
                lmod = LM_LL;
            }
            else
            {
                break;
            }
        }

        if ((f = *fmt++) == '\0')
        {
            (*func) (farg, '%');
            return;
        }

        str = string;
        length = 0;
        zeros = 0;
        pfxlen = 0;
        isnum = 0;

        switch (f)
        {
        case 'c':
            string[0] = va_arg(ap, int);
            length = 1;
            fill = ' ';
            break;

        case 's':
            str = va_arg(ap, char *);

            if (NULL == str)
            {
                str = "(null)";
            }
            while (str[length] != '\0' && (!hasprec || length < prec))
            {
                length++;
            }
            fill = ' ';
            break;

        case 'd':
        case 'i':
            switch (lmod)
            {
            case LM_HH: sv = (signed char) va_arg(ap, int); break;
            case LM_H:  sv = (short) va_arg(ap, int);       break;
            case LM_L:  sv = va_arg(ap, long);              break;
            case LM_LL: sv = va_arg(ap, long long);         break;
            default:    sv = va_arg(ap, int);               break;
            }
            if (sv < 0)
            {
                pfx[pfxlen++] = '-';
                uv = 0ULL - (ullong) sv;
            }
            else
            {
                uv = (ullong) sv;
                if (plus)
                {
                    pfx[pfxlen++] = '+';
                }
                else if (space)
                {
                    pfx[pfxlen++] = ' ';
                }
            }
            length = _prtu(uv, 10, 0, str);
            isnum = 1;
            break;

        case 'u':
        case 'o':
        case 'x':
        case 'X':
        case 'b':
            switch (lmod)
            {
            case LM_HH: uv = (unsigned char) va_arg(ap, int);  break;
            case LM_H:  uv = (unsigned short) va_arg(ap, int); break;
            case LM_L:  uv = va_arg(ap, unsigned long);        break;
            case LM_LL: uv = va_arg(ap, ullong);               break;
            default:    uv = va_arg(ap, unsigned int);         break;
            }
            base = (f == 'u') ? 10 : (f == 'o') ? 8 : (f == 'b') ? 2 : 16;
            if (alt && uv != 0)
            {
                if (f == 'o')
                {
                    pfx[pfxlen++] = '0';
                }
                else if (f != 'u')      /* x, X, b -> 0x, 0X, 0b */
                {
                    pfx[pfxlen++] = '0';
                    pfx[pfxlen++] = f;
                }
            }
            length = _prtu(uv, base, f == 'X', str);
            isnum = 1;
            break;

        case 'p':
            uv = (unsigned long) va_arg(ap, void *);
            if (uv == 0)
            {
                str = "(nil)";
                length = 5;
                fill = ' ';
            }
            else
            {
                pfx[pfxlen++] = '0';
                pfx[pfxlen++] = 'x';
                length = _prtu(uv, 16, 0, str);
                isnum = 1;
            }
            break;

#ifdef FDOPRNT_FLOAT
        case 'f':
        case 'F':
            {
                double dv = va_arg(ap, double);

                if (dv != dv)
                {
                    str = "nan";
                    length = 3;
                    fill = ' ';
                    break;
                }
                if (dv < 0)
                {
                    pfx[pfxlen++] = '-';
                    dv = -dv;
                }
                else if (plus)
                {
                    pfx[pfxlen++] = '+';
                }
                else if (space)
                {
                    pfx[pfxlen++] = ' ';
                }
                if (dv > 1.8e19)        /* too big for ullong (or inf)  */
                {
                    str = "inf";
                    length = 3;
                    fill = ' ';
                }
                else
                {
                    length = _prtf(dv, hasprec ? prec : 6, alt, str);
                }
            }
            break;
#endif

        default:
            /* Unsupported conversion: show it instead of eating it. */
            (*func) (farg, '%');
            (*func) (farg, f);
            continue;
        }

        /* "-" overrides "0"; a precision on an integer overrides "0" */
        if (leftjust)
        {
            fill = ' ';
        }
        if (fmin > MAXSTR)
        {
            fmin = 0;
        }
        if (isnum && hasprec)
        {
            fill = ' ';
            if (prec == 0 && length == 1 && str[0] == '0')
            {
                length = 0;             /* "%.0d" of 0 prints nothing   */
            }
            else if (prec > length)
            {
                zeros = prec - length;
            }
        }

        /* Layout:  [spaces] [sign/prefix] [zero fill] [precision zeros]
         *          text  [spaces if left-justified]                     */
        pad = fmin - (pfxlen + zeros + length);

        if (leftjust == 0 && fill == ' ')
        {
            for (i = 0; i < pad; i++)
            {
                (*func) (farg, ' ');
            }
        }
        for (i = 0; i < pfxlen; i++)
        {
            (*func) (farg, pfx[i]);
        }
        if (leftjust == 0 && fill == '0')
        {
            for (i = 0; i < pad; i++)
            {
                (*func) (farg, '0');
            }
        }
        for (i = 0; i < zeros; i++)
        {
            (*func) (farg, '0');
        }
        for (i = 0; i < length; i++)
        {
            (*func) (farg, str[i]);
        }
        if (leftjust != 0)
        {
            for (i = 0; i < pad; i++)
            {
                (*func) (farg, ' ');
            }
        }
    }

}

/*------------------------------------------------------------------------
 *  _prtu  -  Convert unsigned 64-bit value to a string in base 2, 8, 10
 *            or 16.  Returns the number of digits written.
 *
 *            Base 10 uses a table of powers of ten and subtraction
 *            instead of "/" and "%", so a 32-bit kernel does not need
 *            libgcc's __udivdi3 / __umoddi3 to link.
 *------------------------------------------------------------------------
 */
static int      _prtu(
                  ullong        num,
                  int           base,
                  int           upper,
                  char          *str
                )
{
    static const ullong p10[20] = {
        1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL,
        10000000ULL, 100000000ULL, 1000000000ULL, 10000000000ULL,
        100000000000ULL, 1000000000000ULL, 10000000000000ULL,
        100000000000000ULL, 1000000000000000ULL, 10000000000000000ULL,
        100000000000000000ULL, 1000000000000000000ULL,
        10000000000000000000ULL
    };
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[64];
    int n = 0;
    int i, d, shift;

    if (base == 10)
    {
        for (i = 19; i > 0 && num < p10[i]; i--)
        {;
        }
        for (; i >= 0; i--)
        {
            for (d = 0; num >= p10[i]; d++)
            {
                num -= p10[i];
            }
            str[n++] = '0' + d;
        }
    }
    else
    {
        shift = (base == 16) ? 4 : (base == 8) ? 3 : 1;
        i = 0;
        do
        {
            temp[i++] = digits[num & (base - 1)];
            num >>= shift;
        } while (num != 0);
        while (i > 0)
        {
            str[n++] = temp[--i];
        }
    }
    str[n] = '\0';
    return n;
}

#ifdef FDOPRNT_FLOAT
/*------------------------------------------------------------------------
 *  _prtf  -  Convert a non-negative double (< 1.8e19) to "iii.fff" with
 *            'prec' fractional digits.  Rounds half-up on the binary
 *            value, so exact ties may differ from glibc (which rounds
 *            half-to-even).  No libm needed.
 *------------------------------------------------------------------------
 */
static int      _prtf(
                  double        val,
                  int           prec,
                  int           alt,
                  char          *str
                )
{
    ullong ip;
    double rnd, frac;
    int n, i, d;

    rnd = 0.5;
    for (i = 0; i < prec; i++)
    {
        rnd /= 10.0;
    }
    val += rnd;

    ip = (ullong) val;
    n = _prtu(ip, 10, 0, str);
    frac = val - (double) ip;

    if (prec > 0 || alt)
    {
        str[n++] = '.';
    }
    for (i = 0; i < prec; i++)
    {
        frac *= 10.0;
        d = (int) frac;
        if (d > 9)
        {
            d = 9;
        }
        str[n++] = '0' + d;
        frac -= d;
    }
    str[n] = '\0';
    return n;
}
#endif

///* fdoprnt.c - _fdoprnt, _prtl2, _prtl8, _prtl10, _prtX16, _prtl16 */
//
//#include <stdarg.h>
//
//#define	MAXSTR	80
//#define NULL    0
//
//static void _prtl10(long num, char *str);
//static void _prtl8(long num, char *str);
//static void _prtX16(long num, char *str);
//static void _prtl16(long num, char *str);
//static void _prtl2(long num, char *str);
//
///*------------------------------------------------------------------------
// *  _fdoprnt  -  Format and write output using 'func' to write characters.
// *				 (Patched for Sun3 by Shawn Ostermann.)
// *				 All arguments passed as 4 bytes, long==int.
// *------------------------------------------------------------------------
// */
//void	_fdoprnt(
//	  char		*fmt,			/* format string	*/
//	  va_list	ap,			/* ap list of values	*/
//	  int		(*func)(int, int),	/* char output function	*/
//	  int		farg			/* arg for char output	*/
//	)
//{
//    int c;
//    int i;
//    int f;                      /* The format character (comes after %) */
//    char *str;                  /* Running pointer in string            */
//    char string[20];            /* The string str points to this output */
//
//    /*  from number conversion              */
//    int length;                 /* Length of string "str"               */
//    char fill;                  /* Fill character (' ' or '0')          */
//    int leftjust;               /* 0 = right-justified, else left-just  */
//    int fmax, fmin;             /* Field specifications % MIN . MAX s   */
//    int leading;                /* No. of leading/trailing fill chars   */
//    char sign;                  /* Set to '-' for negative decimals     */
//    char digit1;                /* Offset to add to first numeric digit */
//    long larg;
//
//    for (;;)
//    {
//        /* Echo characters until '%' or end of fmt string */
//        while ((c = *fmt++) != '%')
//        {
//            if (c == '\0')
//            {
//                return;
//            }
//            (*func) (farg, c);
//        }
//        /* Echo "...%%..." as '%' */
//        if (*fmt == '%')
//        {
//            (*func) (farg, *fmt++);
//            continue;
//        }
//        /* Check for "%-..." == Left-justified output */
//        if ((leftjust = ((*fmt == '-')) ? 1 : 0))
//        {
//            fmt++;
//        }
//        /* Allow for zero-filled numeric outputs ("%0...") */
//        fill = (*fmt == '0') ? *fmt++ : ' ';
//        /* Allow for minimum field width specifier for %d,u,x,o,c,s */
//        /* Also allow %* for variable width (%0* as well)       */
//        fmin = 0;
//        if (*fmt == '*')
//        {
//            fmin = va_arg(ap, int);
//
//            ++fmt;
//        }
//        else
//        {
//            while ('0' <= *fmt && *fmt <= '9')
//            {
//                fmin = fmin * 10 + *fmt++ - '0';
//            }
//        }
//        /* Allow for maximum string width for %s */
//        fmax = 0;
//        if (*fmt == '.')
//        {
//            if (*(++fmt) == '*')
//            {
//                fmax = va_arg(ap, int);
//                ++fmt;
//            }
//            else
//            {
//                while ('0' <= *fmt && *fmt <= '9')
//                {
//                    fmax = fmax * 10 + *fmt++ - '0';
//                }
//            }
//        }
//
//        str = string;
//        if ((f = *fmt++) == '\0')
//        {
//            (*func) (farg, '%');
//            return;
//        }
//        sign = '\0';            /* sign == '-' for negative decimal */
//
//        switch (f)
//        {
//        case 'c':
//            string[0] = va_arg(ap, int);
//            string[1] = '\0';
//            fmax = 0;
//            fill = ' ';
//            break;
//
//        case 's':
//            str = va_arg(ap, char *);
//
//            if (NULL == str)
//            {
//                str = "(null)";
//            }
//            fill = ' ';
//            break;
//
//        case 'd':
//            larg = va_arg(ap, long);
//
//            if (larg < 0)
//            {
//                sign = '-';
//                larg = -larg;
//            }
//            _prtl10(larg, str);
//            break;
//
//        case 'u':
//            digit1 = '\0';
//            /* "negative" longs in unsigned format  */
//            /* can't be computed with long division */
//            /* convert *args to "positive", digit1  */
//            /* = how much to add back afterwards    */
//            larg = va_arg(ap, long);
//
//            while (larg < 0)
//            {
//                larg -= 1000000000L;
//                ++digit1;
//            }
//            _prtl10(larg, str);
//            str[0] += digit1;
//            fmax = 0;
//            break;
//
//        case 'o':
//            larg = va_arg(ap, long);
//
//            _prtl8(larg, str);
//            fmax = 0;
//            break;
//
//        case 'X':
//            larg = va_arg(ap, long);
//
//            _prtX16(larg, str);
//            fmax = 0;
//            break;
//
//        case 'x':
//            larg = va_arg(ap, long);
//
//            _prtl16(larg, str);
//            fmax = 0;
//            break;
//
//        case 'b':
//            larg = va_arg(ap, long);
//
//            _prtl2(larg, str);
//            fmax = 0;
//            break;
//
//        default:
//            (*func) (farg, f);
//            break;
//        }
//        for (length = 0; str[length] != '\0'; length++)
//        {;
//        }
//        if (fmin > MAXSTR || fmin < 0)
//        {
//            fmin = 0;
//        }
//        if (fmax > MAXSTR || fmax < 0)
//        {
//            fmax = 0;
//        }
//        leading = 0;
//        if (fmax != 0 || fmin != 0)
//        {
//            if (fmax != 0)
//            {
//                if (length > fmax)
//                {
//                    length = fmax;
//                }
//            }
//            if (fmin != 0)
//            {
//                leading = fmin - length;
//            }
//            if (sign == '-')
//            {
//                --leading;
//            }
//        }
//        if (sign == '-' && fill == '0')
//        {
//            (*func) (farg, sign);
//        }
//        if (leftjust == 0)
//        {
//            for (i = 0; i < leading; i++)
//            {
//                (*func) (farg, fill);
//            }
//        }
//        if (sign == '-' && fill == ' ')
//        {
//            (*func) (farg, sign);
//        }
//        for (i = 0; i < length; i++)
//        {
//            (*func) (farg, str[i]);
//        }
//        if (leftjust != 0)
//        {
//            for (i = 0; i < leading; i++)
//                (*func) (farg, fill);
//        }
//    }
//
//}
//
///*------------------------------------------------------------------------
// *  _prtl10  -  Converts long to base 10 string.
// *------------------------------------------------------------------------
// */
//static void	_prtl10(
//		  long		num,
//		  char		*str
//		)
//{
//    int i;
//    char temp[11];
//
//    temp[0] = '\0';
//    for (i = 1; i <= 10; i++)
//    {
//        temp[i] = num % 10 + '0';
//        num /= 10;
//    }
//    for (i = 10; temp[i] == '0'; i--);
//    if (i == 0)
//        i++;
//    while (i >= 0)
//        *str++ = temp[i--];
//}
//
///*------------------------------------------------------------------------
// *  _prtl8  -  Converts long to base 8 string.
// *------------------------------------------------------------------------
// */
//static void	_prtl8(
//		  long		num,
//		  char		*str
//		)
//{
//    int i;
//    char temp[12];
//
//    temp[0] = '\0';
//    for (i = 1; i <= 11; i++)
//    {
//        temp[i] = (num & 07) + '0';
//        num = num >> 3;
//    }
//    temp[11] &= '3';
//    for (i = 11; temp[i] == '0'; i--);
//    if (i == 0)
//        i++;
//    while (i >= 0)
//        *str++ = temp[i--];
//}
//
///*------------------------------------------------------------------------
// *  _prtl16  -  Converts long to lowercase hex string.
// *------------------------------------------------------------------------
// */
//static void	_prtl16(
//		  long		num,
//		  char		*str
//		)
//{
//    int i;
//    char temp[9];
//
//    temp[0] = '\0';
//    for (i = 1; i <= 8; i++)
//    {
//        temp[i] = "0123456789abcdef"[num & 0x0F];
//        num = num >> 4;
//    }
//    for (i = 8; temp[i] == '0'; i--);
//    if (i == 0)
//        i++;
//    while (i >= 0)
//        *str++ = temp[i--];
//}
//
///*------------------------------------------------------------------------
// *  _prtX16  -  Converts long to uppercase hex string.
// *------------------------------------------------------------------------
// */
//static void	_prtX16(
//		  long		num,
//		  char		*str
//		)
//{
//    int i;
//    char temp[9];
//
//    temp[0] = '\0';
//    for (i = 1; i <= 8; i++)
//    {
//        temp[i] = "0123456789ABCDEF"[num & 0x0F];
//        num = num >> 4;
//    }
//    for (i = 8; temp[i] == '0'; i--);
//    if (i == 0)
//        i++;
//    while (i >= 0)
//        *str++ = temp[i--];
//}
//
///*------------------------------------------------------------------------
// *  _prtl2  -  Converts long to binary string.
// *------------------------------------------------------------------------
// */
//static void	_prtl2(
//		  long		num,
//		  char		*str
//		)
//{
//    int i;
//    char temp[35];
//
//    temp[0] = '\0';
//    for (i = 1; i <= 32; i++)
//    {
//        temp[i] = ((num % 2) == 0) ? '0' : '1';
//        num = num >> 1;
//    }
//    for (i = 32; temp[i] == '0'; i--);
//    if (i == 0)
//        i++;
//    while (i >= 0)
//        *str++ = temp[i--];
//}
