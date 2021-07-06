/* Todo: Hardware specific pin declaration. 
 * Or: Use platform-independent pin configs like "D0" and SPI.
 */

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>

// auto-detetct pin-mapping for known boards, otherwise use defaults
// SPI is always assumed to be the standard Arduino SPI instance with default pins.
#if defined(ARDUINO_SAMD_FEATHER_M0)
#define RADIO_NSS 8
#define RADIO_DIO0 6
// assumes external jumpers [feather_lora_jumper]
#define RADIO_DIO1 5
#define RADIO_RST 4
#elif defined(ARDUINO_ARCH_ESP32)
/* IOx definitions for ESP32 boards. SPI is standard one on 
   MOSI=23, MISO=19, SCLK=18. */
#define RADIO_NSS 5
#define RADIO_DIO0 17
#define RADIO_DIO1 16
#define RADIO_RST 4
#elif defined(ARDUINO_ARCH_AVR)
/* AVR type microcontrollers */
#define RADIO_NSS 10
#define RADIO_DIO0 3
#define RADIO_DIO1 4
#define RADIO_RST 5
#else 
/* for all others (STM32, etc), use Arduino pin name descriptions */
#define RADIO_NSS D10
#define RADIO_DIO0 D3
#define RADIO_DIO1 D4
#define RADIO_RST D5
#endif

// Adapted for Feather M0 per p.10 of [feather]
const lmic_pinmap lmic_pins = {
    .nss = RADIO_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RADIO_RST,
    .dio = {RADIO_DIO0, RADIO_DIO1, LMIC_UNUSED_PIN}, 
};
