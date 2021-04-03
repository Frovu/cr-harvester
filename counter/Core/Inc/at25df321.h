/*
* Based on https://github.com/Synapseware/libs/blob/master/drivers/at25df321/at25df321.c
*/

#ifndef __AT25DF321_H
#define __AT25DF321_H

#include "stm32f1xx_hal.h"

#define AT25_TIMEOUT 250u

void init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin);
inline void at25_select();
inline void at25_unselect();
inline HAL_StatusTypeDef at25_transmit_byte(uint8_t *char);
inline HAL_StatusTypeDef at25_receive_byte(uint8_t *char);
inline void at25_write_enable();
inline void at25_write_disable();
uint8_t at25_read_status_register();
uint8_t at25_is_ready();
uint8_t at25_write_ok();
uint8_t at25_is_valid();

// protection and block erase operations are not used hence not implemented here

void at25_erase_all()
uint8_t at25_write_block(uint32_t address, const uint8_t *data, uint16_t count)
uint16_t at25_read_block(uint32_t address, const uint8_t *data, uint16_t count)

#endif
