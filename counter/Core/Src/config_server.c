
#include "config_server.h"

uint8_t srv_buf[SRV_BUF_SIZE];
static const uint32_t meme_signature = 0xdeadc0de;
Configuration *cfg = NULL; // global variable for whole program
uint8_t config_modified = 0;

uint8_t config_server_init()
{

}

// @retval: 0 - idle, 1 - client connected
uint8_t config_server_run()
{
  uint16_t len, status;
  switch (getSn_SR(SERVER_SOCKET)) {
    case SOCK_ESTABLISHED:
      len = getSn_RX_RSR(SERVER_SOCKET);
      if (len > 0)
      {
        len = (len < SRV_BUF_SIZE) ? len : SRV_BUF_SIZE;
        len = recv(SERVER_SOCKET, srv_buf, len);
        debug_printf("srv: got request of len %u\r\n", len);
        len = strlen(html_template);
        memcpy(srv_buf, html_template, len);
        send(SERVER_SOCKET, srv_buf, len);
        disconnect(SERVER_SOCKET);
      }
      return 1;
    case SOCK_CLOSE_WAIT:
      disconnect(SERVER_SOCKET);
      debug_printf("srv: disconnect()\r\n");
      break;
    case SOCK_INIT:
      listen(SERVER_SOCKET);
      break;
    case SOCK_CLOSED:
      status = socket(SERVER_SOCKET, Sn_MR_TCP, CONFIG_SERVER_PORT, 0x00);
      debug_printf("srv: socket(%d) = %d\r\n", SERVER_SOCKET, status);
    default:
      break;
  }
  return 0;
}

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
