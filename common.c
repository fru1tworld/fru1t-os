#include "common.h"
#include "kernel.h"

void putchar(char ch);

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // Skip '%'
            switch (*fmt) { // Read the next character
                case '\0': // '%' at the end of the format string
                    putchar('%');
                    goto end;
                case '%': // Print '%'
                    putchar('%');
                    break;
                case 's': { // Print a NULL-terminated string.
                    const char *s = va_arg(vargs, const char *);
                    while (*s) {
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': { // Print an integer in decimal.
                    int value = va_arg(vargs, int);
                    unsigned magnitude = value; // https://github.com/nuta/operating-system-in-1000-lines/issues/64
                    if (value < 0) {
                        putchar('-');
                        magnitude = -magnitude;
                    }

                    unsigned divisor = 1;
                    while (magnitude / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        putchar('0' + magnitude / divisor);
                        magnitude %= divisor;
                        divisor /= 10;
                    }

                    break;
                }
                case 'x': { // Print an integer in hexadecimal.
                    unsigned value = va_arg(vargs, unsigned);
                    for (int i = 7; i >= 0; i--) {
                        unsigned nibble = (value >> (i * 4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                    break;
                }
                case 'u': { // Print unsigned integer
                    unsigned value = va_arg(vargs, unsigned);
                    unsigned divisor = 1;
                    while (value / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        putchar('0' + value / divisor);
                        value %= divisor;
                        divisor /= 10;
                    }
                    break;
                }
                case 'l': { // Handle long/long long integers
                    fmt++; // Skip first 'l'
                    if (*fmt == 'l') {
                        // long long
                        fmt++; // Skip second 'l'
                        if (*fmt == 'u') {
                            // unsigned long long
                            uint64_t value = va_arg(vargs, uint64_t);

                            // Special case for 0
                            if (value == 0) {
                                putchar('0');
                                break;
                            }

                            // Convert to string (simplified for 32-bit)
                            char buf[32];
                            int pos = 0;

                            while (value > 0 && pos < 31) {
                                buf[pos++] = '0' + (value % 10);
                                value /= 10;
                            }

                            // Print in reverse
                            for (int i = pos - 1; i >= 0; i--) {
                                putchar(buf[i]);
                            }
                        } else if (*fmt == 'd') {
                            // signed long long
                            int64_t value = va_arg(vargs, int64_t);
                            uint64_t magnitude = value;

                            if (value < 0) {
                                putchar('-');
                                magnitude = -magnitude;
                            }

                            if (magnitude == 0) {
                                putchar('0');
                                break;
                            }

                            char buf[32];
                            int pos = 0;

                            while (magnitude > 0 && pos < 31) {
                                buf[pos++] = '0' + (magnitude % 10);
                                magnitude /= 10;
                            }

                            for (int i = pos - 1; i >= 0; i--) {
                                putchar(buf[i]);
                            }
                        }
                    } else if (*fmt == 'u') {
                        // long unsigned
                        unsigned long value = va_arg(vargs, unsigned long);
                        unsigned long divisor = 1;
                        while (value / divisor > 9)
                            divisor *= 10;

                        while (divisor > 0) {
                            putchar('0' + value / divisor);
                            value %= divisor;
                            divisor /= 10;
                        }
                    } else if (*fmt == 'd') {
                        // long signed
                        long value = va_arg(vargs, long);
                        unsigned long magnitude = value;
                        if (value < 0) {
                            putchar('-');
                            magnitude = -magnitude;
                        }

                        unsigned long divisor = 1;
                        while (magnitude / divisor > 9)
                            divisor *= 10;

                        while (divisor > 0) {
                            putchar('0' + magnitude / divisor);
                            magnitude %= divisor;
                            divisor /= 10;
                        }
                    }
                    break;
                }
                case 'p': { // Print pointer address
                    void *ptr = va_arg(vargs, void *);
                    unsigned value = (unsigned)(uintptr_t)ptr;
                    putchar('0');
                    putchar('x');
                    for (int i = 7; i >= 0; i--) {
                        unsigned nibble = (value >> (i * 4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                    break;
                }
            }
        } else {
            putchar(*fmt);
        }

        fmt++;
    }

end:
    va_end(vargs);
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = c;
    }
    return s;
}

void handle_syscall(struct trap_frame *f) {
    (void)f;
}
