#ifndef __INTERNET_H
#define __INTERNET_H

#define _WIZCHIP_ 5500
#include "counter.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"

#define DHCP_SN             2       // DHCP socket number (0 ~ 7)
#define DHCP_BUF_SIZE       548
#define W5500_SPI_TIMEOUT   300
#define W5500_VERSIONR      0x04

uint8_t W5500_Connected(void);
uint8_t W5500_Init();
uint8_t W5500_RunDHCP();

#endif
