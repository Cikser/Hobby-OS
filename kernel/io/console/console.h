#ifndef RISC_V_CONSOLE_H
#define RISC_V_CONSOLE_H

#include "../../types.h"
#include "uart.h"
#include "../../proc/sync/lock.h"
#include "../../proc/sync/sem.h"

class Console {

public:
    static char kgetc();
    static void kputc(char ch);
    static void panic(const char *s);
    static void kprintf(const char *fmt, ...);

    static void interruptHandler();

    static void init() {
        writeReg(0x01, 0x01);
        while ((readReg(CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT) != 0) {
            (void)readReg(CONSOLE_RX_DATA);
        }
    }

private:
    static void kputs(const char *s);
    static void kputulong(uint64_t xx, uint32_t base = 10);
    static void kputi(int64_t xx, uint32_t base = 10);

    static uint8_t readReg(uint32_t offset);
    static void writeReg(uint32_t offset, uint8_t value);

    static constexpr uint32_t BUFFER_SIZE = 4096;
    inline static char m_buffer[BUFFER_SIZE];
    static uint32_t m_head, m_tail;

    static Semaphore* m_sem;

    static Semaphore* sem() {
        if (!m_sem) {
            m_sem = new Semaphore(0);
        }
        return m_sem;
    }

    static Lock m_lock;
};

inline void Console::writeReg(uint32_t offset, uint8_t value) {
    *(uint8_t*)(CONSOLE_BASE + offset) = value;
}

inline uint8_t Console::readReg(uint32_t offset) {
    return *(uint8_t*)(CONSOLE_BASE + offset);
}

#endif