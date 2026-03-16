#ifndef RISC_V_UART_H
#define RISC_V_UART_H

#include "../../types.h"
#include "../../hw/memlayout.h"

static constexpr uint64_t CONSOLE_BASE = MemoryLayout::UART_BASE;
static constexpr uint64_t CONSOLE_STATUS = 0x005;
static constexpr uint64_t CONSOLE_TX_DATA = 0x0;
static constexpr uint64_t CONSOLE_RX_DATA = 0x0;
static constexpr uint64_t CONSOLE_RX_STATUS_BIT = 1;
static constexpr uint64_t CONSOLE_TX_STATUS_BIT = 1 << 6;

#endif //RISC_V_UART_H