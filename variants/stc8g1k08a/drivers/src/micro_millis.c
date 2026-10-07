#include "Arduino.h"

static volatile uint8_t timer_initialized = 0;
static volatile __xdata uint32_t _millis = 0;

#define CYCLES_PER_MS      ((uint16_t)(F_CPU / 1000UL))
#define TIMER_RELOAD_VALUE ((uint16_t)(65536UL - (F_CPU / 1000UL)))

void timer0_init(void)
{
  if (timer_initialized) return;

  // Stop timer first
  TCON &= ~(1 << 4);  // TR0 = 0

  // Configure Timer0: Mode 0 (16-bit auto-reload)
  TMOD &= 0xF0;       // Clear T0 mode bits (M1=0, M0=0 = Mode 0)
  TMOD &= ~(1 << 2);  // C/T = 0 (timer mode)
  TMOD &= ~(1 << 3);  // GATE = 0

  // Use 1T mode for precision
  AUXR |= (1 << 7);   // T0x12 = 1 (1T mode)

  // Set reload value for 1ms at F_CPU in 1T mode
  TH0 = (uint8_t)(TIMER_RELOAD_VALUE >> 8);
  TL0 = (uint8_t)(TIMER_RELOAD_VALUE & 0xFF);

  // Clear overflow flag
  TCON &= ~(1 << 5);  // TF0 = 0

  // Enable Timer0 interrupt
  IE |= (1 << 1);     // ET0 = 1

  // Enable global interrupts
  IE |= (1 << 7);     // EA = 1

  // Start timer
  TCON |= (1 << 4);   // TR0 = 1
  
  timer_initialized = 1;
}

uint32_t millis(void)
{
  uint32_t m;
  uint8_t saved_ea;
  
  // Initialize on first call
  if (!timer_initialized)
  {
    timer0_init();
  }
  
  // Disable interrupts briefly to read atomically
  saved_ea = READ_BIT(IE, 7);
  CLEAR_BIT(IE, 7);
  m = _millis;
  if (saved_ea)
  {
    SET_BIT(IE, 7);
  }
  
  return m;
}

uint32_t micros(void)
{
  uint32_t m;
  uint16_t timer_count;
  uint8_t tl, th;
  uint8_t saved_ea;
  
  // Initialize on first call
  if (!timer_initialized)
  {
    timer0_init();
  }
  
  // Disable interrupts to read atomically
  saved_ea = READ_BIT(IE, 7);
  CLEAR_BIT(IE, 7);
  
  // Double-read to prevent 16-bit timer rollover glitch
  do {
    th = TH0;
    tl = TL0;
  } while (th != TH0);
  timer_count = ((uint16_t)th << 8) | tl;
  
  // Get current millisecond count
  m = _millis;
  
  // If timer overflow occurred while interrupts were disabled, adjust m
  if (READ_BIT(TCON, 5) && (tl < 200))
  {
    m++;
  }
  
  // Restore interrupt state
  if (saved_ea)
  {
    SET_BIT(IE, 7);
  }
  
  // Calculate elapsed cycles since last reload
  uint16_t elapsed_cycles;
  if (timer_count >= TIMER_RELOAD_VALUE) {
    elapsed_cycles = timer_count - TIMER_RELOAD_VALUE;
  } else {
    // Handle overflow case
    elapsed_cycles = (65536UL - TIMER_RELOAD_VALUE) + timer_count;
  }
  
  uint32_t elapsed_us = ((uint32_t)elapsed_cycles * 1000UL) / CYCLES_PER_MS;
  
  return (m * 1000UL) + elapsed_us;
}

void delay_ms(uint32_t ms)
{
  uint32_t start = millis();
  while ((millis() - start) < ms)
  {
    // Busy wait
  }
}

void delay_us(uint32_t us)
{
  uint32_t start = micros();
  while ((micros() - start) < us)
  {
    // Busy wait
  }
}

void delay_s(uint16_t seconds)
{
  while (seconds--)
  {
    delay_ms(1000);
  }
}

// Timer 0 Overflow Interrupt Service Routine
void timer0_isr(void) __interrupt(TIMER0_ISR_VECTOR)
{
  _millis++;
}