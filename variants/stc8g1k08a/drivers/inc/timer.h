#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "Arduino.h"

// Timer0 register bit positions
#define TR0_BIT 4    // Timer0 run control bit in TCON
#define TF0_BIT 5    // Timer0 overflow flag bit in TCON
#define T0x12_BIT 7  // Timer0 1T mode bit in AUXR

// Timer1 register bit positions
#define TR1_BIT 6    // Timer1 run control bit in TCON
#define TF1_BIT 7    // Timer1 overflow flag bit in TCON
#define T1x12_BIT 6  // Timer1 1T mode bit in AUXR

#ifdef __cplusplus
extern "C" {
#endif

  /**
 * @brief Initialize Timer0 for millis/micros system tick
 */
  void timer0_init(void);

  /**
 * @brief Delay for specified microseconds (non-destructive)
 * @param us Microseconds to delay
 */
  void delay_us(uint32_t us);

  /**
 * @brief Delay for specified milliseconds (non-destructive)
 * @param ms Milliseconds to delay
 */
  void delay_ms(uint32_t ms);

  /**
 * @brief Delay for specified seconds (non-destructive)
 * @param seconds Seconds to delay
 */
  void delay_s(uint16_t seconds);

#ifdef __cplusplus
}
#endif

#endif  // TIMER_H