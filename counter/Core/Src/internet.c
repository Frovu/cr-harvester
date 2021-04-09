
#include "internet.h"

extern SPI_HandleTypeDef hspi1;
extern uint16_t flags;
wiz_NetInfo netinfo = {
  .mac = {0x00, 0x08, 0xdc, 0x7b, 0xa3, 0xdf},
  .dhcp = NETINFO_DHCP
};

uint8_t ntp_buf[sizeof(NtpMessage)];
NtpMessage *ntp_msg = (NtpMessage*) ntp_buf;
uint8_t dhcp_buf[DHCP_BUF_SIZE];
extern uint32_t last_period_tick;
extern DateTime last_period_tm;

uint8_t try_sync_ntp(uint32_t timeout);
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
uint8_t W5500_Connected(void) {
    return getVERSIONR() == W5500_VERSIONR;
}

uint8_t W5500_Init()
{
  reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
  reg_wizchip_spiburst_cbfunc(W5500_ReadBuf, W5500_WriteBuf);
  if (!W5500_Connected()) {
    return 0;
  }
  // register hardware specific functions
  // wizchip_settimeout(..);
  wizchip_init(NULL, NULL); // default 2KB buffers
  wizchip_setnetinfo(&netinfo);
  // wizchip_getnetinfo(&netinfo);
  DHCP_init(DHCP_SOCKET, dhcp_buf);
  RAISE(FLAG_DHCP_RUN);
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

  result = sendto(NTP_SOCKET, (uint8_t*)ntp_msg, NTP_BUF_SIZE, (uint8_t*)NTP_SERVER_IP, NTP_PORT);
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
