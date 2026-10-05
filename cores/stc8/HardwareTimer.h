#ifndef HARDWARETIMER_H
#define HARDWARETIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "Arduino.h"

// Callback function type for Timer ISR
typedef void (*timerCallback_t)(void);

// HardwareTimer interface structure
typedef struct
{
    void (*attachInterrupt)(timerCallback_t callback) __reentrant;
    void (*detachInterrupt)(void);
    void (*setPeriod)(uint32_t microseconds) __reentrant;
    void (*setFrequency)(uint32_t frequency_hz) __reentrant;
    void (*start)(void);
    void (*stop)(void);
    void (*restart)(void);
    void (*resume)(void);
    void (*pause)(void);
} HardwareTimer_t;

// Hardware Timer instance (stored in Flash ROM)
extern const HardwareTimer_t Timer;

#endif // HARDWARETIMER_H
