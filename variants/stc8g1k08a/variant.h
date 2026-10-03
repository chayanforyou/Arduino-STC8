#ifndef _VARIANT_STC8G1K08A_H_
#define _VARIANT_STC8G1K08A_H_

#include <stdint.h>
#include "drivers/inc/gpio.h"
#include "drivers/inc/timer.h"

// STC8G1K08A Register Definitions
__sfr __at(0x80) P0;        // Port 0 data register
__sfr __at(0x81) SP;        // Stack Pointer
__sfr __at(0x82) DPL;       // Data Pointer Low byte
__sfr __at(0x83) DPH;       // Data Pointer High byte
__sfr __at(0x87) PCON;      // Power Control register
__sfr __at(0x88) TCON;      // Timer 0/1 Control register
__sfr __at(0x89) TMOD;      // Timer 0/1 Mode register
__sfr __at(0x8A) TL0;       // Timer 0 Low byte
__sfr __at(0x8B) TL1;       // Timer 1 Low byte
__sfr __at(0x8C) TH0;       // Timer 0 High byte
__sfr __at(0x8D) TH1;       // Timer 1 High byte
__sfr __at(0x8E) AUXR;      // Auxiliary Register 1
__sfr __at(0x8F) INTCLKO;   // External interrupt / clock output control register
__sfr __at(0x90) P1;        // Port 1 data register
__sfr __at(0x91) P1M1;      // Port 1 Mode register 1 (configuration)
__sfr __at(0x92) P1M0;      // Port 1 Mode register 0 (configuration)
__sfr __at(0x93) P0M1;      // Port 0 Mode register 1 (configuration)
__sfr __at(0x94) P0M0;      // Port 0 Mode register 0 (configuration)
__sfr __at(0x98) SCON;      // UART1 Control register
__sfr __at(0x99) SBUF;      // UART1 Data buffer register
__sfr __at(0x9D) IRCBAND;   // IRC Band selection register
__sfr __at(0x9E) LIRTRIM;   // IRC Frequency fine trim register
__sfr __at(0x9F) IRTRIM;    // IRC Frequency adjustment register
__sfr __at(0xA0) P2;        // Port 2 data register
__sfr __at(0xA2) P_SW1;     // Peripheral port switch register 1
__sfr __at(0xA8) IE;        // Interrupt Enable register
__sfr __at(0xA9) SADDR;     // UART1 Slave address register
__sfr __at(0xB0) P3;        // Port 3 data register
__sfr __at(0xB1) P3M1;      // Port 3 Mode register 1 (configuration)
__sfr __at(0xB2) P3M0;      // Port 3 Mode register 0 (configuration)
__sfr __at(0xB8) IP;        // Interrupt Priority register
__sfr __at(0xB9) SADEN;     // UART1 Slave address mask register
__sfr __at(0xBA) P_SW2;     // Peripheral port switch register 2 (EAXFR access)
__sfr __at(0xC2) IAP_DATA;  // Flash / EEPROM data register
__sfr __at(0xC3) IAP_ADDRH; // Flash / EEPROM address high byte
__sfr __at(0xC4) IAP_ADDRL; // Flash / EEPROM address low byte
__sfr __at(0xC5) IAP_CMD;   // Flash / EEPROM command register (1=Read, 2=Write, 3=Erase)
__sfr __at(0xC6) IAP_TRIG;  // Flash / EEPROM trigger register (Write 0x5A, 0xA5)
__sfr __at(0xC7) IAP_CONTR; // Flash / EEPROM control register
__sfr __at(0xC8) P5;        // Port 5 data register
__sfr __at(0xC9) P5M1;      // Port 5 Mode register 1 (configuration)
__sfr __at(0xCA) P5M0;      // Port 5 Mode register 0 (configuration)
__sfr __at(0xD0) PSW;       // Program Status Word
__sfr __at(0xE0) ACC;       // Accumulator
__sfr __at(0xEF) AUXINTIF;  // Auxiliary Interrupt Flags
__sfr __at(0xF0) B;         // B register (multiplication/division)
__sfr __at(0xF5) IAP_TPS;   // Flash / EEPROM timing parameter register

// Extended SFRs (XDATA area, requires EAXFR bit in P_SW2 set to 1)
__xdata __at(0xFE00) volatile uint8_t CKSEL;    // Clock source selection register
__xdata __at(0xFE01) volatile uint8_t CLKDIV;   // Clock division register
__xdata __at(0xFE02) volatile uint8_t HIRCCR;   // Internal High-speed IRC Control register
__xdata __at(0xFE03) volatile uint8_t XOSCCR;   // External Crystal Oscillator Control register
__xdata __at(0xFE04) volatile uint8_t IRC32KCR; // Internal 32KHz IRC Control register
__xdata __at(0xFE05) volatile uint8_t MCLKOCR;  // Master Clock Output Control register
__xdata __at(0xFE13) volatile uint8_t P3PU;     // Port 3 internal pull-up resistor enable
__xdata __at(0xFE15) volatile uint8_t P5PU;     // Port 5 internal pull-up resistor enable
__xdata __at(0xFE33) volatile uint8_t P3IE;     // Port 3 digital input enable register
__xdata __at(0xFE35) volatile uint8_t P5IE;     // Port 5 digital input enable register

// P_SW2 (Peripheral Port Switch Register 2) bit definitions
#define EAXFR 0x80 // Enable Extended SFR access in XDATA space (FE00H-FEFFH)

// UART1 pin switch selection (P_SW1[7:6]) for STC8G1K08A
#define S1_S_P30_P31 0x00 // RxD=P3.0, TxD=P3.1 (default)
#define S1_S_P32_P33 0x40 // RxD=P3.2, TxD=P3.3
#define S1_S_P54_P55 0x80 // RxD=P5.4, TxD=P5.5

// Helper bitmasks
#define MASK_TWO_BITS_HIGH 0x03 // 2-bit mask

// UART Mode definitions
#define UART_MODE_1 0x01 // 8-bit UART, variable baud rate (Timer-generated)

// Port 3 pin numbers
#define P3_0 0 // Pin P3.0
#define P3_1 1 // Pin P3.1
#define P3_2 2 // Pin P3.2
#define P3_3 3 // Pin P3.3

// Port 5 pin numbers
#define P5_4 4 // Pin P5.4
#define P5_5 5 // Pin P5.5

// Port identifiers
#define PORT3 3 // Port 3 ID
#define PORT5 5 // Port 5 ID

// Interrupt Vector numbers for SDCC __interrupt(n)
#define INT0_ISR_VECTOR   0  // External Interrupt 0 (P3.2)
#define TIMER0_ISR_VECTOR 1  // Timer 0 Overflow Interrupt
#define INT1_ISR_VECTOR   2  // External Interrupt 1 (P3.3)
#define UART1_ISR_VECTOR  4  // UART1 Serial Interrupt (RI / TI)
#define INT2_ISR_VECTOR   10 // External Interrupt 2 (P5.4 / P3.6 falling edge)
#define INT3_ISR_VECTOR   11 // External Interrupt 3 (P5.5 / P3.7 falling edge)
#define INT4_ISR_VECTOR   16 // External Interrupt 4 (P3.0 falling edge)

#endif // _VARIANT_STC8G1K08A_H_