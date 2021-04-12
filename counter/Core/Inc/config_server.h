/* Provide device configuration options such as target ip, dhcp/static config etc.
*  Allow user to change config via connecting to device as HTTP server
*/
#ifndef __CONFIG_SRV_H
#define __CONFIG_SRV_H

#include "wizchip_conf.h"
#include "socket.h"
#include "counter.h"
#include <string.h>

/* Config structure fits in one flash page, but in case of first pages corruption
*  whole first 4K sector is reserved for settings storage
*/
#define CONFIG_FLASH_PAGE_FIRST  0
#define CONFIG_FLASH_PAGES_COUNT 16

#define DEV_ID_MAX_LEN  16
#define ADDR_MAX_LEN    32
#define PATH_MAX_LEN    64

typedef struct {
  uint16_t target_port;
  uint8_t dev_id[DEV_ID_MAX_LEN];
  uint8_t target_addr[ADDR_MAX_LEN];
  uint8_t target_path[PATH_MAX_LEN];
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
  .dev_id = "anon",
  .target_addr = "crst.izmiran.ru",
  .target_path = "/api/data",
  .target_port = 8787,
  .ntp_addr = "time.google.com",
  .local_ip = {192, 168, 1, 217},
  .local_gw = {192, 168, 1, 1},
  .local_sn = {255, 255, 255, 0},
  .local_dns = {8, 8, 8, 8},
  .dhcp_mode = NETINFO_STATIC,
};

#define SRV_BUF_SIZE       2048
#define CONFIG_SERVER_PORT 80

#define HTTP_OK_RESP "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"

static const uint8_t html_template[] = HTTP_OK_RESP"<html><head><title>NM</title></head><body>"
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
"<p>Target domain <input name=\"ta\" value=\"$\"><b>OR</b> IP <input name=\"tip\" value=\"$\">"
"<br>Target URL Path <input name=\"path\" value=\"$\">, TCP Port <input name=\"port\" value=\"$\"></p>"
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

static const uint8_t html_ok[] = HTTP_OK_RESP"<html><head><title>NM</title></head><body>"
"<h2 style=\"color:green;\">Device configuration updated!</h2></body></html>";

static const uint8_t html_error[] = HTTP_OK_RESP"<html><head><title>NM</title></head><body>"
"<h2 style=\"color:red;\">Invalid secret code!</h2></body></html>";

static const uint8_t secret[] = "alpine";

extern Configuration *cfg;

uint8_t config_server_run();

void config_set_default();

/* This function is called every time external flash is initialized
*/
void config_initialize();

/* This function is called only when config changed
*/
void config_save();

#endif
