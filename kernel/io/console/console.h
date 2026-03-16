#ifndef RISC_V_CONSOLE_H
#define RISC_V_CONSOLE_H

#include "../../types.h"
#include "uart.h"

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

    static uint8_t readReg(uint32_t offset);
    static void writeReg(uint32_t offset, uint8_t value);
    
};

inline void Console::writeReg(uint32_t offset, uint8_t value) {
    *(uint8_t*)(CONSOLE_BASE + offset) = value;
}

inline uint8_t Console::readReg(uint32_t offset) {
    return *(uint8_t*)(CONSOLE_BASE + offset);
}

#endif