// ============= isr.c =============
// Interrupt service routines referenced by the vector table in main.c.
// This module is always linked, so keep it small: no buffers, no
// peripheral setup code - only the minimal state each ISR needs.
#include "isr.h"
#include "variant.h"

// ---------------- UART1 ----------------
volatile __xdata uint8_t *_uart1_rx_buf = 0;
volatile uint8_t _uart1_rx_head = 0;
volatile uint8_t _uart1_rx_tail = 0;

void uart1_isr(void) __interrupt(UART1_ISR_VECTOR) {
  // Handle receive interrupt
  if (READ_BIT(SCON, 0))  // RI flag
  {
    uint8_t next_head = (_uart1_rx_head + 1) & SERIAL_RX_BUFFER_MASK;
    uint8_t received_byte = SBUF;

    // No NULL check on _uart1_rx_buf: rx_buffer can legitimately sit at
    // XRAM address 0x0000, and this ISR is only enabled (ES) by
    // Serial.begin() after the pointer is set.
    if (next_head != _uart1_rx_tail) {
      _uart1_rx_buf[_uart1_rx_head] = received_byte;
      _uart1_rx_head = next_head;
    }

    CLEAR_BIT(SCON, 0);  // Clear RI
  }
}

// ---------------- PCA hardware timer ----------------
// Timer uses no PCA module, so all three CCP channels stay free for PWM.
// It counts PCA counter overflows (CF). On each overflow the ISR adds to CH
// so the next overflow comes `units` x 256 cycles after the previous one.
// 8-bit PWM only compares CL, which is never written, so PWM is unaffected.
// A period is _pca_intervals intervals of _pca_units units; the first
// _pca_extra intervals get one more unit, and the leftover cycles (_pca_frac)
// accumulate until they add a unit, so the average period is exact.
timerCallback_t _pca_user_handler = 0;
uint16_t _pca_intervals = 0;
uint16_t _pca_extra = 0;
uint8_t _pca_units = 1;
uint8_t _pca_frac = 0;
uint8_t _pca_frac_acc = 0;
volatile uint16_t _pca_interval_idx = 0xFFFF;

void pca_isr(void) __interrupt(PCA_ISR_VECTOR) {
  if (CCON & 0x80)  // CF: PCA counter overflow
  {
    uint8_t units;
    uint8_t period_done = 0;

    CCON &= ~0x80;  // Clear CF

    // This overflow ends one interval and starts the next
    // (0xFFFF + 1 wraps to 0: the priming overflow after start/setPeriod)
    if (++_pca_interval_idx >= _pca_intervals) {
      _pca_interval_idx = 0;
      period_done = 1;
    }

    // Units for the interval starting now (at most 254 + 1 + 1 = 256, which
    // wraps to 0 below and correctly gives a full 65536-cycle interval)
    units = _pca_units;
    if (_pca_interval_idx < _pca_extra) {
      units++;
    }
    if (_pca_interval_idx == 0) {
      uint16_t acc = (uint16_t)_pca_frac_acc + _pca_frac;
      if (acc >= 256) {
        units++;
        acc -= 256;
      }
      _pca_frac_acc = (uint8_t)acc;
    }

    // Add relative to the current CH so ISR latency doesn't shift the
    // schedule. Don't read-modify-write CH just before CL wraps.
    while (CL >= 0xF0)
      ;
    CH += (uint8_t)(0 - units);

    if (period_done && _pca_user_handler) {
      _pca_user_handler();
    }
  }
}

// ---------------- External interrupts ----------------
voidFuncPtr int0_user_handler = 0;
voidFuncPtr int1_user_handler = 0;
voidFuncPtr int2_user_handler = 0;
voidFuncPtr int3_user_handler = 0;
voidFuncPtr int4_user_handler = 0;

void INT0_ISR(void) __interrupt(INT0_ISR_VECTOR) {
  if (int0_user_handler) {
    int0_user_handler();
  }
  CLEAR_BIT(TCON, 1);  // Clear IE0 flag
}

void INT1_ISR(void) __interrupt(INT1_ISR_VECTOR) {
  if (int1_user_handler) {
    int1_user_handler();
  }
  CLEAR_BIT(TCON, 3);  // Clear IE1 flag
}

void INT2_ISR(void) __interrupt(INT2_ISR_VECTOR) {
  if (int2_user_handler) {
    int2_user_handler();
  }
  AUXINTIF &= ~0x20;  // Clear INT2IF flag
}

void INT3_ISR(void) __interrupt(INT3_ISR_VECTOR) {
  if (int3_user_handler) {
    int3_user_handler();
  }
  AUXINTIF &= ~0x40;  // Clear INT3IF flag
}

void INT4_ISR(void) __interrupt(INT4_ISR_VECTOR) {
  if (int4_user_handler) {
    int4_user_handler();
  }
  AUXINTIF &= ~0x80;  // Clear INT4IF flag
}
