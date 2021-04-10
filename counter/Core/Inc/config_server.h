/* Provide device configuration options such as target ip, dhcp/static config etc.
*  Allow user to change config via connecting to device as HTTP server
*/
#ifndef __CONFIG_SRV_H
#define __CONFIG_SRV_H

#include "wizchip_conf.h"
#include "socket.h"
#include "counter.h"

#define DEV_ID_MAX_LEN  16
#define ADDR_MAX_LEN    32

/* Config structure fits in one flash page, but in case of first pages corruption
*  whole first 4K sector is reserved for settings storage
*/
#define CONFIG_FLASH_PAGE_FIRST  0
#define CONFIG_FLASH_PAGES_COUNT 16

typedef struct {
  uint16_t target_port;
  uint8_t dev_id[DEV_ID_MAX_LEN];
  uint8_t target_addr[ADDR_MAX_LEN];
  uint8_t ntp_addr[ADDR_MAX_LEN];
  uint8_t target_ip[4];  // if dns refuses to work we have an ip saved
  uint8_t ntp_ip[4];     // if dns refuses to work we have an ip saved
  uint8_t local_ip[4];
  uint8_t local_gw[4];
  uint8_t local_sn[4];
  uint8_t local_dns[4];
  dhcp_mode dhcp_mode;  // typedef in wizchip_conf.h
} Configuration;

static const Configuration default_cfg = {
  .target_port = "8787",
  .dev_id = "unknown",
  .target_addr = "crst.izmiran.ru",
  .ntp_addr = "time.google.com",
  .dhcp_mode = NETINFO_DHCP
};

/* These functions are called only if config button is pressed while device boots
*/
uint8_t config_server_init();
void     config_server_run();

/* This function is called every time external flash is initialized
*/
void config_initialize();

/* This function is called only when user changed settings via config server
*/
void config_save();

#endif
