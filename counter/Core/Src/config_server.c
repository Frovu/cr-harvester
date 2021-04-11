
#include "config_server.h"

static const uint32_t meme_signature = 0xdeadc0de;
Configuration *cfg = NULL; // global variable for whole program
uint8_t config_modified = 0;


/*************************************************************************
*************************** BEGIN SECTION FLASH ****************************
**************************************************************************/

void config_initialize()
{
  if (cfg && config_modified) // settings were modified while flash was absent
  {
    config_save();
  }
  else // try to load config from flash
  {
    if (cfg == NULL) {
      cfg = (Configuration*) malloc(sizeof(Configuration));
    }
    uint8_t buf[sizeof(meme_signature)+sizeof(Configuration)];
    for (uint16_t page = CONFIG_FLASH_PAGE_FIRST; page < CONFIG_FLASH_PAGES_COUNT; ++page) {
      at25_read_block(page * AT25_PAGE_SIZE, buf, sizeof(meme_signature)+sizeof(Configuration));
      if (*((uint32_t*)buf) == meme_signature)
      { // found settings in flash
        memcpy(cfg, buf + sizeof(meme_signature), sizeof(Configuration));
        debug_printf("init: flash_cfg\tOK\r\n");
        return; // done
      }
    }
    debug_printf("init: flash_cfg\tFAIL\r\n");
    memcpy(cfg, &default_cfg, sizeof(Configuration));
  }
}

void config_save()
{ // erase config sector first
  at25_erase(CONFIG_FLASH_PAGE_FIRST, CONFIG_FLASH_PAGES_COUNT);
  uint8_t buf[sizeof(meme_signature)+sizeof(Configuration)];
  memcpy(buf, &meme_signature, sizeof(meme_signature));
  memcpy(buf + sizeof(meme_signature), cfg, sizeof(Configuration));
  for (uint16_t page = CONFIG_FLASH_PAGE_FIRST; page < CONFIG_FLASH_PAGES_COUNT; ++page) {
    if (at25_write_block(page * AT25_PAGE_SIZE, buf, sizeof(meme_signature)+sizeof(Configuration), DEFAULT_TIMEOUT))
    { // write succeeded
      debug_printf("flash: saved config\r\n");
      return;
    }
  }
}
