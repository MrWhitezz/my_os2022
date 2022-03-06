#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

enum flag_itoa {
    FILL_ZERO = 1,
    PUT_PLUS = 2,
    PUT_MINUS = 4,
    BASE_2 = 8,
    BASE_10 = 16,
};

static char * sitoa(char * buf, unsigned int num, int width, enum flag_itoa flags)
{
    unsigned int base;
    if (flags & BASE_2)
        base = 2;
    else if (flags & BASE_10)
        base = 10;
    else
        base = 16; // hex

    char tmp[32];
    char *p = tmp;
    do {
        int rem = num % base;
        *p++ = (rem <= 9) ? (rem + '0') : (rem + 'a' - 0xA);
    } while ((num /= base));
    width -= p - tmp;
    char fill = (flags & FILL_ZERO)? '0' : ' ';
    while (0 <= --width) {
        *(buf++) = fill;
    }
    if (flags & PUT_MINUS)
        *(buf++) = '-';
    else if (flags & PUT_PLUS)
        *(buf++) = '+';
    do
        *(buf++) = *(--p);
    while (tmp < p);
    return buf;
}


int vsprintf(char *out, const char *fmt, va_list ap) {
    char c;
    const char *save = out;

    while ((c  = *fmt++)) {
        int width = 0;
        enum flag_itoa flags = 0;
        if (c != '%') {
            *(out++) = c;
            continue;
        }
    redo_spec:
        c  = *fmt++;
        switch (c) {
        case '%':
            *(out++) = c;
            break;
        case 'c':;
            *(out++) = va_arg(ap, int);
            break;
        case 'd':;
            int num = va_arg(ap, int);
            if (num < 0) {
                num = -num;
                flags |= PUT_MINUS;
            }
            out = sitoa(out, num, width, flags | BASE_10);
            break;
        case 'u':
            out = sitoa(out, va_arg(ap, unsigned int), width, flags | BASE_10);
            break;
        case 'p':
            out = sitoa(out, va_arg(ap, unsigned int), 12, flags | FILL_ZERO); // not sure what %p is
            break;
        case 'x':
            out = sitoa(out, va_arg(ap, unsigned int), width, flags); // hex need no flag
            break;
        case 'b':
            out = sitoa(out, va_arg(ap, unsigned int), width, flags | BASE_2);
            break;
        case 's':;
            const char *p  = va_arg(ap, const char *);
            if (p) {
                while (*p)
                    *(out++) = *(p++);
            }
            break;
        // case 'm':; // I don't know what is m for;
        //     const uint8_t *m  = va_arg(ap, const uint8_t *);
        //     width = width < 64 ? width : 64; // buffer limited to 256!
        //     if (m)
        //         for (;;) {
        //             out = sitoa(out, *(m++), 2, FILL_ZERO);
        //             if (--width <= 0)
        //                 break;
        //             *(out++) = ':';
        //         }
        //     break;
        case '0':
            if (!width)
                flags |= FILL_ZERO;
            // fall through
        case '1'...'9':
            width = width * 10 + c - '0';
            goto redo_spec;
        // case '*':
        //     width = va_arg(ap, unsigned int); // don't know this * for
        //     goto redo_spec;
        case '+':
            flags |= PUT_PLUS;
            goto redo_spec;
        case '\0':
        default:
            assert(0);
            *(out++) = '?'; // bad res
        }
        width = 0;
    }
    *out = '\0';
    return out - save;  
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap); 
  return ret;
}

int printf(const char *fmt, ...) {
    char buff[2048]; // the output should not be longer than 2048
    va_list ap;
    va_start(ap, fmt);
    int ret = vsprintf(buff, fmt, ap);
    va_end(ap); 
    char* p = buff;
    while (*p != '\0') putch(*p++); 
    return ret;
    return 0;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
