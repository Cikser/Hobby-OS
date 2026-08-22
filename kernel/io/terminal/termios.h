#ifndef RISC_V_TERMIOS_H
#define RISC_V_TERMIOS_H

#include "../../types.h"

static constexpr uint64_t TCGETS = 0x5401;
static constexpr uint64_t TCSETS = 0x5402;
static constexpr uint64_t TCSETSW = 0x5403;
static constexpr uint64_t TCSETSF = 0x5404;
static constexpr uint64_t TIOCGPGRP = 0x540F;
static constexpr uint64_t TIOCSPGRP = 0x5410;
static constexpr uint64_t TIOCGWINSZ = 0x5413;
static constexpr uint64_t FIONREAD = 0x541B;


static constexpr uint32_t NCCS = 32;

static constexpr uint32_t VINTR = 0;
static constexpr uint32_t VQUIT = 1;
static constexpr uint32_t VERASE = 2;
static constexpr uint32_t VKILL = 3;
static constexpr uint32_t VEOF = 4;
static constexpr uint32_t VTIME = 5;
static constexpr uint32_t VMIN = 6;
static constexpr uint32_t VSWTC = 7;
static constexpr uint32_t VSTART = 8;
static constexpr uint32_t VSTOP = 9;
static constexpr uint32_t VSUSP = 10;
static constexpr uint32_t VEOL = 11;
static constexpr uint32_t VREPRINT = 12;
static constexpr uint32_t VDISCARD = 13;
static constexpr uint32_t VWERASE = 14;
static constexpr uint32_t VLNEXT = 15;
static constexpr uint32_t VEOL2 = 16;

static constexpr uint32_t ICRNL = 0000400;
static constexpr uint32_t IXON = 0002000;

static constexpr uint32_t OPOST = 0000001;
static constexpr uint32_t ONLCR = 0000004;

static constexpr uint32_t ISIG = 0000001;
static constexpr uint32_t ICANON = 0000002;
static constexpr uint32_t ECHO = 0000010;
static constexpr uint32_t ECHOE = 0000020;
static constexpr uint32_t ECHOK = 0000040;
static constexpr uint32_t ECHONL = 0000100;
static constexpr uint32_t NOFLSH = 0000200;
static constexpr uint32_t TOSTOP = 0000400;
static constexpr uint32_t IEXTEN = 0100000;

struct ktermios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    uint8_t c_line;
    uint8_t c_cc[NCCS];
    uint32_t c_ispeed;
    uint32_t c_ospeed;
};

struct kwinsize {
    uint16_t ws_row;
    uint16_t ws_col;
    uint16_t ws_xpixel;
    uint16_t ws_ypixel;
};

#endif