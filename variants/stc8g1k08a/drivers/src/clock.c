#include "Arduino.h"

/**
 * @brief Initialize the system clock for STC8G1K08A
 * Configures the Internal High-speed RC Oscillator (IRC) and system clock divider.
 */
void clock_init(void)
{
  // Enable Extended SFR (EAXFR) access for registers in 0xFE00-0xFEFF range
  P_SW2 |= 0x80;

  // Enable High-speed Internal IRC oscillator (Bit 7 = EN_IRC)
  HIRCCR |= (1 << 7);
  while (!(HIRCCR & 0x01))
    ; // Wait until IRC is stable and ready (Bit 0 = HIRCRDY)
  
  // Select Internal High-Speed IRC as Master Clock source (MCLKSEL = 000)
  CKSEL &= ~(0x07 << 0);

  // Configure IRC Frequency Band and Clock Division based on F_CPU
#if F_CPU <= 14700000
  // Low-to-medium frequencies: Select 20MHz band with divider
  IRCBAND &= ~0x01; // Select 20MHz band
  CLKDIV = (20000000UL + F_CPU - 1) / F_CPU;

#elif F_CPU <= 26000000
  // Standard 20MHz band without division (frequency tuned via ISP)
  IRCBAND &= ~0x01; // Select 20MHz band
  CLKDIV = 0x01;    // No division (MCLK / 1)

#else
  // High frequencies (up to 35MHz): Select 33MHz band (frequency tuned via ISP)
  IRCBAND |= 0x01;  // Select 33MHz band
  CLKDIV = 0x01;    // No division (MCLK / 1)
#endif

  // Disable Extended SFR access to protect XDATA registers
  P_SW2 &= ~0x80;
}