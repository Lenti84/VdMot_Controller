#ifndef _WEB_H_
#define _WEB_H_

#include <Arduino.h>
#include "Logger.h"
//#include <FS.h>
//#include <WebServer.h>
//#include <WebServer_WT32_ETH01.h>

//extern WebServer server;
//extern File fsUploadFile;

extern Logger logger;

void webserver_loop();
void webserver_setup();
void handleFileUpload();
void handleFlash();
void handleFileDelete();
void handleListFiles();
String makePage(String title, String contents);

#endif