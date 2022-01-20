/*
 * MIT License
 * Copyright (c) 2021 Frovy  (https://github.com/frovu)
 *
 * A DS18B20 driver for single sensor on bus
 */

#ifndef DS18B20_H_
#define DS18B20_H_

/* The pin defined in main.h */
#define _OW_GPIO_PIN OW_DS18B20

#define _OW_GPIO _OW_GPIO_PIN##_GPIO_Port
#define _OW_PIN  _OW_GPIO_PIN##_Pin

#define OW_INIT() \
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_##_OW_PIN); \
  _OW_GPIO->CRH |= GPIO_CRH_MODE##_OW_PIN;       \
  _OW_GPIO->CRH |= GPIO_CRH_CNF##_OW_PIN##_0;      \
  _OW_GPIO->CRH &= ~GPIO_CRH_CNF##_OW_PIN##_1
#define OW_HIGH() \
  _OW_GPIO->ODR |= GPIO_ODR_ODR##_OW_PIN
#define OW_LOW() \
  _OW_GPIO->ODR &= ~GPIO_ODR_ODR##_OW_PIN
#define OW_READ() \
  (_OW_GPIO->IDR & GPIO_IDR_IDR##_OW_PIN ? 1 : 0)

__STATIC_INLINE void delay_us(volatile uint32_t us)
{
  uint32_t start_cycle = DWT->CYCCNT;
  us *= HAL_RCC_GetHCLKFreq() / 1000000;
  while ((DWT->CYCCNT - start_cycle) < us);
}

#define DS18B20_PRESENCE 0x0

#define DS18B20_ROM_READ      0x33
#define DS18B20_ROM_MATCH     0x55
#define DS18B20_ROM_SKIP      0xCC
#define DS18B20_ROM_SEARCH    0xF0
#define DS18B20_ROM_ALARM     0xEC

#define DS18B20_MEM_WRITE     0x4E
#define DS18B20_MEM_READ      0xBE
#define DS18B20_MEM_COPY      0x48
#define DS18B20_MEM_CONVERT_T 0x44
#define DS18B20_MEM_RECALL    0xB8
#define DS18B20_MEM_SUPPLY    0xB4

#define DS18B20_RESOLUTION_SHIFT 0x5

#define DS18B20_CONFIG_9BIT  (0x0 << DS18B20_RESOLUTION_SHIFT)
#define DS18B20_CONFIG_10BIT (0x1 << DS18B20_RESOLUTION_SHIFT)
#define DS18B20_CONFIG_11BIT (0x2 << DS18B20_RESOLUTION_SHIFT)
#define DS18B20_CONFIG_12BIT (0x3 << DS18B20_RESOLUTION_SHIFT)

#define DS18B20_TH_DEFAULT 0x50 // +80 *C
#define DS18B20_TL_DEFAULT 0xA8 // -40 *C

uint8_t ow_reset_presence(void);
uint8_t ow_read_bit(void);
uint8_t ow_read(void);
uint8_t ow_write_bit(uint8_t bit);
uint8_t ow_write(uint8_t byte);
uint8_t ow_crc8(uint8_t *data, uint32_t len);

HAL_StatusTypeDef ds18b20_init(void);
HAL_StatusTypeDef ds18b20_start_conversion(void);
HAL_StatusTypeDef ds18b20_read_temperature(float *temperature);

#endif /* DS18B20_H_ */
