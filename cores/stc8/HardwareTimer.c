#include "HardwareTimer.h"
#include "variant.h"

// User callback pointer
static timerCallback_t _pca_user_handler = 0;

// Prescaler variables for multi-millisecond/second periods
static volatile uint16_t _pca_prescaler = 0;
static volatile uint16_t _pca_prescaler_target = 1;
static uint16_t _pca_step = 9216;
static uint8_t _pca_is_running = 0;

static void pca_timer_set_period(uint32_t microseconds) __reentrant
{
    uint32_t f_cpu_khz = F_CPU / 1000UL;
    uint8_t was_running = _pca_is_running;

    if (microseconds == 0) microseconds = 1;

    // Stop PCA while configuring
    CCON &= ~(1 << 6); // CR = 0

    // Check if period fits in 1T mode (up to ~5ms)
    if (microseconds <= 5000UL)
    {
        uint32_t cycles_1t = (f_cpu_khz * microseconds) / 1000UL;
        if (cycles_1t > 0 && cycles_1t <= 65535UL)
        {
            CMOD = 0x08; // 1T mode (SYSclk/1)
            _pca_step = (uint16_t)cycles_1t;
            _pca_prescaler_target = 1;
        }
        else
        {
            CMOD = 0x08;
            _pca_step = (uint16_t)f_cpu_khz;
            _pca_prescaler_target = 1;
        }
    }
    // Check if period fits in 12T mode (up to ~50ms)
    else if (microseconds <= 50000UL && ((f_cpu_khz * microseconds) / 12000UL) <= 65535UL)
    {
        CMOD = 0x00; // 12T mode (SYSclk/12)
        _pca_step = (uint16_t)((f_cpu_khz * microseconds) / 12000UL);
        _pca_prescaler_target = 1;
    }
    else
    {
        // For larger periods (e.g., 100ms, 500ms, 1s, etc.):
        // Use 10ms base tick in 12T mode with software counter in ISR
        CMOD = 0x00; // 12T mode (SYSclk/12)
        _pca_step = (uint16_t)(f_cpu_khz * 10UL / 12UL); // 10ms base tick (9216 at 11.0592MHz)
        _pca_prescaler_target = (uint16_t)((microseconds + 5000UL) / 10000UL);
        if (_pca_prescaler_target == 0) _pca_prescaler_target = 1;
    }

    _pca_prescaler = 0;

    // CCAPM0 = 0x49 (ECOM0=1, MAT0=1, ECCF0=1) -> 16-bit match timer with interrupt
    CCAPM0 = 0x49;

    uint16_t cur = ((uint16_t)CH << 8) | CL;
    cur += _pca_step;
    CCAP0L = (uint8_t)(cur & 0xFF);
    CCAP0H = (uint8_t)(cur >> 8);

    if (was_running)
    {
        CCON |= (1 << 6); // CR = 1
        IE |= (1 << 7);   // EA = 1
    }
}

static void pca_timer_set_frequency(uint32_t frequency_hz) __reentrant
{
    if (frequency_hz == 0) return;

    if (frequency_hz >= 1000000UL)
    {
        pca_timer_set_period(1);
    }
    else
    {
        pca_timer_set_period(1000000UL / frequency_hz);
    }
}

static void pca_timer_attach_interrupt(timerCallback_t callback) __reentrant
{
    _pca_user_handler = callback;
    if (callback)
    {
        CCAPM0 = 0x49;  // Enable match & interrupt
        IE |= (1 << 7); // EA = 1
    }
    else
    {
        CCAPM0 &= ~0x01; // ECCF0 = 0
    }
}

static void pca_timer_detach_interrupt(void)
{
    CCAPM0 &= ~0x01; // ECCF0 = 0
    _pca_user_handler = 0;
}

static void pca_timer_start(void)
{
    _pca_prescaler = 0;
    CCON &= ~0x01; // Clear CCF0

    uint16_t cur = ((uint16_t)CH << 8) | CL;
    cur += _pca_step;
    CCAP0L = (uint8_t)(cur & 0xFF);
    CCAP0H = (uint8_t)(cur >> 8);

    CCAPM0 = 0x49;    // Enable match & interrupt
    CCON |= (1 << 6); // CR = 1 (Start counter)
    IE |= (1 << 7);   // EA = 1
    _pca_is_running = 1;
}

static void pca_timer_stop(void)
{
    CCON &= ~(1 << 6); // CR = 0 (Stop counter)
    _pca_is_running = 0;
}

static void pca_timer_restart(void)
{
    pca_timer_stop();
    _pca_prescaler = 0;
    CL = 0;
    CH = 0;
    pca_timer_start();
}

// HardwareTimer object instance (stored in Flash ROM)
const HardwareTimer_t Timer = {
    .attachInterrupt = pca_timer_attach_interrupt,
    .detachInterrupt = pca_timer_detach_interrupt,
    .setPeriod = pca_timer_set_period,
    .setFrequency = pca_timer_set_frequency,
    .start = pca_timer_start,
    .stop = pca_timer_stop,
    .restart = pca_timer_restart,
    .resume = pca_timer_start,
    .pause = pca_timer_stop,
};

// PCA Hardware Timer Interrupt Service Routine
void pca_isr(void) __interrupt(PCA_ISR_VECTOR)
{
    if (CCON & 0x01) // CCF0
    {
        CCON &= ~0x01; // Clear CCF0
        uint16_t cur = ((uint16_t)CCAP0H << 8) | CCAP0L;
        cur += _pca_step;
        CCAP0L = (uint8_t)(cur & 0xFF);
        CCAP0H = (uint8_t)(cur >> 8);

        _pca_prescaler++;
        if (_pca_prescaler >= _pca_prescaler_target)
        {
            _pca_prescaler = 0;
            if (_pca_user_handler)
            {
                _pca_user_handler();
            }
        }
    }
    if (CCON & 0x80) // CF
    {
        CCON &= ~0x80;
    }
}
