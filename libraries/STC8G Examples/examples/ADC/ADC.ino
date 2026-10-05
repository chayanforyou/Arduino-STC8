/*
  AnalogRead (ADC)
  Reads an analog input on pin A0 (or A1..A5) and prints the result to the Serial Monitor.

  Analog Pins on STC8G1K08A:
  - A0 (P3_0)
  - A1 (P3_1)
  - A2 (P3_2)
  - A3 (P3_3)
  - A4 (P5_4)
  - A5 (P5_5)

  Connect the center pin of a potentiometer to pin A4, and the outside pins to VCC and GND.

  by Chayan Mistry

  This example code is in the public domain.
*/

#include <Arduino.h>

// Define the analog pin being used
const int analogPin = A4;

// Variable to store the raw digital value (0 to 1023)
int sensorValue = 0;

void setup() {
  // Initialize serial communication at 115200 bits per second
  Serial.begin(115200);
}

void loop() {
  // Read the analog value from pin A0
  sensorValue = analogRead(analogPin);

  // Print the raw value to the Serial Monitor
  Serial.print("Raw ADC Value: ");
  Serial.printlnNumber(sensorValue);

  // Wait 500 milliseconds before taking the next reading
  delay(500);
}
