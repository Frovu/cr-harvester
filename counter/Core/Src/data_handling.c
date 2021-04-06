/*
*   Logic dedicated to storing and sending data lines
*/

#include "couter.h"

// if first 4 bytes of flash page contain this, they are counted as saved data lines
static const uint32_t data_line_signature = 0xdeadbeef;

static const uint16_t struct_size = sizeof(DataLine);
static const uint16_t chunk_size  = sizeof(data_line_signature) + struct_size;

extern uint16_t flags;

DataLine * current_period = NULL;
DataLine * data_buffer[DATA_BUFFER_LEN];

uint16_t saved_periods_count = 0;
uint16_t flash_page_first = 0;
uint16_t flash_pages_used = 0;
uint16_t flash_page_pointer = 0;  // pointer to what is assumed first writable empty page

uint8_t buffer[chunk_size];

void init_read_flash()
{
  for (uint16_t page = 0; page < AT25_PAGES_COUNT; ++page)
  {
    at25_read_block(page * AT25_PAGE_SIZE, buffer, chunk_size);
    uint32_t *signature = (uint32_t*) buffer;
    if (*signature == data_line_signature)
    { // found saved data line
      if (flash_pages_used == 0) {
        flash_page_first = page;
      }
      ++flash_pages_used;
    }
  }
  saved_periods_count += flash_pages_used;
  flash_page_pointer = flash_page_first + flash_pages_used;
  if (flash_pages_used != 0) {
    RAISE(FLAG_FLASH_NOT_EMPTY);
  }
  debug_printf("found %d records in flash starting from page %d", flash_pages_used, flash_page_first);
}

uint8_t write_to_flash(const DataLine *dl, uint32_t timeout)
{
  if (!at25_is_valid()) {
    if (IS_SET(FLAG_FLASH_OK)) {
      TOGGLE(FLAG_FLASH_OK);
    }
    return 0;
  }
  uint32_t tickstart = HAL_GetTick();
  memcpy(buffer, data_line_signature, sizeof(data_line_signature));
  memcpy((buffer + sizeof(data_line_signature)), dl, struct_size);
  while ((flash_page_pointer < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
  {
    while (!at25_is_ready()) {
      if (HAL_GetTick() - tickstart < timeout) {
        return 0;
      }
      HAL_Delay(30);
    }
    at25_write_block(flash_page_pointer * AT25_PAGE_SIZE, buffer, chunk_size);
    ++flash_page_pointer;
    while (!at25_is_ready()) {
      if (HAL_GetTick() - tickstart < timeout) {
        return 0;
      }
      HAL_Delay(10);
    }
    if (at25_write_ok())
    { // flash write succeeded
      if (flash_pages_used == 0)
      {
        flash_page_first = flash_page_pointer - 1;
      }
      ++flash_pages_used;
      return 1;
    }
    else
    {
      debug_printf("failed to write flash page %d", flash_page_pointer - 1);
    }
  }
}

void data_end_period(const uint16_t * counts)
{
  if (current_period)
  {
    for (uint16_t i=0; i<CHANNELS_COUNT; ++i) {
      current_period->counts[i] = counts[i];
    }
    if (saved_periods_count < DATA_BUFFER_LEN)
    { // save data line to buffer
      data_buffer[saved_periods_count] = current_period;
      ++saved_periods_count;
    }
    else // save data line to flash
    {
      RAISE(FLAG_FLASH_NOT_EMPTY);
      if (NOT_SET(FLAG_FLASH_BUFFER))
      { // if buffer was not in flash, save it
        RAISE(FLAG_FLASH_BUFFER);
        for(uint16_t i=0; i<CHANNELS_COUNT; ++i)
        {
          write_to_flash(data_buffer[i], DEFAULT_TIMEOUT);
        }
      }
      write_to_flash(current_period, DEFAULT_TIMEOUT);
    }
  }
}

void data_new_period(const DateTime *dt, float t, float p)
{

}

void data_send_one(uint32_t timeout);
//uint8_t read_line_if_any();
