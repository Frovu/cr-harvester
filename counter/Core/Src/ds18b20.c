/*
 * MIT License
 * Copyright (c) 2021 Frovy  (https://github.com/frovu)
 */

#include "ds18b20.h"

__STATIC_INLINE void delay_us(volatile uint32_t us)
{
  us *= (HAL_RCC_GetHCLKFreq() / 1000000) / 9;
  while (us--);
}

uint8_t ow_reset_presence(void)
{
  OW_LOW();
  delay_us(500); // 480-960 according to datasheet
  OW_HIGH();
  delay_us(100);  // 60+ according to datasheet
  uint8_t presence = OW_READ();
  delay_us(500);  // 480+ according to datasheet
  return presence; // 0 for ds18b20 presence pulse
}

uint8_t ow_read_bit(void)
{
  OW_LOW();
  delay_us(2); // Tinit > 1 us
  OW_HIGH();
  delay_us(13); // Trdv = 15 us
  uint8_t bit = OW_READ();
  delay_us(46); // Tts + Trc = 61 us
  return bit;
}

uint8_t ow_read(void)
{
  uint8_t data = 0;
  for (uint32_t i = 0; i < 8; ++i) {
    data += ow_read_bit() << i;
  }
  return data;
}

void ow_write_bit(uint8_t bit)
{
  if (bit) { // WRITE 1
    OW_LOW();
    delay_us(5);
    OW_HIGH();
    delay_us(55);
  } else {   // WRITE 0
    OW_LOW();
    delay_us(60);
    OW_HIGH();
  }
  delay_us(3); // recovery time
}

void ow_write(uint8_t data)
{
  for (uint32_t i = 0; i < 8; ++i) {
    ow_write_bit(data >> i & 0x1);
  }
}

uint8_t ow_crc8(uint8_t *data, uint32_t len)
{
  uint8_t crc = 0, byte, mix;
  while (len--) {
    byte = *data++;
    for (uint8_t i = 8; i; --i) {
      mix = (crc ^ byte) & 0x01;
      crc >>= 1;
      if (mix) {
        crc ^= 0x8C;
      }
      byte >>= 1;
    }
  }
  return crc;
}

HAL_StatusTypeDef ds18b20_init(void)
{
  if (ow_reset_presence() != DS18B20_PRESENCE) {
    return HAL_ERROR;
  }
  ow_write(DS18B20_ROM_SKIP);   // Broadcast
  ow_write(DS18B20_MEM_WRITE);  // Write scratchpad
  ow_write(DS18B20_TH_DEFAULT); // Trigger high 80 *С
  ow_write(DS18B20_TL_DEFAULT); // Trigger low -40 *С
  ow_write(DS18B20_CONFIG_12BIT); // Max resolution
  return HAL_OK;
}

HAL_StatusTypeDef ds18b20_start_conversion(void) {
  if (ow_reset_presence() != DS18B20_PRESENCE) {
    return HAL_ERROR;
  }
  ow_write(DS18B20_ROM_SKIP);      // Broadcast
  ow_write(DS18B20_MEM_CONVERT_T); // Convert temperature
  return HAL_OK;
}

HAL_StatusTypeDef ds18b20_read_temperature(float *temperature) {
  if (ow_reset_presence() != DS18B20_PRESENCE) {
    return HAL_ERROR;
  }
  ow_write(DS18B20_ROM_SKIP);  // Broadcast
  ow_write(DS18B20_MEM_READ);  // Read scratchpad
  uint8_t data[8];
  for (uint32_t i = 0; i < 8; ++ i) {
    data[i] = ow_read();
  }
  uint8_t crc = ow_read();
  if (ow_crc8(data, 8) !=  crc) {
    return HAL_ERROR;
  }

  int32_t raw = data[1] << 8 | data[0];
  // if sign bits are set, account for two's compliment by subtracting 0x10000 (flip all bits and add 1, but negative)
  *temperature = (float) (raw & 0xF000 ? raw - 0x10000 : raw) / 16.0;

  return HAL_OK;
}
