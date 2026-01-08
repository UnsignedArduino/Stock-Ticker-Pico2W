#include <Arduino.h>
#include <MD_MAX72xx.h>

const MD_MAX72XX::moduleType_t HARDWARE_TYPE = MD_MAX72XX::FC16_HW;

#define USE_HARDWARE_SPI
const uint8_t CLK_PIN = 2;
const uint8_t DATA_PIN = 3;
const uint8_t CS_PIN = 5;

const uint8_t CONFIG_BTN_PIN = 22;
