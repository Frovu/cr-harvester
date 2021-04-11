/* Provide device configuration options such as target ip, dhcp/static config etc.
*  Allow user to change config via connecting to device as HTTP server
*/
#ifndef __CONFIG_SRV_H
#define __CONFIG_SRV_H

#include "wizchip_conf.h"
#include "socket.h"
#include "counter.h"
#include <string.h>

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
  .target_port = 8787,
  .dev_id = "unknown",
  .target_addr = "crst.izmiran.ru",
  .ntp_addr = "time.google.com",
  .dhcp_mode = NETINFO_DHCP
};

#define SRV_BUF_SIZE       2048
#define CONFIG_SERVER_PORT 80

static const uint8_t html_template[] = "<html><head><title>NM</title></head><body>"
"<h1>Status</h1>"
"Local time: %lu<br>"
"Uptime: %lu minutes<br>"
"Flash failures: %lu<br>"
"Last ntp sync: %lu m ago<br>Flags: %x"
"<h2>Device Configuration</h2>"
"Be caerful, typo in config can lead to device hard fault!<br>"
"If you specify IP, domain field should be empty!<br>Don't forget to set DHCP on/off!"
"<form method=\"post\">"
"<p>Secret <input name=\"secret\" value=\"\"></p>"
"<p>Device ID <input name=\"id\" value=\"$\"></p>"
"<p>Target domain <input name=\"ta\" value=\"$\">"
"<b>OR</b> IP <input name=\"tip\" value=\"$\">, Port <input name=\"port\" value=\"$\"></p>"
"<p>NTP server domain <input name=\"na\" value=\"$\">"
"<b>OR</b> IP <input name=\"nip\" value=\"$\"></p>"
"<p>IP config <input type=\"radio\" name=\"dhcp\" value=\"1\" checked>DHCP <input type=\"radio\" name=\"dhcp\" value=\"0\">Static</p>"
"<p>IP <input name=\"ip\" value=\"$\"></p>"
"<p>Gateway <input name=\"gw\" value=\"$\"></p>"
"<p>Subnet <input name=\"sn\" value=\"$\"></p>"
"<p>DNS <input name=\"dns\" value=\"$\"></p>"
"above 4 have no effect if DHCP on"
"<p><input type=\"submit\" value=\"Update Settings\"></p>"
"</form></body>"
"</html>";

static const uint8_t secret[] = "890gsdfh";

extern Configuration *cfg;

/* These functions are called only if config button is pressed while device boots
*/
uint8_t config_server_init();
uint8_t config_server_run();

/* This function is called every time external flash is initialized
*/
void config_initialize();

/* This function is called only when user changed settings via config server
*/
void config_save();

#endif
