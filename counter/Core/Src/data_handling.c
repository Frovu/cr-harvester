/*
*   Logic dedicated to storing and sending data lines
*/

#include "counter.h"

#define FIRST_DATA_PAGE CONFIG_FLASH_PAGES_COUNT // reserved couple of pages for device config
// if first 4 bytes of flash page contain this, they are counted as saved data lines
static const uint32_t data_line_signature = 0xdeadbeef;

#define STRUCT_SIZE sizeof(DataLine)
#define SIGNATURE_SIZE sizeof(data_line_signature)
#define CHUNK_SIZE sizeof(data_line_signature) + sizeof(DataLine)

extern uint16_t flags;
extern uint32_t cycle_counter;

DataLine * current_period = NULL;
DataLine * data_buffer[DATA_BUFFER_LEN];
DataLine data_line_buffer;

uint16_t buffer_periods_count = 0;
uint16_t flash_page_first = FIRST_DATA_PAGE;    // aka where-to-read (nothing useful before it)
uint16_t flash_pages_used = 0;    // number of lines stored in flash
uint16_t flash_page_pointer = FIRST_DATA_PAGE;  // pointer to what is assumed first writable unused page, aka where-to-write

uint8_t flash_buf[CHUNK_SIZE];
uint8_t * buffer = flash_buf;

void reset_flash();
uint8_t write_to_flash(const DataLine *dl, uint32_t timeout);
uint8_t read_from_flash(DataLine *dl, uint32_t timeout);

void init_read_flash()
{
  debug_printf("flash: init read start\r\n");
  uint32_t tickstart = HAL_GetTick();
  flash_pages_used = 0;
  flash_page_first = FIRST_DATA_PAGE;
  for (uint16_t page = 0; page < AT25_PAGES_COUNT; ++page)
  {
    at25_read_block(page * AT25_PAGE_SIZE, buffer, CHUNK_SIZE);
    uint32_t *signature = (uint32_t*) buffer;
    if (*signature == data_line_signature)
    { // found saved data line
      if (flash_pages_used == 0) {
        flash_page_first = page;
      }
      ++flash_pages_used;
    }
    if(page % 100 == 0) {
      LED_BLINK_INV(BOARD_LED, 3);
    }
  }
  flash_page_pointer = flash_page_first + flash_pages_used;
  debug_printf("flash: init read done in %lu ms\r\n", HAL_GetTick() - tickstart);
  debug_printf("flash: found %d records starting from page %d\r\n", flash_pages_used, flash_page_first);
}

uint8_t write_to_flash(const DataLine *dl, uint32_t timeout)
{
  if (NOT_SET(FLAG_FLASH_OK) || !at25_is_valid()) {
    if (IS_SET(FLAG_FLASH_OK))
      TOGGLE(FLAG_FLASH_OK);
    return 0;
  }
  uint32_t tickstart = HAL_GetTick();
  memcpy(buffer, &data_line_signature, SIGNATURE_SIZE);
  memcpy(buffer + SIGNATURE_SIZE, dl, STRUCT_SIZE);
  while ((flash_page_pointer < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
  {
    ++flash_page_pointer;
    FlashErrno res = at25_write_block(flash_page_pointer * AT25_PAGE_SIZE, buffer, CHUNK_SIZE, DEFAULT_TIMEOUT);
    if (res == FLASH_ERRNO_OK)
    { // flash write succeeded
      if (flash_pages_used == 0)
      {
        flash_page_first = flash_page_pointer - 1;
      }
      ++flash_pages_used;
      debug_printf("flash: successfully wrote page %d (%dms)\r\n", flash_page_pointer - 1, HAL_GetTick() - tickstart);
      return 1;
    }
    else if (res == FLASH_ERRNO_FAIL)
    {
      debug_printf("flash: write error on page %d\r\n", flash_page_pointer - 1);
    }
    else // flash timed out
    {
      debug_printf("flash: timed out page at %d\r\n", flash_page_pointer - 1);
      if (IS_SET(FLAG_FLASH_OK))
        TOGGLE(FLAG_FLASH_OK);
      return 0;
    }
  }
  debug_printf("flash: end of writing loop (p# = %d / %d)\r\n", flash_page_pointer, AT25_PAGES_COUNT);
  return 0; // run out of flash pages or timed out
}

uint8_t read_from_flash(DataLine *dl, uint32_t timeout) {
  if (NOT_SET(FLAG_FLASH_OK) || !at25_is_valid()) {
    if (IS_SET(FLAG_FLASH_OK))
      TOGGLE(FLAG_FLASH_OK);
    return 0;
  }
  uint32_t tickstart = HAL_GetTick();
  while ((flash_page_first < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
  {
    at25_read_block(flash_page_first * AT25_PAGE_SIZE, buffer, CHUNK_SIZE);
    uint32_t *signature = (uint32_t*) buffer;
    if (*signature == data_line_signature)
    { // found saved data line
      debug_printf("flash: successfuly read page %d\r\n", flash_page_first);
      memcpy(dl, buffer + SIGNATURE_SIZE, STRUCT_SIZE);
      return 1;
    }
    else
    {
      debug_printf("flash: skipping page %d\r\n", flash_page_first);
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
  debug_printf("flash: erasing data section\r\n");
  uint32_t tickstart = HAL_GetTick();
  at25_erase(FIRST_DATA_PAGE, flash_page_pointer);
  debug_printf("flash: erased in %lu ms\r\n", (uint32_t)(HAL_GetTick()-tickstart));
  flash_page_first = FIRST_DATA_PAGE;
  flash_pages_used = 0;
  flash_page_pointer = FIRST_DATA_PAGE;
}

void data_period_transition(const volatile uint16_t * counts, DateTime *dt, float t, float p)
{
  if (current_period)
  {
    debug_printf("period: counts b/f = %d / %d\r\n", buffer_periods_count, flash_pages_used);
    for (uint16_t i=0; i<CHANNELS_COUNT; ++i) {
      current_period->counts[i] = counts[i];
    }
    if (NOT_SET(FLAG_FLASH_OK))
    { // if flash is broken forget about data saved there
      flash_pages_used = 0;
    }
    if ((buffer_periods_count < DATA_BUFFER_LEN) && (flash_pages_used == 0))
    { // save data line to buffer
      data_buffer[buffer_periods_count] = current_period;
      ++buffer_periods_count;
      debug_printf("period: saved to buffer\r\n");
    }
    else if (IS_SET(FLAG_FLASH_OK)) // save data line to flash
    {
      // if buffer was not in flash, save it
      for (uint16_t i=0; i<buffer_periods_count; ++i) {
        write_to_flash(data_buffer[i], DEFAULT_TIMEOUT);
        free(data_buffer[i]);
      }
      buffer_periods_count = 0;
      write_to_flash(current_period, DEFAULT_TIMEOUT);
      // free data since it is now saved in flash (at least we hope so)
      free(current_period);
      debug_printf("period: saved to flash (%d) < %d / %d >\r\n", flash_pages_used, flash_page_first, flash_page_pointer);
    }
  }
  /*
  ******************* Start new period ******************
  */
  current_period = (DataLine*) malloc(STRUCT_SIZE);
  current_period->timestamp = mktime(dt);
  current_period->temperature = t;
  current_period->pressure = p;
  current_period->cycle = cycle_counter;
  uint32_t info = 0;
  if (NOT_SET(FLAG_TIME_TRUSTED)) {
    info |= 1;
  }
  current_period->info = info;
  debug_printf("period: start %lu\r\n", current_period->timestamp);
}

// returns
//    positive number if line sent but there is more remaining
//    negative number if failed to send
//    0 if sent all
DataStatus data_send_one(uint32_t timeout)
{
  // if flash_pages_used is >0, buffer_periods_count is 0
  if (buffer_periods_count + flash_pages_used <= 0) {
    return DATA_CLEAR;
  }
  DataLine *line_to_send;
  if (flash_pages_used == 0)
  { // data is stored in buffer only
    line_to_send = data_buffer[0];
  }
  else // read data from flash
  {
    line_to_send = &data_line_buffer;
    if (!read_from_flash(line_to_send, timeout))
    {
      debug_printf("dataline: failed to retrieve from flash\r\n");
      return DATA_FLASH_ERROR; // failed to read anything useful from flash, aborting
    }
  }

  debug_printf("()--> %lu / %.2f hPa / %.2f C ()-->\r\n", line_to_send->timestamp,
    line_to_send->pressure, line_to_send->temperature);
  DataStatus status = send_data_to_server(line_to_send, timeout);

  if (status == DATA_OK)
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
    if (flash_pages_used + buffer_periods_count <= 0) {
      status = DATA_CLEAR;
    }
    debug_printf("dataline: sent, counts b/f = %d / %d\r\n", buffer_periods_count, flash_pages_used);
  }
  else
  {
    debug_printf("dataline: failed to send\r\n");
  }
  return status;
}
