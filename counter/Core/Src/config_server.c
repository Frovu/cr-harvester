
//Configuration config = default_config;

#include "config_server.h"

Configuration cfg = {
  .dev_id = "unknown",
  .target_addr = "crst.izmiran.ru",
  .ntp_addr = "time.google.com",
  .dhcp_mode = NETINFO_DHCP
};
