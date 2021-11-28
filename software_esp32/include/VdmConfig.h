
#pragma once

#pragma pack(push)  // store current alignment
#pragma pack(1)     // set alignment to 1 byte boundary
typedef struct {
  uint16_t  eth_wifi:1;     // 0 = eth (default); 1 = wifi
  uint16_t  dhcpEnabled:1;  // 0 = no DHCP, 1 = use DHCP
} VDM_NETWORK_CONFIG_FLAGS;

typedef struct {
  uint8_t  size;
  VDM_NETWORK_CONFIG_FLAGS netConfigFlags;
  uint32_t staticIp;
  uint32_t mask;
  uint32_t gateway;
  uint32_t dnsIp;          //IP address of the domain name server
} VDM_NETWORK_CONFIG;

typedef struct {
  uint8_t  size;
  uint8_t  dataProtocol; // 0 = no protocol , 1 = mqtt
  uint32_t brokerIp;
  uint16_t brokerPort;
} VDM_PROTOCOL_CONFIG;

typedef struct 
{
  uint32_t size;
  VDM_NETWORK_CONFIG netConfig;
  VDM_PROTOCOL_CONFIG protConfig;
} CONFIG_FLASH;
#pragma pack(pop)   // restore alignment

class CVdmConfig
{
public:
  CVdmConfig();
  void init();
  void setDefault();
  void readConfig();
  void writeConfig();

  CONFIG_FLASH configFlash;
};

extern CVdmConfig VdmConfig;