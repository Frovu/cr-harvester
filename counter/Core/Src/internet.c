
#include "internet.h"

extern SPI_HandleTypeDef hspi1;
wiz_NetInfo netinfo = {
  .mac = {0x00, 0x08, 0xdc, 0x7b, 0xa3, 0xdf},
  .dhcp = NETINFO_DHCP
};

uint8_t ntp_buf[sizeof(NtpMessage)];
NtpMessage *ntp_msg = (NtpMessage*) ntp_buf;
uint8_t dhcp_buf[DHCP_BUF_SIZE];
uint8_t http_host[HTTP_HOST_SIZE];
uint8_t http_body[HTTP_BODY_SIZE];
uint8_t http_buf[HTTP_BUF_SIZE];
uint8_t dns_buf[MAX_DNS_BUF_SIZE];
extern uint32_t last_period_tick;
extern DateTime last_period_tm;

/********************* Hardware abstraction for w5500 **********************/
void W5500_Select(void);
void W5500_Unselect(void);
void W5500_ReadBuf (uint8_t *pBuf, uint16_t len);
void W5500_WriteBuf(uint8_t *pBuf, uint16_t len);

void W5500_Select(void) {
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
}

void W5500_Unselect(void) {
  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);
}

void W5500_ReadBuf(uint8_t *pBuf, uint16_t len) {
  HAL_SPI_Receive(&hspi1, pBuf, len, W5500_SPI_TIMEOUT);
}

void W5500_WriteBuf(uint8_t *pBuf, uint16_t len) {
  HAL_SPI_Transmit(&hspi1, pBuf, len, W5500_SPI_TIMEOUT);
}
/***************************************************************************/
void copy_ip(uint8_t *dst, uint8_t *src) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];
}

uint8_t W5500_Connected(void) {
    return getVERSIONR() == W5500_VERSIONR;
}

uint8_t W5500_Init()
{
  // register hardware specific functions
  reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
  reg_wizchip_spiburst_cbfunc(W5500_ReadBuf, W5500_WriteBuf);
  if (!W5500_Connected()) {
    return 0;
  }
  // wizchip_settimeout(..);
  wizchip_init(NULL, NULL); // default 2KB buffers

  netinfo.dhcp = cfg->dhcp_mode;
  copy_ip(netinfo.ip, cfg->local_ip);
  copy_ip(netinfo.gw, cfg->local_gw);
  copy_ip(netinfo.sn, cfg->local_sn);
  copy_ip(netinfo.dns, cfg->local_dns);

  wizchip_setnetinfo(&netinfo);
  // wizchip_getnetinfo(&netinfo);
  if (cfg->dhcp_mode == NETINFO_DHCP)
    DHCP_init(DHCP_SOCKET, dhcp_buf);
  DNS_init(DNS_SOCKET, dns_buf);
  return 1;
}

// DHCP_FAILED = 0,  ///< Processing Fail
// DHCP_RUNNING,     ///< Processing DHCP protocol
// DHCP_IP_ASSIGN,   ///< First Occupy IP from DHPC server
// DHCP_IP_CHANGED,  ///< Change IP address by new ip from DHCP
// DHCP_IP_LEASED,   ///< Stand by
// DHCP_STOPPED      ///< Stop processing DHCP protocol
uint8_t W5500_RunDHCP()
{
  switch (DHCP_run()) {
    case DHCP_IP_ASSIGN:
    case DHCP_IP_CHANGED:
      wizchip_getnetinfo(&netinfo);
      debug_printf("dhcp: new ip assigned: %d.%d.%d.%d\r\n",
        netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);
      return 1;
    case DHCP_IP_LEASED:
      wizchip_getnetinfo(&netinfo);
      debug_printf("dhcp: ip is leased: %d.%d.%d.%d\r\n",
        netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3]);
      debug_printf("dhcp: lease time: %d\r\n", getDHCPLeasetime());
      return 1;
    case DHCP_FAILED:
      debug_printf("dhcp: failed\r\n");
    case DHCP_STOPPED:
    case DHCP_RUNNING:
      break;
  }
  return 0;
}

uint16_t ip_sum(uint8_t * ip) {
  return ip[0] + ip[1] + ip[2] + ip[3];
}

int32_t _stoi(uint8_t *string) {
  int32_t buf = 0;
  uint8_t ch = *string;
  while (ch >= '0' && ch <= '9') {
    buf = buf * 10 + ch - '0';
    ch = *(++string);
  }
  return buf;
}

/*************************************************************************
*************************** BEGIN SECTION HTTP ***************************
**************************************************************************/
uint16_t construct_post_query(DataLine *dl)
{
  uint16_t content_len = 0;
  if (cfg->target_addr[0] == '\0') {
    snprintf(http_host, HTTP_HOST_SIZE, "%u.%u.%u.%u:%u",
      cfg->target_ip[0], cfg->target_ip[0], cfg->target_ip[0], cfg->target_ip[0], cfg->target_port);
  } else {
    snprintf(http_host, HTTP_HOST_SIZE, "%s:%u", cfg->target_addr, cfg->target_port);
  }
  /* string format general info, see README for protocol description */
  #define PFSTRING "k=%s&dt=%lu&upt=%lu&inf=%lu&ff=%lu&t=%.2f&p=%.2f"
  content_len = snprintf(http_body, HTTP_BODY_SIZE, PFSTRING,
    cfg->dev_id, dl->timestamp, dl->cycle, dl->info, flash_error_count,
    dl->temperature, dl->pressure);
  /* string format counters */
  for(uint16_t ch = 0; ch < CHANNELS_COUNT; ++ch) {
    content_len += snprintf(http_body+content_len, HTTP_BODY_SIZE-content_len,
      "&c[%u]=%u", ch, dl->counts[ch]);
  }
  debug_printf("(%u) %s\r\n", content_len, http_body);
  return snprintf(http_buf, HTTP_BUF_SIZE, query_template, cfg->target_path, http_host, content_len, http_body);
}

DataStatus send_data_to_server(DataLine *dl, uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();
  int32_t status; uint16_t length;
  while (HAL_GetTick() - tickstart < timeout)
  {
    switch (getSn_SR(DATA_SOCKET)) {
    case SOCK_CLOSED:
      status = socket(DATA_SOCKET, Sn_MR_TCP, HTTP_PORT, 0x00);
      debug_printf("send: socket() = %d\r\n", (int16_t)status);
      if (status != DATA_SOCKET) {
        return DATA_NET_ERROR;
      }
    case SOCK_INIT:
      status = connect(DATA_SOCKET, cfg->target_ip, cfg->target_port);
      debug_printf("send: connect() = %d\r\n", (int16_t)status);
      if (status == SOCKERR_TIMEOUT || status == SOCKERR_SOCKCLOSED) { // SOCKCLOSED hapens if no route to host
        return DATA_NET_TIMEOUT;
      } else if (status != SOCK_OK) {
        return DATA_NET_ERROR;
      }
      break;
    case SOCK_ESTABLISHED:
      length = construct_post_query(dl);
      // debug_printf("\r\n(%u) %s\r\n\r\n", length, http_buf); // printf whole http req
      status = send(DATA_SOCKET, http_buf, length);
      debug_printf("send: send() = %d\r\n", (int16_t)status);
      if (status <= 0) {
        return DATA_NET_ERROR;
      } else { /* wait for server response */
        do {
          length = getSn_RX_RSR(DATA_SOCKET);
          if (HAL_GetTick() - tickstart > timeout) {
            debug_printf("send: resp timeout\r\n");
            return DATA_NET_TIMEOUT;
          }
        } while (length <= 0);
        length = (length < HTTP_BUF_SIZE) ? length : HTTP_BUF_SIZE;
        length = recv(DATA_SOCKET, http_buf, length);
        http_buf[length] = '\0';
        /* parse http status */
        uint8_t found_head = 0;
        for(uint16_t i=0; i < HTTP_BUF_SIZE; ++i) {
          if (strncmp(http_buf+i, "HTTP", 4) == 0) {
            found_head = 1;
            /* skip until space */
          } else if (found_head && http_buf[i]==' ') {
            status = _stoi(http_buf + i + 1);
            debug_printf("send: HTTP %d\r\n", (int16_t)status);
            if (status == 200) {
              return DATA_OK;
            } else {
              return DATA_NET_NOT_OK;
            }
          }
        }
        debug_printf("send: corrupt response\r\n");
        return DATA_NET_ERROR;
      }
    case SOCK_CLOSE_WAIT:
      debug_printf("send: disconnect()\r\n");
      disconnect(DATA_SOCKET);
      break;
    default:
      return DATA_NET_ERROR;
    }
  }
  close(DATA_SOCKET);
  return DATA_NET_ERROR; // timed out not because of target host down
}
/*************************************************************************
*************************** BEGIN SECTION NTP ****************************
**************************************************************************/

uint64_t convert_ntp_timestamp(uint64_t * timestamp) {
  uint32_t seconds = 0, fraction = 0;
  uint8_t * bytes = (uint8_t*)timestamp;
  for (uint16_t i=0; i < 4; ++i) {
    seconds =  (seconds  << 8) | bytes[i];
    fraction = (fraction << 8) | bytes[i + 4];
  }
  return (((uint64_t)seconds-NTP_EPOCH_OFFSET) * 1000) + ((uint64_t)fraction * 1000 / 0xffffffff);
}

uint8_t try_sync_ntp(uint32_t timeout)
{
  if (ip_sum(cfg->ntp_ip) == 0) {
    debug_printf("ntp: no server ip\r\n");
    return 0;
  }
  // protect from int overflow
  if (last_period_tick > 0xffffffffu - BASE_PERIOD_LEN_MS) {
    return 0;
  }
  debug_printf("ntp: starting sync\r\n");
  uint32_t origin_since_period = HAL_GetTick() - last_period_tick;
  uint32_t origin_s = mktime(&last_period_tm) + (origin_since_period / 1000);

  int32_t result = socket(NTP_SOCKET, Sn_MR_UDP, NTP_PORT, 0x00); // open udp socket
  if (result <= SOCK_ERROR) {
    debug_printf("ntp: failed to socket(), SOCK_ERROR = %d\r\n", result);
    return 0;
  }
  uint16_t length; uint16_t destPort; uint8_t destIp[4];
  uint32_t tickstart = HAL_GetTick();
  // prepare ntp packet
  memset(ntp_msg, 0, NTP_BUF_SIZE);
  //memcpy(ntp_msg.destination, &NTP_SERVER_IP, 4);
  ntp_msg->flags = NTP_REQ_FLAGS;
  ntp_msg->originTimestamp = ((uint64_t)origin_s << 32);

  result = sendto(NTP_SOCKET, (uint8_t*)ntp_msg, NTP_BUF_SIZE, cfg->ntp_ip, NTP_PORT);
  if (result <= SOCK_ERROR) {
    debug_printf("ntp: failed to sendto(), SOCK_ERROR = %d\r\n", result);
    return 0;
  }

  while (HAL_GetTick() - tickstart < timeout) {
    length = getSn_RX_RSR(NTP_SOCKET);
    if (length > 0)
    {
      uint32_t arrival_since_period = HAL_GetTick() - last_period_tick;
      if (length > NTP_BUF_SIZE) {
        length = NTP_BUF_SIZE;
      }
      result = recvfrom(NTP_SOCKET, (uint8_t*)ntp_msg, length, destIp, &destPort);
      close(NTP_SOCKET);
      if (result <= SOCK_ERROR) {
        debug_printf("ntp: failed to recvfrom(), SOCK_ERROR = %d\r\n", result);
        return 0;
      } else {
        uint64_t origin_ms  = (uint64_t)origin_s * 1000 + (origin_since_period % 1000);
        uint64_t arrival_ms = (uint64_t)origin_s * 1000 + (arrival_since_period % 1000);
        uint64_t recieve_ms  = convert_ntp_timestamp(&ntp_msg->receiveTimestamp);
        uint64_t transmit_ms = convert_ntp_timestamp(&ntp_msg->transmitTimestamp);
        int64_t roundtrip = (((int64_t)arrival_ms - origin_ms) - ((int64_t)recieve_ms - transmit_ms));
        int64_t shift = (int64_t)arrival_ms - transmit_ms;
        debug_printf("ntp: got response\r\n");
        debug_printf("ntp: local clocks are %d sec %d ms behind\r\n", (int16_t)(shift/1000), (int16_t)(shift%1000));
        debug_printf("ntp: whole trip time is %d ms\r\n", (int16_t)roundtrip);

        if (shift > roundtrip || shift < -1 * roundtrip)
        {
          const time_t tstmp = transmit_ms / 1000 + 1; // +1 for next sec
          DateTime * new_date = gmtime(&tstmp);
          debug_printf("ntp: writing RTC, wait ~%d ms\r\n", (int16_t)(1000 - (transmit_ms % 1000)));

          // wait until next second beginning to be precise
          int32_t ms_until_next_sec = (1000 - (transmit_ms % 1000)) - (HAL_GetTick() - last_period_tick - arrival_since_period);
          if (ms_until_next_sec < 0) {
            ms_until_next_sec = 1000 + ms_until_next_sec;
            new_date->tm_sec += 1;
            if(new_date->tm_sec == 60) new_date->tm_sec = 0;
          }
          HAL_Delay(ms_until_next_sec);

          if (RTC_WriteDateTime(new_date, 100) != HAL_OK) {
            debug_printf("ntp: failed to write rtc\r\n");
            return 0;
          } else {
            debug_printf("ntp: sync done!\r\n");
          }
          // mitigation for time_left being incorrect in counter.c at first cycle
          last_period_tick -= transmit_ms % 60000;
        }
        else
        {
          debug_printf("ntp: sync not required, |%d| < %d\r\n", (int16_t)shift, (int16_t)roundtrip);
        }
        return 1;
      }
    }
  }
  return 0; // timed out
}

/*************************************************************************
*************************** BEGIN SECTION DNS ****************************
**************************************************************************/
uint8_t run_dns_queries()
{
  uint8_t dns_ip_buf[4];
  uint8_t target_ip_saved[4];
  uint8_t *dns_ip = dns_ip_buf;
  if (cfg->dhcp_mode == NETINFO_DHCP) {
    getDNSfromDHCP(dns_ip);
  } else {
    if(ip_sum(cfg->local_dns) == 0) {
      return 0; // local dns not declared as is dhcp one
    }
    dns_ip = cfg->local_dns;
  }
  copy_ip(target_ip_saved, cfg->target_ip);
  if ((cfg->target_addr[0] != 0) && DNS_run(dns_ip, cfg->target_addr, cfg->target_ip) <= 0) {
    debug_printf("dns: failed to resolve %s\r\n", cfg->target_addr);
    return 0;
  }
  if ((cfg->ntp_addr[0] != 0) && DNS_run(dns_ip, cfg->ntp_addr, cfg->ntp_ip) <= 0) {
    debug_printf("dns: failed to resolve %s\r\n", cfg->ntp_addr);
    return 0;
  }
  debug_printf("dns: ok\r\n\t%s = %u.%u.%u.%u\r\n\t%s = %u.%u.%u.%u\r\n",
    cfg->target_addr, cfg->target_ip[0], cfg->target_ip[1], cfg->target_ip[2], cfg->target_ip[3],
    cfg->ntp_addr, cfg->ntp_ip[0], cfg->ntp_ip[1], cfg->ntp_ip[2], cfg->ntp_ip[3]);
  if (ip_sum(target_ip_saved) != ip_sum(cfg->target_ip)) {
      config_save(); // ip was actually modified by dns
   }

  return 1;
}
