
#include "config_server.h"

uint8_t srv_buf[SRV_BUF_SIZE];
static const uint32_t meme_signature = 0xdeadc0de;
Configuration *cfg = NULL; // global variable for whole program
uint8_t config_modified = 0;

typedef enum {
  T_STRING,
  T_IP,
  T_INT
} t_type_t;

uint16_t atoi(uint8_t *string)
{

}

void token_ctl(uint8_t mode_write, uint8_t *token, uint8_t *dest, uint16_t destlen) {
  t_type_t type;
  void *res;
  uint8_t *s_res;
  if (strcmp(token, "id") == 0) {
    type = T_STRING;
    res = cfg->dev_id;
  } else if (strcmp(token, "ta") == 0) {
    type = T_STRING;
    res = cfg->target_addr;
  } else if (strcmp(token, "port") == 0) {
    type = T_INT;
    res = &cfg->target_port;
  } else if (strcmp(token, "tip") == 0) {
    type = T_IP;
    res = cfg->target_ip;
  } else if (strcmp(token, "na") == 0) {
    type = T_STRING;
    res = cfg->ntp_addr;
  } else if (strcmp(token, "nip") == 0) {
    type = T_IP;
    res = cfg->ntp_ip;
  } else if (strcmp(token, "ip") == 0) {
    type = T_IP;
    res = cfg->local_ip;
  } else if (strcmp(token, "gw") == 0) {
    type = T_IP;
    res = cfg->local_gw;
  } else if (strcmp(token, "sn") == 0) {
    type = T_IP;
    res = cfg->local_sn;
  } else if (strcmp(token, "dns") == 0) {
    type = T_IP;
    res = cfg->local_dns;
  } else if (strcmp(token, "dhcp") == 0) {
    // only in parsing mode
    if (dest[0] == '0') {
      cfg->dhcp_mode = NETINFO_STATIC;
    } else {
      cfg->dhcp_mode = NETINFO_DHCP;
    }
    return;
  }
  if (mode_write)
  {
    switch (type) {
      case T_STRING:
    }

  }
  else // parse string
  {
    s_res = (uint8_t*) res;
    switch (type) {
      case T_STRING:
        memcpy(s_res, dest, destlen);
        s_res[destlen] = '\0';
      case T_IP:
        memcpy(s_res, dest, destlen);
        s_res[destlen] = '\0';
    }
  }
}

uint16_t prepare_html_resp()
{
  if (current_period) { // TODO: flash failures
    return snprintf(srv_buf, SRV_BUF_SIZE, html_template, current_period->timestamp, cycle_counter, 0,
      (cycle_counter-last_ntp_sync), flags);
  } else {
    memcpy(srv_buf, html_template, strlen(html_template));
    return strlen(html_template);
  }
}

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
        len = prepare_html_resp();
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
