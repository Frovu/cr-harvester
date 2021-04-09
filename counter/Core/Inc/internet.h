#ifndef __INTERNET_H
#define __INTERNET_H

#define _WIZCHIP_ 5500
#include "counter.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"
#include <time.h>
#include <string.h>

#define DHCP_SOCKET         2       // DHCP socket number (0 ~ 7)
#define DHCP_BUF_SIZE       548
#define W5500_SPI_TIMEOUT   300
#define W5500_VERSIONR      0x04

typedef struct {
  uint8_t  flags;
  uint8_t  stratum;
  uint8_t  pollingInterval;
  uint8_t  precision;
  uint32_t  rootDelay;
  uint32_t  rootDispersion;
  uint32_t  refID;
  uint64_t  referenceTimestamp;
  uint64_t  originTimestamp;
  uint64_t  receiveTimestamp;
  uint64_t  transmitTimestamp;
} NtpMessage;

static const uint8_t NTP_SERVER_IP[4] = {216, 239, 35, 4};
#define NTP_PORT            123
#define NTP_SOCKET          1
#define NTP_BUF_SIZE        sizeof(NtpMessage)
#define NTP_VERSION         0x4     // v4
#define NTP_MODE            0x3     // client
#define NTP_REQ_FLAGS    (((NTP_VERSION & 0x7) << 3) | (NTP_MODE & 0x7))
#define NTP_EPOCH_OFFSET    2208988800

uint8_t W5500_Connected(void);
uint8_t W5500_Init();
uint8_t W5500_RunDHCP();

uint8_t query_ntp(uint32_t timeout, uint32_t originTimestamp);
uint8_t try_sync_ntp();
#endif
