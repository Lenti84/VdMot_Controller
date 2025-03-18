#pragma once

#include "globals.h"

#define RTC_RAM_PATTERN "RTCPattern001"

typedef struct {
  char pattern[15];  
  float target;
  float value;
  float iProp;
  float esum;
  float lastControlPosition;
} PI_CONTROL_RTC;

typedef struct {
    PI_CONTROL_RTC piControlRtc[ACTUATOR_COUNT];
} PI_CONTROL_RTCS;


extern RTC_NOINIT_ATTR int readingCnt;
extern RTC_NOINIT_ATTR PI_CONTROL_RTCS piControlRtcs;