/*
*   Logic dedicated to storing and sending data lines
*/

#include "couter.h"

// if first 4 bytes of flash page contain this, they are counted as saved data lines
static const uint32_t data_line_signature = 0xdeadbeef;

static const uint16_t struct_size = sizeof(DataLine);
static const uint16_t signature_size = sizeof(data_line_signature);
static const uint16_t chunk_size  = sizeof(data_line_signature) + struct_size;

extern uint16_t flags;

DataLine * current_period = NULL;
DataLine * data_buffer[DATA_BUFFER_LEN];

uint16_t buffer_periods_count = 0;
uint16_t flash_page_first = 0;    // aka where-to-read (nothing useful before it)
uint16_t flash_pages_used = 0;    // number of lines stored in flash
uint16_t flash_page_pointer = 0;  // pointer to what is assumed first writable unused page, aka where-to-write

uint8_t buffer[chunk_size];

uint8_t write_to_flash(const DataLine *dl, uint32_t timeout);
uint8_t read_from_flash(DataLine *dl, uint32_t timeout);

void init_read_flash()
{
  flash_pages_used = 0;
  flash_page_first = 0;
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
  flash_page_pointer = flash_page_first + flash_pages_used;
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
  memcpy(buffer, data_line_signature, signature_size);
  memcpy(buffer + signature_size, dl, struct_size);
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
  return 0; // run out of flash pages or timed out
}

uint8_t read_from_flash(DataLine *dl, uint32_t timeout) {
  if (!at25_is_valid()) {
    if (IS_SET(FLAG_FLASH_OK)) {
      TOGGLE(FLAG_FLASH_OK);
    }
    return 0;
  }
  while ((flash_page_first < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
  {
    at25_read_block(flash_page_first * AT25_PAGE_SIZE, buffer, chunk_size);
    uint32_t *signature = (uint32_t*) buffer;
    if (*signature == data_line_signature)
    { // found saved data line
      memcpy(dl, buffer + signature_size, struct_size);
      return 1;
    }
    else
    {
      ++flash_page_first;
    }
  }
  if (flash_page_first >= AT25_PAGES_COUNT)
  { // data not found, flash is assumed to be empty
    reset_flash();
  }
  return 0;
}

void reset_flash() {
  at25_erase_all(); // complete operation takes about 1 minute
  flash_page_first = 0;
  flash_pages_used = 0;
  flash_page_pointer = 0;
}

void data_period_transition(const uint16_t * counts, const DateTime *dt, float t, float p)
{
  if (current_period)
  {
    for (uint16_t i=0; i<CHANNELS_COUNT; ++i) {
      current_period->counts[i] = counts[i];
    }
    if ((buffer_periods_count < DATA_BUFFER_LEN) && (flash_pages_used == 0))
    { // save data line to buffer
      data_buffer[buffer_periods_count] = current_period;
      ++buffer_periods_count;
    }
    else // save data line to flash
    {
      // if buffer was not in flash, save it
      for (uint16_t i=0; i<buffer_periods_count; ++i) {
        write_to_flash(data_buffer[i], DEFAULT_TIMEOUT);
      }
      buffer_periods_count = 0;
      write_to_flash(current_period, DEFAULT_TIMEOUT);
      // free data since it is now saved in flash (at least we hope so)
      free(current_period);
    }
  }
  /*
  ******************* Start new period ******************
  */
  current_period = malloc(struct_size);
  current_period->timestamp = mktime(dt);
  current_period->temperature = t;
  current_period->pressure = p;

}

uint16_t data_send_one(uint32_t timeout)
{
  uint16_t total_count = flash_pages_used ? flash_pages_used : buffer_periods_count;
  if (total_count > 0)
  {
    DataLine *line_to_send;
    if (flash_pages_used == 0)
    { // data is stored in buffer only
      line_to_send = data_buffer[0];
    }
    else // read data from flash
    {
      line_to_send = malloc(struct_size);
      if (!read_from_flash(line_to_send, timeout))
      {
        return total_count; // failed to read anything useful from flash, aborting
      }
    }
    // TODO: prepare and send data string over HTTP
    uint8_t status = 0;
    if (status)
    {
      if (flash_pages_used == 0)
      { // data was taken from buffer, free and shift buffer to the left
        free(line_to_send);
        --buffer_periods_count;
        for (uint16_t i = 0; i < buffer_periods_count; ++i) {
          data_buffer[i] = data_buffer[i + 1];
        }
      }
      else
      { // data was taken from flash, decrement flash counter and erase if nothing left on chip
        --flash_pages_used;
        ++flash_page_first;
        if (flash_pages_used == 0)
        {
          reset_flash();
        }

      }
    }
  }
  return total_count;
}