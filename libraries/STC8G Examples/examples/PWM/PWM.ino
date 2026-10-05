/*
  Hardware PWM / Fading
  Demonstrates smooth hardware PWM fading using analogWrite() on STC8G.

  Hardware PWM Pins on STC8G1K08A:
  - P3_2 (PWM0 / CCP0)
  - P3_3 (PWM1 / CCP1)
  - P5_4 (PWM2 / CCP2)

  Connect an LED with a resistor to P3_3 (or any PWM pin).

  by Chayan Mistry

  This example code is in the public domain.
*/

#include <Arduino.h>

const int ledPin = P3_3; // LED connected to PWM pin P3_3

void setup() {
  // analogWrite() does not strictly require pinMode(), 
  // but defining it as an OUTPUT is good practice.
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Fade IN: Increase brightness from 0 to 255
  for (int brightness = 0; brightness <= 255; brightness++) {
    analogWrite(ledPin, brightness);
    delay(10); // Wait 10ms to see the fading effect
  }

  // Fade OUT: Decrease brightness from 255 down to 0
  for (int brightness = 255; brightness >= 0; brightness--) {
    analogWrite(ledPin, brightness);
    delay(10);
  }
}
