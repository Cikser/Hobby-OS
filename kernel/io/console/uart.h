#ifndef RISC_V_UART_H
#define RISC_V_UART_H

#include "../../types.h"

static constexpr uint64_t CONSOLE_BASE = 0x10000000;
static constexpr uint64_t CONSOLE_STATUS = 0x10000005;
static constexpr uint64_t CONSOLE_TX_DATA = 0x10000000;
static constexpr uint64_t CONSOLE_RX_DATA = 0x10000000;
static constexpr uint64_t CONSOLE_RX_STATUS_BIT = 1;
static constexpr uint64_t CONSOLE_TX_STATUS_BIT = 1 << 6;

#endif //RISC_V_UART_H