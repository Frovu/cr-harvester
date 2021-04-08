
#include "internet.h"

extern SPI_HandleTypeDef hspi1;
extern uint16_t flags;
wiz_NetInfo netinfo = {
  .mac = {0x00, 0x08, 0xdc, 0x7b, 0xa3, 0xdf},
  .dhcp = NETINFO_DHCP
};

uint8_t dhcp_buf[DHCP_BUF_SIZE];

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
  DHCP_init(DHCP_SN, dhcp_buf);
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
