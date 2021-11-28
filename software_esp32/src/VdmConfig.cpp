/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


*END************************************************************************/


#include <stdint.h>
#include "VdmConfig.h"

CVdmConfig VdmConfig;

CVdmConfig::CVdmConfig()
{
}

void CVdmConfig::init()
{
  setDefault();
  readConfig();
}

void CVdmConfig::setDefault()
{
  configFlash.size = sizeof (configFlash);
  configFlash.netConfig.size = sizeof (configFlash.netConfig);
  configFlash.netConfig.netConfigFlags.eth_wifi = 0;	      // 0 = eth (default)
  configFlash.netConfig.netConfigFlags.dhcpEnabled = 1;    // 0 = no DHCP, 1 = use DHCP
  configFlash.netConfig.staticIp = 0;
  configFlash.netConfig.mask = 0;
  configFlash.netConfig.gateway = 0;
  configFlash.netConfig.dnsIp = 0; 
}



void CVdmConfig::readConfig()
{

}

void CVdmConfig::writeConfig()
{

}