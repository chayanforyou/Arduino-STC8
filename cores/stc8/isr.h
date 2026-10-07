// ============= isr.h =============
// Shared state between the always-linked ISR stubs (isr.c) and the
// peripheral modules. Keeping the ISRs out of HardwareSerial.c,
// HardwareTimer.c and interrupt.c means the vector table in main.c no longer
// forces those modules (and their buffers) into every sketch.
#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include "interrupt.h"
#include "HardwareTimer.h"

// UART1 RX ring buffer (buffer itself lives in HardwareSerial.c)
#define SERIAL_RX_BUFFER_SIZE 64
#define SERIAL_RX_BUFFER_MASK (SERIAL_RX_BUFFER_SIZE - 1)

extern volatile __xdata uint8_t *_uart1_rx_buf;  // Set by Serial.begin()
extern volatile uint8_t _uart1_rx_head;
extern volatile uint8_t _uart1_rx_tail;

// PCA hardware timer state (Timer counts PCA counter overflows, see isr.c)
extern timerCallback_t _pca_user_handler;
extern uint16_t _pca_intervals;  // Overflow intervals per period, 0 = not set
extern uint16_t _pca_extra;      // First _pca_extra intervals get one more unit
extern uint8_t _pca_units;       // 256-cycle units per interval (1..254)
extern uint8_t _pca_frac;        // Leftover cycles per period (0..255)
extern uint8_t _pca_frac_acc;
extern volatile uint16_t _pca_interval_idx;  // 0xFFFF = next overflow starts a period

// External interrupt user handlers
extern voidFuncPtr int0_user_handler;
extern voidFuncPtr int1_user_handler;
extern voidFuncPtr int2_user_handler;
extern voidFuncPtr int3_user_handler;
extern voidFuncPtr int4_user_handler;

#endif  // ISR_H
