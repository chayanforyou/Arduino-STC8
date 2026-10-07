#include "Arduino.h"
#include "variant.h"

void analogWrite(uint8_t pin, uint8_t val) {
  // The PCA counter is shared with Timer (which only uses its overflow, no
  // module) and is never stopped once running; both use PCA_CPS_SYSCLK, so
  // PWM frequency is always F_CPU / 256 (~43.2 kHz at 11.0592 MHz).
  if (!READ_BIT(CCON, 6)) {
    CMOD = (CMOD & ~0x0E) | PCA_CPS_SYSCLK;  // Keeps Timer's ECF bit
    SET_BIT(CCON, 6);                        // CR = 1 (Start PCA counter)
  }

  switch (pin) {
    case P3_2:  // PCA Channel 0 (CCP0)
      pinMode(P3_2, OUTPUT);
      if (val == 0) {
        CCAPM0 = 0x00;  // Disable PWM mode on CCP0
        digitalWrite(P3_2, LOW);
      } else if (val == 255) {
        CCAPM0 = 0x00;  // Disable PWM mode on CCP0
        digitalWrite(P3_2, HIGH);
      } else {
        uint8_t threshold = (uint8_t)(256 - val);
        PCA_PWM0 = 0x00;  // 8-bit PWM mode
        CCAP0L = threshold;
        CCAP0H = threshold;
        CCAPM0 = 0x42;  // ECOM0=1, PWM0=1 (8-bit PWM output)
      }
      break;

    case P3_3:  // PCA Channel 1 (CCP1)
      pinMode(P3_3, OUTPUT);
      if (val == 0) {
        CCAPM1 = 0x00;  // Disable PWM mode on CCP1
        digitalWrite(P3_3, LOW);
      } else if (val == 255) {
        CCAPM1 = 0x00;  // Disable PWM mode on CCP1
        digitalWrite(P3_3, HIGH);
      } else {
        uint8_t threshold = (uint8_t)(256 - val);
        PCA_PWM1 = 0x00;  // 8-bit PWM mode
        CCAP1L = threshold;
        CCAP1H = threshold;
        CCAPM1 = 0x42;  // ECOM1=1, PWM1=1 (8-bit PWM output)
      }
      break;

    case P5_4:  // PCA Channel 2 (CCP2)
      pinMode(P5_4, OUTPUT);
      if (val == 0) {
        CCAPM2 = 0x00;  // Disable PWM mode on CCP2
        digitalWrite(P5_4, LOW);
      } else if (val == 255) {
        CCAPM2 = 0x00;  // Disable PWM mode on CCP2
        digitalWrite(P5_4, HIGH);
      } else {
        uint8_t threshold = (uint8_t)(256 - val);
        PCA_PWM2 = 0x00;  // 8-bit PWM mode
        CCAP2L = threshold;
        CCAP2H = threshold;
        CCAPM2 = 0x42;  // ECOM2=1, PWM2=1 (8-bit PWM output)
      }
      break;

    default:
      // Fallback for non-PWM pins (digital on/off threshold)
      pinMode(pin, OUTPUT);
      digitalWrite(pin, (val >= 128) ? HIGH : LOW);
      break;
  }
}

int analogRead(uint8_t pin) __reentrant {
  uint8_t channel;
  switch (pin) {
    case P3_0:
      channel = 0;
      P3M0 &= ~0x01;
      P3M1 |= 0x01;  // High-impedance input mode
      break;
    case P3_1:
      channel = 1;
      P3M0 &= ~0x02;
      P3M1 |= 0x02;
      break;
    case P3_2:
      channel = 2;
      P3M0 &= ~0x04;
      P3M1 |= 0x04;
      break;
    case P3_3:
      channel = 3;
      P3M0 &= ~0x08;
      P3M1 |= 0x08;
      break;
    case P5_4:
      channel = 4;
      P5M0 &= ~0x10;
      P5M1 |= 0x10;
      break;
    case P5_5:
      channel = 5;
      P5M0 &= ~0x20;
      P5M1 |= 0x20;
      break;
    default:
      if (pin == 15) {
        channel = 15;  // Internal 1.19V Bandgap Reference (BGV)
      } else {
        return 0;
      }
      break;
  }

  // Enable Extended SFR to configure ADCTIM
  P_SW2 |= EAXFR;
  ADCTIM = 0x3F;  // CSSETUP=0, CSHOLD=1, SMPDUTY=31 (stable sampling time)
  P_SW2 &= ~EAXFR;

  // Configure ADC: Right-aligned 10-bit result (RESFMT=1), ADC Clock = SYSclk/2/16 (SPEED=0x0F)
  ADCCFG = 0x2F;

  // Power on ADC and select channel
  if (!(ADC_CONTR & 0x80)) {
    ADC_CONTR = 0x80 | channel;
    delay_ms(1);  // Wait for ADC power to stabilize
  } else {
    ADC_CONTR = 0x80 | channel;
  }

  // Start conversion (ADC_START = bit 6)
  ADC_CONTR |= 0x40;
    __asm
        nop
        nop
    __endasm;

  // Wait for conversion completion (ADC_FLAG = bit 5)
  while (!(ADC_CONTR & 0x20))
    ;

  // Clear completion flag
  ADC_CONTR &= ~0x20;

  // Return 10-bit right-aligned result (0 to 1023)
  return (int)(((uint16_t)ADC_RES << 8) | ADC_RESL);
}
