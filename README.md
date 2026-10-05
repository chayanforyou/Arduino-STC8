# STC8 Arduino Core

Arduino support for STC8 microcontrollers with a familiar Arduino-like API.

## Installation

### Via Arduino Board Manager (Recommended)

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to **Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/chayanforyou/Arduino-STC8/main/package_stc8051_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "STC"
6. Click **Install** on "STC Boards"

### Requirements

- Python 3.6 or later (for stcgal programmer)
- Arduino IDE 1.8.x or Arduino IDE 2.x

### Supported Boards

- STC8G1K08A (8KB Flash, 1KB RAM)

## Usage

1. Select **Tools → Board → STC Boards → STC8G1K08A-8PIN**
2. Select your clock frequency under **Tools → Clock Speed**
3. Select your serial port under **Tools → Port**
4. Upload your sketch

## Supported Features & APIs

The core implements standard Arduino functions and STC8 hardware-optimized peripherals:

| Feature | Key APIs / Syntax | Description & Hardware Mapping | Example Sketch |
| :--- | :--- | :--- | :--- |
| **Digital I/O** | `pinMode()`, `digitalWrite()`, `digitalRead()`, `digitalToggle()` | Supports `INPUT`, `OUTPUT`, `INPUT_PULLUP` | `Blink`, `Button` |
| **ADC (Analog Read)** | `analogRead(pin)` | 10-bit resolution (0–1023) across 6 channels (`P3_0`..`P3_3`, `P5_4`, `P5_5`, and internal 1.19V reference) | `ADC` |
| **Hardware PWM** | `analogWrite(pin, value)` | 8-bit PCA PWM (0–255) on `P3_2` (PWM0), `P3_3` (PWM1), `P5_4` (PWM2) | `PWM` |
| **Timing** | `millis()`, `micros()`, `delay()`, `delay_ms()`, `delay_us()` | Non-blocking & blocking delays via Timer 0 | `Millis_Micros` |
| **Hardware Timer** | `Timer.setPeriod()`, `Timer.attachInterrupt()`, `Timer.start()`, `Timer.stop()` | 16-bit hardware PCA timer for periodic background tasks | `Timer` |
| **Serial (UART)** | `Serial.begin()`, `Serial.beginWithPins()`, `print()`, `read()`, `write()` | Hardware UART with pin selection (`UART_PINS_DEFAULT`, `UART_PINS_P32_P33`, `UART_PINS_P54_P55`) | `Serial` |
| **Interrupts** | `attachInterrupt()`, `detachInterrupt()`, `interrupts()`, `noInterrupts()` | Ext. INT0 (`P3_2`), INT1 (`P3_3`), INT2 (`P5_4`), INT3 (`P5_5`), INT4 (`P3_0`) | `Interrupt` |
| **Math & Utils** | `map()`, `constrain()`, `min()`, `max()`, `abs()`, `sq()` | Standard Arduino mathematical functions | `ADC`, `PWM` |
| **EEPROM** | `EEPROM.read()`, `EEPROM.write()`, `EEPROM.update()`, `EEPROM.length()` | Internal IAP Flash EEPROM storage | `EEPROM` |

## Pin Mapping

| Arduino Pin |Pin Number| Physical Pin | Functions |
|------------|-----|--------------|-----------|
| P3_0 / A0 |0| P3.0 | GPIO, RX (UART alt), INT4, **ADC0** |
| P3_1 / A1 |1| P3.1 | GPIO, TX (UART alt), **ADC1** |
| P3_2 / A2 |2| P3.2 | GPIO, RX (UART alt), INT0, **PWM0 (CCP0)**, **ADC2** |
| P3_3 / A3 |3| P3.3 | GPIO, TX (UART alt), INT1, **PWM1 (CCP1)**, **ADC3** |
| P5_4 / A4 |4| P5.4 | GPIO, RX (UART alt), INT2, **PWM2 (CCP2)**, **ADC4** |
| P5_5 / A5 |5| P5.5 | GPIO, TX (UART alt), INT3, **LED_BUILTIN**, **ADC5** |

## Examples

The core includes several examples demonstrating basic functionality:

- **ADC** - 10-bit analog input reading with `analogRead()`
- **Blink** - Basic LED blinking
- **Button** - Reading digital input
- **EEPROM** - Read/write to internal EEPROM
- **Interrupt** - Using external interrupts
- **Millis_Micros** - Non-blocking timing with millis()/micros()
- **PWM** - Smooth hardware PWM LED fading with `analogWrite()`
- **Serial** - Serial communication and echo
- **Timer** - Periodic hardware timer interrupt with `Timer`

## Uploading Sketches

### Connection

Connect a USB-to-TTL adapter to your STC8 board:

| Adapter | STC8 Board |
|---------|------------|
| 3.3V | VCC |
| GND | GND |
| TX | RX (depends on UART pin selection) |
| RX | TX (depends on UART pin selection) |

### Upload Process

1. Click **Upload** in Arduino IDE
2. When prompted, **power cycle the board** (disconnect and reconnect GND)
3. Upload will proceed automatically

## Clock Frequencies

Supported internal RC oscillator frequencies:
- 11.0592 MHz (default)
- 12 MHz
- 16 MHz
- 20 MHz
- 22.1184 MHz
- 24 MHz
- 30 MHz
- 33.1776 MHz

Select frequency via **Tools → Clock Speed** menu.

## Programming Notes

### Memory Limitations
- **Flash:** 8 KB total
- **RAM:** 1 KB (256 bytes internal + 768 bytes XRAM)
- Use `__xdata` keyword for large buffers to save internal RAM

### SDCC Specific
- This core uses SDCC compiler
- Some C++ features may be limited
- Use `__reentrant` on Serial functions with parameters to ensure interrupt-safe execution under SDCC.
- Global variables default to internal RAM (limited to 256 bytes)

<!-- ## Troubleshooting

### Upload Issues
- **No response from MCU:** Ensure power cycle during upload
- **Wrong baud rate:** Try different clock frequencies
- **Permission denied (Linux):** Add user to dialout group: `sudo usermod -a -G dialout $USER`

### Compilation Errors
- **Sketch too big:** Reduce code size or remove unused libraries
- **Out of memory:** Use `__xdata` for large arrays
- **Undefined reference:** Ensure all functions are declared before use -->

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## Resources

- **GitHub Repository:** https://github.com/chayanforyou/Arduino-STC8
- **STC8G Documentation:** https://www.stcmicro.com/stc/stc8g1k08.html
- **Arduino Reference:** https://www.arduino.cc/reference/en/

## Submit Bugs
I would appreciate if you could submit any bugs or issues you find on the [GitHub Issues Page](https://github.com/chayanforyou/Arduino-STC8/issues).