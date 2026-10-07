/*
  EEPROM Save State
  Saves the state of an LED to EEPROM so it persists across resets/power cycles.
  Reads a button on pin P5_5 and toggles LED on pin P3_3 with debouncing.
  
  by Chayan Mistry

  This example code is in the public domain.
*/

#include <Arduino.h>
#include <EEPROM.h>

#define LED_PIN P3_3
#define BUTTON_PIN P5_5
#define EEPROM_ADDR 0

uint8_t ledState;

int buttonState;
int lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Read saved LED state from EEPROM
  ledState = EEPROM.read(EEPROM_ADDR);

  // Treat empty EEPROM (0xFF) as LED ON
  if (ledState == 0xFF) {
    ledState = HIGH;
  }

  digitalWrite(LED_PIN, ledState);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  // Reset debounce timer when button state changes
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Check if button state is stable
  if (millis() - lastDebounceTime > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // Button pressed (Active LOW)
      if (buttonState == LOW) {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);

        // Save new LED state to EEPROM (Erase sector before writing)
        EEPROM.eraseSector(EEPROM_ADDR);
        EEPROM.write(EEPROM_ADDR, ledState);
      }
    }
  }

  lastButtonState = reading;
}
