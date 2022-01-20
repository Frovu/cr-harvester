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
DataLine * last_period = NULL;
DataLine * data_buffer[DATA_BUFFER_LEN];
DataLine data_line_buffer;

uint16_t buffer_periods_count = 0;
uint16_t flash_page_first = FIRST_DATA_PAGE;    // aka where-to-read (nothing useful before it)
uint16_t flash_pages_used = 0;    // number of lines stored in flash
uint16_t flash_page_pointer = FIRST_DATA_PAGE;  // pointer to what is assumed first writable unused page, aka where-to-write

uint8_t flash_buf[CHUNK_SIZE];
uint8_t * buffer = flash_buf;
//
// void reset_flash();
// uint8_t write_to_flash(const DataLine *dl, uint32_t timeout);
// uint8_t read_from_flash(DataLine *dl, uint32_t timeout);
//
// void init_read_flash()
// {
//   debug_printf("flash: init read start\r\n");
//   uint32_t tickstart = HAL_GetTick();
//   flash_pages_used = 0;
//   flash_page_first = FIRST_DATA_PAGE;
//   for (uint16_t page = 0; page < AT25_PAGES_COUNT; ++page)
//   {
//     at25_read_block(page * AT25_PAGE_SIZE, buffer, CHUNK_SIZE);
//     uint32_t *signature = (uint32_t*) buffer;
//     if (*signature == data_line_signature)
//     { // found saved data line
//       if (flash_pages_used == 0) {
//         flash_page_first = page;
//       }
//       ++flash_pages_used;
//     }
//     if(page % 100 == 0) {
//       LED_BLINK_INV(BOARD_LED, 3);
//     }
//   }
//   flash_page_pointer = flash_page_first + flash_pages_used;
//   debug_printf("flash: init read done in %lu ms\r\n", HAL_GetTick() - tickstart);
//   debug_printf("flash: found %d records starting from page %d\r\n", flash_pages_used, flash_page_first);
// }
//
// uint8_t write_to_flash(const DataLine *dl, uint32_t timeout)
// {
//   if (NOT_SET(FLAG_FLASH_OK) || !at25_is_valid()) {
//     if (IS_SET(FLAG_FLASH_OK))
//       TOGGLE(FLAG_FLASH_OK);
//     return 0;
//   }
//   uint32_t tickstart = HAL_GetTick();
//   memcpy(buffer, &data_line_signature, SIGNATURE_SIZE);
//   memcpy(buffer + SIGNATURE_SIZE, dl, STRUCT_SIZE);
//   while ((flash_page_pointer < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
//   {
//     FlashErrno res = at25_write_block(flash_page_pointer * AT25_PAGE_SIZE, buffer, CHUNK_SIZE, DEFAULT_TIMEOUT);
//     ++flash_page_pointer;
//     if (res == FLASH_ERRNO_OK)
//     { // flash write succeeded
//       if (flash_pages_used == 0)
//       {
//         flash_page_first = flash_page_pointer - 1;
//       }
//       ++flash_pages_used;
//       debug_printf("flash: successfully wrote page %d (%dms)\r\n", flash_page_pointer-1, HAL_GetTick() - tickstart);
//       return 1;
//     }
//     else if (res == FLASH_ERRNO_FAIL)
//     {
//       debug_printf("flash: write error on page %d\r\n", flash_page_pointer-1);
//     }
//     else // flash timed out
//     {
//       debug_printf("flash: timed out page at %d\r\n", flash_page_pointer-1);
//       if (IS_SET(FLAG_FLASH_OK))
//         TOGGLE(FLAG_FLASH_OK);
//       return 0;
//     }
//   }
//   debug_printf("flash: end of writing loop (p# = %d / %d)\r\n", flash_page_pointer, AT25_PAGES_COUNT);
//   return 0; // run out of flash pages or timed out
// }
//
// uint8_t read_from_flash(DataLine *dl, uint32_t timeout) {
//   if (NOT_SET(FLAG_FLASH_OK) || !at25_is_valid()) {
//     if (IS_SET(FLAG_FLASH_OK))
//       TOGGLE(FLAG_FLASH_OK);
//     // if flash is broken forget about data saved there
//     flash_pages_used = 0;
//     return 0;
//   }
//   uint32_t tickstart = HAL_GetTick();
//   while ((flash_page_first < AT25_PAGES_COUNT) && (HAL_GetTick() - tickstart < timeout))
//   {
//     at25_read_block(flash_page_first * AT25_PAGE_SIZE, buffer, CHUNK_SIZE);
//     uint32_t *signature = (uint32_t*) buffer;
//     if (*signature == data_line_signature)
//     { // found saved data line
//       debug_printf("flash: successfuly read page %d\r\n", flash_page_first);
//       memcpy(dl, buffer + SIGNATURE_SIZE, STRUCT_SIZE);
//       return 1;
//     }
//     else
//     {
//       debug_printf("flash: skipping page %d\r\n", flash_page_first);
//       ++flash_page_first;
//     }
//   }
//   if (flash_page_first >= AT25_PAGES_COUNT)
//   { // data not found, flash is assumed to be empty
//     reset_flash();
//   }
//   return 0;
// }
//
// void reset_flash() {
//   debug_printf("flash: erasing data section\r\n");
//   uint32_t tickstart = HAL_GetTick();
//   at25_erase(FIRST_DATA_PAGE, flash_page_pointer);
//   debug_printf("flash: erased in %lu ms\r\n", (uint32_t)(HAL_GetTick()-tickstart));
//   flash_page_first = FIRST_DATA_PAGE;
//   flash_pages_used = 0;
//   flash_page_pointer = FIRST_DATA_PAGE;
// }

/* Data saving algorithm works as follows:
 * - When new period occurs, it looks if last_period is sent/saved and saves it if it isn't,
 *    after that it saves current_period to last_period and allocates new current_period
 * - When its possible to send data and last_period is not sent/saved it tries to send it and
 *    saves if failed to send, otherwise it tries to send data from storage
 * - Storage consists of energy dependent data_buffer[DATA_BUFFER_LEN] and independent FLASH
 * - When system wants to save line to storage it looks if FLASH is connected and not full,
 *    if so, it saves line to next unused page in flash and also saves data_buffer if it is not empty,
 *    otherwise if flash was not ok, it saves to data_buffer end, shifting it to left if it was full,
 *    so the buffer will store DATA_BUFFER_LEN of last lines in case of flash being full or broken
 * - When system wants to save retrieve a line from storage for sending, it first looks
 *    if data_buffer is not empty, and takes first from there, shifting buffer to the left if send succeeded
 *    otherwise it takes from flash, incrementing flash_pointer if send succeeded
 *    note: flash erase happens ONLY after all lines stored were sent or read failed to find valid line
 */
static void save_last_period() {
  // shift buffer to left if it is full, forgetting the first element
  if (buffer_periods_count >= DATA_BUFFER_LEN) {
    buffer_periods_count = DATA_BUFFER_LEN - 1;
    free(data_buffer[0]);
    for (uint32_t i = 0; i < DATA_BUFFER_LEN; ++i) {
      data_buffer[i] = data_buffer[i + 1];
    }
  }
  data_buffer[buffer_periods_count] = last_period;
  ++buffer_periods_count;
  debug_printf("period: saved to buffer\r\n");
}

void data_period_transition(const volatile uint16_t * counts, DateTime *dt, float t, float p)
{
  if (current_period) // save period data to send it later in the loop
  {
    debug_printf("period: counts b/f = %d / %d\r\n", buffer_periods_count, flash_pages_used);
    for (uint32_t i=0; i<CHANNELS_COUNT; ++i) {
      current_period->counts[i] = counts[i];
    }
    if (last_period)
      save_last_period();
    last_period = current_period;
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

typedef enum {
  S_LAST_PERIOD,
  S_BUFFER,
  S_FLASH
} DataSource;

// returns
//    positive number if line sent but there is more remaining
//    negative number if failed to send
//    0 if sent all
DataStatus data_send_one(uint32_t timeout)
{
  // determine data source
  DataSource source = last_period ? S_LAST_PERIOD : S_BUFFER;
  DataLine *line_to_send;
  switch (source) {
    case S_LAST_PERIOD:
      line_to_send = last_period;
      break;
    case S_BUFFER:
      if (!buffer_periods_count)
        return DATA_CLEAR;
      line_to_send = data_buffer[0];
      break;
  }

  debug_printf("()--> %lu / %.2f hPa / %.2f C ()-->\r\n", line_to_send->timestamp,
    line_to_send->pressure, line_to_send->temperature);
  DataStatus status = send_data_to_server(line_to_send, timeout);

  if (status == DATA_OK) // data is sent, clear it from storage
  {
    switch (source) {
    case S_LAST_PERIOD:
      free(last_period);
      last_period = NULL;
      break;
    case S_BUFFER: // free first and shift buffer to the left
      free(line_to_send);
      --buffer_periods_count;
      for (uint16_t i = 0; i < buffer_periods_count; ++i) {
        data_buffer[i] = data_buffer[i + 1];
      }
      break;
    case S_FLASH: // increment flash pointer and erase if nothing left on chip
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
    if (source == S_LAST_PERIOD)
    { // save last period to storage
      save_last_period();
    }
  }
  return status;
}
