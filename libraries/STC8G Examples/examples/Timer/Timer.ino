/*
  Hardware Timer Interrupt Example
  Uses Hardware Timer interrupt to toggle LED periodically.

  by Chayan Mistry

  This example code is in the public domain.
*/

#include <HardwareTimer.h>

volatile uint8_t timer_flag = 0;
volatile uint32_t isr_counter = 0;

void timer_isr() {
  // Direct hardware pin toggle in ISR
  digitalToggle(LED_BUILTIN);
  timer_flag = 1;
  isr_counter++;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hardware Timer Test");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set Timer period to 500,000 microseconds (500 ms / 0.5 sec)
  Timer.setPeriod(500000);

  // Attach callback function to Timer interrupt
  Timer.attachInterrupt(timer_isr);

  // Start the hardware timer
  Timer.start();

  Serial.println("Timer started with 500ms period");
}

void loop() {
  if (timer_flag) {
    timer_flag = 0;
    Serial.print("Timer Tick #");
    Serial.printNumber(isr_counter);
    Serial.println("");
  }
}
