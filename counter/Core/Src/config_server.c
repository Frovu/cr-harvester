
#include "config_server.h"

uint8_t srv_buf[SRV_BUF_SIZE];
static const uint32_t meme_signature = 0xdeadc0de;
Configuration config_buf;
Configuration *cfg = &config_buf; // global variable for whole program
uint8_t config_modified = 0;

typedef enum {
  T_STRING,
  T_IP,
  T_INT
} t_type_t;

void parse_ip(uint8_t *string, uint8_t *ip)
{
  uint16_t buf;
  uint8_t ch = *string;
  for (uint8_t i=0; i<4; ++i) {
    buf = 0;
    while (ch >= '0' && ch <= '9') {
      buf = buf * 10 + ch - '0';
      ch = *(++string);
    }
    ip[i] = buf;
    ch = *(++string); // skip dot
  }
}

void token_ctl(uint8_t mode_write, uint8_t *token, uint8_t *dest, uint16_t destlen) {
  t_type_t type = T_STRING;
  void *res = NULL;
  uint8_t *s_res; uint16_t *i_res;
  if (strcmp(token, "id") == 0) {
    type = T_STRING;
    res = cfg->dev_id;
  } else if (strcmp(token, "ta") == 0) {
    type = T_STRING;
    res = cfg->target_addr;
  } else if (strcmp(token, "port") == 0) {
    type = T_INT;
    res = &cfg->target_port;
  } else if (strcmp(token, "path") == 0) {
    type = T_STRING;
    res = &cfg->target_path;
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
  } else {
    return; // unknown token
  }
  s_res = (uint8_t*) res;
  if (mode_write)
  {
    switch (type) {
      case T_STRING:
        snprintf(dest, destlen, "%s", s_res);
        break;
      case T_IP:
        snprintf(dest, destlen, "%u.%u.%u.%u", s_res[0], s_res[1], s_res[2], s_res[3]);
        break;
      case T_INT:
        snprintf(dest, destlen, "%u", *((uint16_t*) res));
        break;
    }
  }
  else // parse string
  {
    switch (type) {
      case T_STRING:
        memcpy(s_res, dest, destlen);
        s_res[destlen] = '\0';
        break;
      case T_IP:
        parse_ip(dest, s_res);
        break;
      case T_INT:
	    i_res = (uint16_t*) res;
        *i_res = (uint16_t) _stoi(dest);
        break;
    }
  }
}

// parse HTTP response and update settings accordingly
uint8_t update_settings()
{
  uint8_t first_token = 1;
  uint8_t token[16];
  uint16_t i = 0, i_t = 0, i_v = 0;
  while(strncmp(srv_buf+(i++), "\r\n\r\n", 4) != 0 && i < SRV_BUF_SIZE); // skip headers
  for (i+=3; i < SRV_BUF_SIZE; ++i) {
    if (srv_buf[i] == '=')
    { // parse token=value pair
      token[i_t] = '\0';
      for (i_v = ++i; i_v < SRV_BUF_SIZE; ++i_v) {
        if ((srv_buf[i_v] == '&') || (srv_buf[i_v] == '\0')) {
          break;
        }
      }
      if (first_token) {
        if ((strncmp(token, "secret", 6) != 0) || (strncmp(srv_buf+i, secret, sizeof(secret)-1) != 0)) {
          debug_printf("srv: secret invalid\r\n");
          return 0; // secret validation failed
        }
        first_token = 0;
      } else {
        token_ctl(0, token, srv_buf+i, i_v-i);
      }
      i = i_v + 1; // skip value
      i_t = 0;
    } else if (srv_buf[i] == '\0') {
      break;
    }
    token[i_t++] = srv_buf[i];
  }
  return 1; // success
}

uint16_t prepare_html_resp()
{ // TODO: show amount of data stored
  uint8_t token[16];
  uint8_t in_token = 0;
  uint16_t i = 0, templ_i = 0, tok_i = 0;
  snprintf(srv_buf, SRV_BUF_SIZE, html_template, current_period?current_period->timestamp:0, cycle_counter,
    flash_error_count, (cycle_counter-last_ntp_sync), flags);
  // syncronize template and buffer pointers
  while(strncmp(srv_buf+(i++), "<h2>Dev", 7) != 0);
  while(strncmp(html_template+(templ_i++), "<h2>Dev", 7) != 0);
  memset(srv_buf+i, 0, SRV_BUF_SIZE-i);
  for (; i < SRV_BUF_SIZE && templ_i < sizeof(html_template); ++i) {
    if (html_template[templ_i] == '$') {
      templ_i++;
      in_token = 0;
      token_ctl(1, token, srv_buf+i, SRV_BUF_SIZE-i-1);
      while(srv_buf[++i] != '\0'); // skip bytes written by tokenctl
    } else if (html_template[templ_i] == '"') {
      if (in_token || tok_i >= 16) {
        in_token = 0;
        token[tok_i] = '\0';
      } else {
        in_token = 1;
        tok_i = 0;
      }
    } else if(in_token) {
      token[tok_i++] = html_template[templ_i];
    }
    srv_buf[i] = html_template[templ_i];
    templ_i++;
  }
  return i;
}

// @retval: 0 - idle, 1 - client connected
DataStatus config_server_run()
{
  uint16_t len; int16_t status;
  switch (getSn_SR(SERVER_SOCKET)) {
    case SOCK_ESTABLISHED:
      len = getSn_RX_RSR(SERVER_SOCKET);
      if (len > 0)
      {
        len = (len < SRV_BUF_SIZE) ? len : SRV_BUF_SIZE;
        len = recv(SERVER_SOCKET, srv_buf, len);
        srv_buf[len] = '\0';
        debug_printf("srv: got request of len %u\r\n", len);
        if (strncmp(srv_buf, "POST", 4) == 0) {
          if (update_settings()) {
            config_modified = 1;
            config_save();
            memcpy(srv_buf, html_ok, sizeof(html_ok));
            send(SERVER_SOCKET, srv_buf, sizeof(html_ok));
            disconnect(SERVER_SOCKET);
            return DATA_CLEAR;
          } else {
            memcpy(srv_buf, html_error, sizeof(html_error));
            send(SERVER_SOCKET, srv_buf, sizeof(html_error));
            disconnect(SERVER_SOCKET);
          }
        } else if (strncmp(srv_buf, "GET", 3) == 0) {
          len = prepare_html_resp();
          send(SERVER_SOCKET, srv_buf, len);
          disconnect(SERVER_SOCKET);
        }
      }
      break;
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
      if (status != SERVER_SOCKET) {
        return DATA_NET_ERROR;
      }
    default:
      break;
  }
  return DATA_OK;
}

/*************************************************************************
*************************** BEGIN SECTION FLASH ****************************
**************************************************************************/

void config_set_default()
{
  memcpy(cfg, &default_cfg, sizeof(Configuration));
}

void config_initialize()
{
  if (config_modified) // settings were modified while flash was absent
  {
    config_save();
  }
  else // try to load config from flash
  {
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
    config_set_default();
  }
}

void config_save()
{ // erase config sector first
  at25_erase(CONFIG_FLASH_PAGE_FIRST, CONFIG_FLASH_PAGES_COUNT);
  uint8_t buf[sizeof(meme_signature)+sizeof(Configuration)];
  memcpy(buf, &meme_signature, sizeof(meme_signature));
  memcpy(buf + sizeof(meme_signature), cfg, sizeof(Configuration));
  for (uint16_t page = CONFIG_FLASH_PAGE_FIRST; page < CONFIG_FLASH_PAGES_COUNT; ++page) {
    if (at25_write_block(page * AT25_PAGE_SIZE, buf, sizeof(meme_signature)+sizeof(Configuration), DEFAULT_TIMEOUT) == FLASH_ERRNO_OK)
    { // write succeeded
      debug_printf("flash: saved config at page %u\r\n", page);
      config_modified = 0;
      return;
    } else {
      debug_printf("flash: failed to save cfg to page %u\r\n", page);
    }
  }
}
