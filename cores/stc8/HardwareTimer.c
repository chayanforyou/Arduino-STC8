#include "HardwareTimer.h"
#include "isr.h"
#include "variant.h"

// Timer counts PCA counter overflows (see pca_isr in isr.c) and uses no PCA
// module, so analogWrite() PWM works on all three CCP pins at the same time.
// The PCA counter is shared with PWM: it runs at SYSclk and is never stopped.

// Shortest period; the ISR (plus the user callback) must finish well within it
#define PCA_MIN_CYCLES 1024UL
// Longest period; keeps the cycle count within 32 bits at 33 MHz
#define PCA_MAX_PERIOD_US 100000000UL
// Default period if start() is called before setPeriod()
#define PCA_DEFAULT_US 10000UL

#define PCA_ECF 0x01  // CMOD.0: enable counter overflow (CF) interrupt
#define PCA_CF 0x80   // CCON.7: counter overflow flag

static uint8_t _pca_is_running = 0;

// Start the shared PCA counter at SYSclk if nobody has started it yet
static void pca_counter_start(void) {
  if (!READ_BIT(CCON, 6)) {
    CMOD = (CMOD & ~0x0E) | PCA_CPS_SYSCLK;
    SET_BIT(CCON, 6);  // CR = 1
  }
}

// Begin a fresh period: force an overflow within 256 cycles (CH = 0xFF);
// the ISR treats it as the start of interval 0. Call with interrupts off.
static void pca_prime(void) {
  _pca_interval_idx = 0xFFFF;
  _pca_frac_acc = 0;
  CCON &= ~PCA_CF;
  CH = 0xFF;
}

static void pca_timer_set_period(uint32_t microseconds) __reentrant {
  uint32_t cycles;
  uint32_t units;
  uint16_t intervals;
  uint8_t per_interval;
  uint8_t saved_ea;

  if (microseconds > PCA_MAX_PERIOD_US)
    microseconds = PCA_MAX_PERIOD_US;

  // cycles = microseconds * F_CPU / 1e6, split so it stays exact in 32 bits
  cycles = (microseconds / 10000UL) * (F_CPU / 100UL) + ((microseconds % 10000UL) * (F_CPU / 100UL)) / 10000UL;
  if (cycles < PCA_MIN_CYCLES)
    cycles = PCA_MIN_CYCLES;

  // Split the period into 256-cycle units, spread over as few overflow
  // intervals as possible with at most 254 units each (room for the +1
  // extra and +1 fractional carry the ISR may add)
  units = cycles >> 8;
  intervals = (uint16_t)((units + 253UL) / 254UL);
  per_interval = (uint8_t)(units / intervals);

  saved_ea = READ_BIT(IE, 7);
  CLEAR_BIT(IE, 7);
  _pca_intervals = intervals;
  _pca_units = per_interval;
  _pca_extra = (uint16_t)(units - (uint32_t)per_interval * intervals);
  _pca_frac = (uint8_t)(cycles & 0xFF);
  if (_pca_is_running) {
    pca_prime();
  }
  if (saved_ea) {
    SET_BIT(IE, 7);
  }
}

static void pca_timer_set_frequency(uint32_t frequency_hz) __reentrant {
  if (frequency_hz == 0)
    return;

  if (frequency_hz >= 1000000UL) {
    pca_timer_set_period(1);
  } else {
    pca_timer_set_period(1000000UL / frequency_hz);
  }
}

static void pca_timer_attach_interrupt(timerCallback_t callback) __reentrant {
  _pca_user_handler = callback;
}

static void pca_timer_detach_interrupt(void) {
  _pca_user_handler = 0;
}

// Also used for restart/resume: always counts a fresh period from now
static void pca_timer_start(void) {
  if (_pca_intervals == 0) {
    pca_timer_set_period(PCA_DEFAULT_US);
  }
  pca_counter_start();

  CLEAR_BIT(IE, 7);
  pca_prime();
  CMOD |= PCA_ECF;
  _pca_is_running = 1;
  SET_BIT(IE, 7);  // EA = 1
}

// Stops only Timer's overflow interrupt; the PCA counter keeps running for PWM
static void pca_timer_stop(void) {
  CMOD &= ~PCA_ECF;
  CCON &= ~PCA_CF;
  _pca_is_running = 0;
}

// HardwareTimer object instance (stored in Flash ROM)
const HardwareTimer_t Timer = {
  .attachInterrupt = pca_timer_attach_interrupt,
  .detachInterrupt = pca_timer_detach_interrupt,
  .setPeriod = pca_timer_set_period,
  .setFrequency = pca_timer_set_frequency,
  .start = pca_timer_start,
  .stop = pca_timer_stop,
  .restart = pca_timer_start,
  .resume = pca_timer_start,
  .pause = pca_timer_stop,
};
