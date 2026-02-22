#include "common.h"
#include "kernel.h"

void putchar(char ch);

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // '%' 건너뛰기
            switch (*fmt) { // 다음 문자 읽기
                case '\0': // 포맷 문자열 끝에 '%'
                    putchar('%');
                    goto end;
                case '%': // '%' 출력
                    putchar('%');
                    break;
                case 's': { // NULL로 끝나는 문자열 출력
                    const char *s = va_arg(vargs, const char *);
                    while (*s) {
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': { // 10진수 정수 출력
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
                case 'x': { // 16진수 정수 출력
                    unsigned value = va_arg(vargs, unsigned);
                    for (int i = 7; i >= 0; i--) {
                        unsigned nibble = (value >> (i * 4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                    break;
                }
                case 'u': { // 부호 없는 정수 출력
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
                case 'l': { // long/long long 정수 처리
                    fmt++; // 첫 번째 'l' 건너뛰기
                    if (*fmt == 'l') {
                        // long long
                        fmt++; // 두 번째 'l' 건너뛰기
                        if (*fmt == 'u') {
                            // unsigned long long
                            uint64_t value = va_arg(vargs, uint64_t);

                            // 0에 대한 특수 처리
                            if (value == 0) {
                                putchar('0');
                                break;
                            }

                            // 문자열로 변환 (32비트용 간소화)
                            char buf[32];
                            int pos = 0;

                            while (value > 0 && pos < 31) {
                                buf[pos++] = '0' + (value % 10);
                                value /= 10;
                            }

                            // 역순으로 출력
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
                case 'p': { // 포인터 주소 출력
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
