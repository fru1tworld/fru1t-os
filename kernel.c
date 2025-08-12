#include "kernel.h"
#include "common.h"

extern char bss[], bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;
    
    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    // Legacy Console Putchar (OpenSBI 1.5.1에서 여전히 지원됨)
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

// 부트 엔트리 포인트
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "la sp, __stack_top\n"    // 스택 포인터 설정
        "call kernel_main\n"      // 커널 메인 호출
        "1: wfi\n"                // 무한 대기
        "j 1b\n"
    );
}

void kernel_main(void) {
    printf("%s", "\n\n Hello Developer Realname Talk room!!");
    
    for (;;) {
        __asm__ __volatile__("wfi");
    }
}
