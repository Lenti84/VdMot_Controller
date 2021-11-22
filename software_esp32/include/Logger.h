#ifndef _LOGGER_h
#define _LOGGER_h

#include "Arduino.h"
#include "TypedQueue.h"

class Logger {
 private:
   TypedQueue<String> m_queue;
   bool m_enabled;
   String m_currentLine;
   byte m_bufferSize;
 public:
   enum LogType {
     SYS = 0,
     DATA = 1,
     PCA301 = 2,
     ONLYSYS = 3
   };
   Logger(byte bufferSize=20);
  void print(String data, LogType type = LogType::SYS);
  void print(uint32_t data, LogType type = LogType::SYS);
  void println(String data, LogType type = LogType::SYS);
  void println(LogType type = LogType::SYS);
  void println(uint32_t data, LogType type = LogType::SYS);
  void logData(String data, LogType type = LogType::SYS);
  int Available();
  String Pop();
  void Disable();
  void Enable();
  bool IsEnabled();
  void Clear();
  void SetBufferSize(byte size);
   
};

extern Logger logger;

#endif

