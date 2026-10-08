/*
  PWM + Hardware Timer
  Demonstrates hardware PWM on all three PWM pins while the hardware Timer
  interrupt is running at the same time.

  Timer counts PCA counter overflows and uses no PWM channel, so P3_2,
  P3_3 and P5_4 all keep giving PWM while Timer ticks.

  - P3_2 (PWM0): fades up and down
  - P3_3 (PWM1): fixed 25% duty
  - P5_4 (PWM2): fixed 75% duty
  - LED_BUILTIN (P5_5): toggled by the Timer interrupt every 500 ms

  Connect LEDs with resistors to P3_2, P3_3 and P5_4, or check them with a
  scope (PWM frequency is F_CPU / 256, about 43.2 kHz at 11.0592 MHz).

  by Chayan Mistry

  This example code is in the public domain.
*/

#include <Arduino.h>

const int fadePin = P3_2;

uint8_t brightness = 0;
int8_t fadeStep = 1;
uint32_t lastFade = 0;

// Runs inside the Timer interrupt: keep it short
void timer_isr() {
  digitalToggle(LED_BUILTIN);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Fixed duty cycles on the other two PWM pins
  analogWrite(P3_3, 64);   // 25%
  analogWrite(P5_4, 192);  // 75%

  // Toggle LED_BUILTIN every 500 ms from the Timer interrupt
  Timer.setPeriod(500000);
  Timer.attachInterrupt(timer_isr);
  Timer.start();
}

void loop() {
  // Non-blocking fade on P3_2: one step every 10 ms
  if (millis() - lastFade >= 10) {
    lastFade = millis();

    analogWrite(fadePin, brightness);

    if (brightness == 0) {
      fadeStep = 1;
    } else if (brightness == 255) {
      fadeStep = -1;
    }
    brightness += fadeStep;
  }
}
