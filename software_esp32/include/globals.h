/**HEADER*******************************************************************
  project : VdMot Controller

  author : SurfGargano, Lenti84

  Comments:

  Version :

  Modifcations :


***************************************************************************
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE DEVELOPER OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Copyright (C) 2021 Lenti84  https://github.com/Lenti84/VdMot_Controller

*END************************************************************************/


#pragma once

#include <Arduino.h>
#include "compile_time.h"

// #define CONFIG_ASYNC_TCP_USE_WDT 0

#define ACTUATOR_COUNT      12
#define TEMP_SENSORS_COUNT  34

// debug mode
#define VISMODE_DEFAULT     VISMODE_ATOMIC               
#define VISMODE_OFF         0               // no messages at all
#define VISMODE_ON          1               // minimum debug messages
#define VISMODE_DETAIL      2               // detailed debug messages
#define VISMODE_ATOMIC      3               // atomic debug messages


// This device info
#define DEVICE_HOSTNAME     "VdMot"
#define APP_NAME            "SysLog"
#define ANOTHER_APP_NAME    "my-another-app"

// hardware
#define NRST                15              // IO15
#define BOOT0               14              // IO14
//#define LED               2               // not available on WT32-ETH01
#define UART_STM32          Serial2         // serial interface connected to STM32
#define UART_DBG			      Serial		    // serial port for debugging

// on WT32-ETH01 V1.2 use for Serial2 pins: RX2 IO5 and TX2 IO17
#define STM32_RX            5
#define STM32_TX            17

// version info
#ifndef FIRMWARE_BUILD
  #define FIRMWARE_BUILD __TIME_UNIX__
#endif



