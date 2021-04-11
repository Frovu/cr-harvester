#ifndef __INTERNET_H
#define __INTERNET_H

#include "counter.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"
#include "dns.h"
#include "config_server.h"
#include <time.h>
#include <string.h>

// socket numbers (0 ~ 7)
#define NTP_SOCKET          1
#define DHCP_SOCKET         2
#define DNS_SOCKET          3
#define SERVER_SOCKET       4
#define DATA_SOCKET         5

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

#define NTP_PORT            123
#define NTP_BUF_SIZE        sizeof(NtpMessage)
#define NTP_VERSION         0x4     // v4
#define NTP_MODE            0x3     // client
#define NTP_REQ_FLAGS    (((NTP_VERSION & 0x7) << 3) | (NTP_MODE & 0x7))
#define NTP_EPOCH_OFFSET    2208988800

// FIXME: query assembly uses more memory than it actually needs, but we have enough RAM available
#define HTTP_PORT           8888
#define HTTP_PATH_SIZE      64
#define HTTP_HOST_SIZE      32
#define HTTP_BODY_SIZE      256
#define HTTP_BUF_SIZE       (128+HTTP_PATH_SIZE+HTTP_HOST_SIZE+HTTP_BODY_SIZE)

static const uint8_t query_path[] = "/";
static const uint8_t query_template[] = "POST %s HTTP/1.1\r\n"
"Host: %s\r\n"
"Content-Type: application/json\r\n"
"Content-Length: %u\r\n"
"\r\n%s";

uint8_t W5500_Connected(void);
uint8_t W5500_Init();
uint8_t W5500_RunDHCP();

int32_t _stoi(uint8_t *string);
HAL_StatusTypeDef send_data_to_server(DataLine *dl, uint32_t timeout);

uint8_t run_dns_queries();
uint8_t try_sync_ntp(uint32_t timeout);
#endif
