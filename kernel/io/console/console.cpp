#include "console.h"
#include "../../hw/riscv.h"
#include "uart.h"

void Console::kputc(const char ch){
    while ((*(char*)(CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT) == 0){}
	*(char*)CONSOLE_TX_DATA = ch;
}

void Console::kputs(const char* s){
    while (*s) kputc(*s++);
}

char Console::kgetc(){
    while ((*(char*)(CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT) == 0){}
    return *(char*)(CONSOLE_RX_DATA);
}

char digits[] = "0123456789ABCDEF";

void Console::kputulong(const uint64_t xx, const uint32_t base){
    uint64_t x = xx;
    char buf[32];
    int i = 0;
    do{
        buf[i++] = digits[x % base];
    }while((x /= base) != 0);

    while(--i >= 0)
        kputc(buf[i]);

}

void Console::panic(const char *s) {
    kputs("PANIC: ");
    kputs(s);
    kputc('\n');
    RiscV::stopEmulation();
}

void Console::kputi(const int64_t xx, const uint32_t base) {
    int64_t val = xx;
    char buf[64];
    int i = 0;
    unsigned long uval;

    if (base == 10 && val < 0) {
        kputc('-');
        uval = (unsigned long)(-val);
    } else {
        uval = (unsigned long)val;
    }

    if (uval == 0) {
        kputc('0');
        return;
    }

    while (uval > 0) {
        unsigned long rem = uval % base;
        if (rem < 10) {
            buf[i++] = rem + '0';
        } else {
            buf[i++] = rem - 10 + 'a';
        }
        uval /= base;
    }

    while (i > 0) {
        kputc(buf[--i]);
    }
}

void Console::kprintf(const char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    for (const char* p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            kputc(*p);
            continue;
        }

        p++;

        bool is_long = false;
        if (*p == 'l') {
            is_long = true;
            p++;
        }

        switch (*p) {
            case 'd':
                if (is_long) {
                    long ld = __builtin_va_arg(args, long);
                    kputi(ld, 10);
                } else {
                    int d = __builtin_va_arg(args, int);
                    kputi(d, 10);
                }
                break;
            case 'x':
                if (is_long) {
                    unsigned long lx = __builtin_va_arg(args, unsigned long);
                    kputi(lx, 16);
                } else {
                    unsigned int x = __builtin_va_arg(args, unsigned int);
                    kputi(x, 16);
                }
                break;
            case 's':
                kputs(__builtin_va_arg(args, const char*));
                break;
            case 'p':
                kputs("0x");
                kputi(__builtin_va_arg(args, unsigned long), 16);
                break;
            case 'c':
                kputc((char)__builtin_va_arg(args, int));
                break;
            default:
                kputc(*p);
                break;
        }
    }
    __builtin_va_end(args);
}