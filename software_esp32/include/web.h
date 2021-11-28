#ifndef _WEB_H_
#define _WEB_H_

#include <Arduino.h>
#include "Logger.h"
#include "ETH.h"
#include "VdmConfig.h"

//#include <FS.h>

//extern File fsUploadFile;

extern Logger logger;


void handleFileUpload();
void handleFlash();
void handleFileDelete();
void handleListFiles();
String makePage(String title, String contents);

String ip2String (IPAddress ipv4addr);
void postValvePos();
String getValveStatus();
String getNetInfo(ETHClass ETH,VDM_NETWORK_CONFIG netConfig); 
String getNetConfig (VDM_NETWORK_CONFIG netConfig);
String getProtConfig (VDM_PROTOCOL_CONFIG protConfig);
String getSysInfo();

#endif