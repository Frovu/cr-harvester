/* Provide device configuration options such as target ip, dhcp/static config etc.
*  Allow user to change config via connecting to device as HTTP server
*/
#ifndef __CONFIG_SRV_H
#define __CONFIG_SRV_H

#include "wizchip_conf.h"
#include "socket.h"

#define DEV_ID_MAX_LEN  16
#define ADDR_MAX_LEN    32

typedef struct {
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



#endif
