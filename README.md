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

- Python 3.8 or later, 3.12 recommended:
  - Windows: [python-3.12.0-amd64.exe](https://www.python.org/ftp/python/3.12.0/python-3.12.0-amd64.exe) (tick **Add python.exe to PATH** in the installer)
  - macOS: [python-3.12.0-macos11.pkg](https://www.python.org/ftp/python/3.12.0/python-3.12.0-macos11.pkg)
  - Linux: `sudo apt install python3`
- Apple Silicon Macs: the bundled SDCC is an x86_64 build and needs Rosetta 2 (`softwareupdate --install-rosetta`)
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
| **Hardware Timer** | `Timer.setPeriod()`, `Timer.attachInterrupt()`, `Timer.start()`, `Timer.stop()` | Periodic interrupt from the PCA counter overflow; works alongside PWM on all 3 pins. See [PCA sharing](#pca-sharing-pwm-and-timer) | `Timer`, `PWM_Timer` |
| **Serial (UART)** | `Serial.begin()`, `Serial.beginWithPins()`, `print()`, `read()`, `write()` | Hardware UART with pin selection (`UART_PINS_DEFAULT`, `UART_PINS_P32_P33`, `UART_PINS_P54_P55`) | `Serial` |
| **Interrupts** | `attachInterrupt()`, `detachInterrupt()`, `interrupts()`, `noInterrupts()` | Ext. INT0 (`P3_2`), INT1 (`P3_3`), INT2 (`P5_4`), INT3 (`P5_5`), INT4 (`P3_0`) | `Interrupt` |
| **Math & Utils** | `map()`, `constrain()`, `min()`, `max()`, `abs()`, `sq()` | Standard Arduino mathematical functions | `ADC`, `PWM` |
| **EEPROM** | `EEPROM.read()`, `EEPROM.write()`, `EEPROM.update()`, `EEPROM.eraseSector()`, `EEPROM.readBlock()`, `EEPROM.writeBlock()`, `EEPROM.length()` | 4 KB IAP data flash (8 × 512-byte sectors). See [EEPROM notes](#eeprom-notes) | `EEPROM`, `EEPROM_SaveState` |

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
- **ADC_PWM** - Read a potentiometer with `analogRead()`, set LED brightness with `analogWrite()`, and print both values
- **Blink** - Basic LED blinking
- **Button** - Reading digital input
- **EEPROM** - Read/write to internal EEPROM
- **Interrupt** - Using external interrupts
- **Millis_Micros** - Non-blocking timing with millis()/micros()
- **PWM** - Smooth hardware PWM LED fading with `analogWrite()`
- **PWM_Timer** - PWM on all three PWM pins while the hardware `Timer` interrupt runs
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
- **RAM:** 256 bytes internal RAM (shared with the stack) + 1 KB XRAM
- The IDE's "Global variables" figure only counts XRAM; watch internal RAM usage too
- Use `__xdata` keyword for large buffers to save internal RAM

### SDCC Specific
- This core uses SDCC compiler
- Some C++ features may be limited
- Use `__reentrant` on Serial functions with parameters to ensure interrupt-safe execution under SDCC.
- Global variables default to internal RAM (limited to 256 bytes)

### EEPROM Notes
The EEPROM is data flash, so it follows flash rules:
- `eraseSector(addr)` erases the whole 512-byte sector containing `addr` to `0xFF`.
- A write can only change bits from 1 to 0. To change a byte otherwise, erase its sector, then write back **every** value you still need in it.
- `write()`, `update()`, `eraseSector()` and `writeBlock()` return `false` if the data could not be stored.
- Each sector survives about 100,000 erases, so don't erase in a loop.

### PCA Sharing (PWM and Timer)
`analogWrite()` and `Timer` share one PCA counter, which runs at `F_CPU` and is never stopped once started. `Timer` uses no PCA module (it counts counter overflows), so:
- PWM works on all 3 pins (`P3_2`, `P3_3`, `P5_4`) while `Timer` is running.
- PWM frequency is always `F_CPU / 256` (~43.2 kHz at 11.0592 MHz). `Timer.setPeriod()`, `Timer.start()` and `Timer.stop()` don't affect PWM.
- `Timer` periods range from 1024 CPU cycles (~93 µs at 11.0592 MHz) to 100 s. Each tick may be up to 256 CPU cycles (~23 µs at 11.0592 MHz) early or late, but the average period is exact.
- If `Timer.start()` is called before `Timer.setPeriod()`, the period defaults to 10 ms.

### Interrupt Callbacks
Callbacks passed to `attachInterrupt()` and `Timer.attachInterrupt()` run inside an ISR: keep them short, make shared variables `volatile`, and put `#pragma nooverlay` before a callback that has local variables.

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