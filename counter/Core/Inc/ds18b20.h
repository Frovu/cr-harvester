/*
 * MIT License
 * Copyright (c) 2021 Frovy  (https://github.com/frovu)
 *
 * A DS18B20 driver for single sensor on bus
 */

#ifndef DS18B20_H_
#define DS18B20_H_

#include "counter.h"

/* ********************************************************************* */
/* ************** To change ow pin manually change these *************** */
/* ********************************************************************* */
#define OW_GPIO GPIOB
#define OW_INIT() \
  HAL_GPIO_DeInit(OW_GPIO, GPIO_PIN_12); \
  OW_GPIO->CRH |= GPIO_CRH_MODE12;    \
  OW_GPIO->CRH |= GPIO_CRH_CNF12_0; \
  OW_GPIO->CRH &= ~GPIO_CRH_CNF12_1
#define OW_HIGH() \
  OW_GPIO->ODR |= GPIO_ODR_ODR12
#define OW_LOW() \
  OW_GPIO->ODR &= ~GPIO_ODR_ODR12
#define OW_READ() \
  (OW_GPIO->IDR & GPIO_IDR_IDR12 ? 1 : 0)
/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */

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
void    ow_write_bit(uint8_t bit);
void    ow_write(uint8_t byte);
uint8_t ow_crc8(uint8_t *data, uint32_t len);

HAL_StatusTypeDef ds18b20_init(void);
HAL_StatusTypeDef ds18b20_start_conversion(void);
HAL_StatusTypeDef ds18b20_read_temperature(float *temperature);

#endif /* DS18B20_H_ */
