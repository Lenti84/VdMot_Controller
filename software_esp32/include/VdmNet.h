/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano

  Comments:


*END************************************************************************/

#pragma once

#include <stdint.h>
#include "VdmConfig.h"

#define noneProtocol 0
#define mqttProtocol 1

class CVdmNet
{
public:
  CVdmNet();
  void init();
  void setup();
  void setupEth();
  void setupWifi();
  void initServer();
  void webServerLoop();
  bool serverIsStarted;
  bool dataBrokerIsStarted;
};

extern CVdmNet VdmNet;


