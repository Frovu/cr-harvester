#include "at25df321.h"

uint32_t flash_error_count = 0;

void at25_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin)
{
  at25_hspi = hspi;
  at25_gpio_bus = gpio_bus;
  at25_gpio_pin = gpio_pin;
  at25_unselect();
}

uint8_t at25_read_status_register()
{
  uint8_t buf;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_STATUS);
  at25_receive_byte(&buf);
  at25_unselect();
  return buf;
}

uint8_t at25_is_ready()
{
  return (at25_read_status_register() & AT25_BIT_READY) == 0;
}

uint8_t at25_write_ok() {
  return (at25_read_status_register() & AT25_BIT_WRITE_OK) == 0;
}

uint8_t at25_is_valid()
{
  uint8_t mfid = 0, devid = 0;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_DEV_ID);
  at25_receive_byte(&mfid);
  at25_receive_byte(&devid);
  at25_unselect();
  return mfid == 0x1F && devid == 0x47;
}

void at25_global_unprotect()
{
  at25_write_enable();
  at25_select();
  at25_transmit_byte(AT25_CMD_WRITE_STATUS_1);
  at25_transmit_byte(0x0); // SPRL and 5,4,3,2 bits unset
  at25_unselect();
}

FlashErrno at25_erase_block(uint32_t address, uint8_t command, uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();
  WAIT_READY();

  at25_write_enable();
  at25_select();
  at25_transmit_byte(command);
  at25_transmit_byte((address >> 16) & 0x3F);
  at25_transmit_byte((address >> 8)  & 0xFF);
  at25_transmit_byte( address        & 0xFF);
  at25_unselect();

  WAIT_READY();
  uint8_t write_ok = at25_write_ok();
  if (!write_ok)
    flash_error_count++;
  return write_ok ? FLASH_ERRNO_OK : FLASH_ERRNO_FAIL;
}

FlashErrno at25_write_block(uint32_t address, uint8_t *data, uint16_t count, uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();
  WAIT_READY();

  at25_write_enable();
  at25_select();
  at25_transmit_byte(AT25_CMD_BYTE_PROGRAM);
  at25_transmit_byte((address >> 16) & 0x3F);
  at25_transmit_byte((address >> 8)  & 0xFF);
  at25_transmit_byte( address        & 0xFF);
  for (uint16_t i=0; i < count; ++i) {
    at25_transmit_byte(data[i]);
  }
  at25_unselect();

  WAIT_READY();
  uint8_t write_ok = at25_write_ok();
  if (!write_ok)
    flash_error_count++;
  return write_ok ? FLASH_ERRNO_OK : FLASH_ERRNO_FAIL;
}

void at25_read_block(uint32_t address, uint8_t *data, uint16_t count)
{
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_ARRAY);
  at25_transmit_byte((address >> 16) & 0x3F);
  at25_transmit_byte((address >> 8)  & 0xFF);
  at25_transmit_byte( address        & 0xFF);

  for (uint16_t i=0; i < count; ++i) {
    at25_receive_byte(&data[i]);
  }
  at25_unselect();
}


/* "Intelligent" erase algorithm uses 4 or 64 Kbyte erase commands to erase
*  pages from A to B, if to_page is == 0 it does not care about last pages.
*  function is blocking and may take up to 64 seconds (!)
*  if any operation takes more than 3 seconds it aborts
*/
void at25_erase(uint16_t from_page, uint16_t to_page)
{
  #define PAGES_PER_64K 256
  #define PAGES_PER_4K   16
  uint16_t erased;
  uint8_t cmd;
  for (uint16_t page = from_page; page < to_page; page+=erased)
  {
    if ((to_page == 0) || (to_page - from_page > PAGES_PER_4K)) {
      cmd = AT25_CMD_ERASE_64K;
      erased = PAGES_PER_64K;
    } else {
      cmd = AT25_CMD_ERASE_4K;
      erased = PAGES_PER_4K;
    }
    FlashErrno res = at25_erase_block(page * AT25_PAGE_SIZE, cmd, AT25_ERASE_TIMEOUT);
    if (res == FLASH_ERRNO_OK) {
      #ifdef DEBUG_UART
      debug_printf("flash: erase %u succeeded on addr %u\r\n", erased, page*AT25_PAGE_SIZE);
      #endif
    } else if (res == FLASH_ERRNO_TIMEOUT) {
      #ifdef DEBUG_UART
      debug_printf("flash: erase %u timed out on addr %u\r\n", erased, page*AT25_PAGE_SIZE);
      #endif
      return;
    }
  }
}
