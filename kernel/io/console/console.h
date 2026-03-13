#ifndef RISC_V_CONSOLE_H
#define RISC_V_CONSOLE_H

#include "../../types.h"

class Console {

public:
    static char kgetc();
    static void kputc(char ch);
    static void panic(const char *s);
    static void kprintf(const char *fmt, ...);

private:
    static void kputs(const char *s);
    static void kputulong(uint64_t xx, uint32_t base = 10);
    static void kputi(int64_t xx, uint32_t base = 10);
    
};

#endif