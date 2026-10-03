#include <EEPROM.h>

void setup() {
  Serial.begin(115200);

  // Erase 512-byte sector at address 0
  EEPROM.eraseSector(0);

  // Write single byte or update (writes only if changed)
  EEPROM.write(0, 42);
  EEPROM.update(1, 100);

  // Read single bytes
  uint8_t val0 = EEPROM.read(0);
  Serial.print("Byte 0: ");
  Serial.printNumber(val0);
  Serial.println("");

  uint8_t val1 = EEPROM.read(1);
  Serial.print("Byte 1: ");
  Serial.printNumber(val1);
  Serial.println("");

  // Write and read block
  char msg_out[] = "Hello STC8!";
  char msg_in[16];

  EEPROM.writeBlock(16, msg_out, sizeof(msg_out));
  EEPROM.readBlock(16, msg_in, sizeof(msg_in));

  Serial.print("String: ");
  Serial.println(msg_in);
}

void loop() {
}
