#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "../../types.h"

class Console {

public:
    static char kgetc();
    static void kputc(char ch);
    static void panic(const char *s);
    static void kprintf(const char *fmt, ...);

private:
    static void kputs(const char *s);
    static void kputulong(const uint64_t xx, const uint32_t base = 10);
    static void kputi(const int64_t xx, const uint32_t base = 10);
    
};

#endif