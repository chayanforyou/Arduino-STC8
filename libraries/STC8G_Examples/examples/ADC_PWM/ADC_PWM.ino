/*
  Analog Input, Analog Output, Serial Output
  Reads an analog input pin, maps the result to a range from 0 to 255 and
  uses the result to set the PWM duty cycle of an output pin.
  Also prints the results to the Serial Monitor.

  The circuit:
  - Potentiometer connected to A4 (P5_4).
    Center pin of the potentiometer goes to the analog pin.
    Side pins of the potentiometer go to VCC and GND.
  - LED connected through a resistor from P3_3 (PWM1) to GND.

  Serial uses the default pins P3_0 (RX) / P3_1 (TX) at 115200 baud.

  Based on the Arduino AnalogInOutSerial example.

  by Chayan Mistry

  This example code is in the public domain.
*/

#include <Arduino.h>

const int analogInPin = A4;     // Analog input pin that the potentiometer is attached to
const int analogOutPin = P3_3;  // PWM output pin that the LED is attached to

int sensorValue = 0;  // value read from the pot
int outputValue = 0;  // value output to the PWM (analog out)

void setup() {
  // initialize serial communications at 115200 bps:
  Serial.begin(115200);
}

void loop() {
  // read the analog in value:
  sensorValue = analogRead(analogInPin);
  // map it to the range of the analog out:
  outputValue = map(sensorValue, 0, 1023, 0, 255);
  // change the analog out value:
  analogWrite(analogOutPin, outputValue);

  // print the results to the Serial Monitor:
  Serial.print("sensor = ");
  Serial.printNumber(sensorValue);
  Serial.print("\t output = ");
  Serial.printlnNumber(outputValue);

  // wait 2 milliseconds before the next loop for the analog-to-digital
  // converter to settle after the last reading:
  delay(2);
}
