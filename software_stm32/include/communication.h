/**HEADER*******************************************************************
  project : VdMot Controller
  author : Lenti84
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

#ifndef _COMMUNICATION_H
	#define _COMMUNICATION_H

#include <Arduino.h>

#define COMM_SER				Serial1		// serial port to ESP32
//#define COMM_DBG				Serial3		// serial port for debugging
#define COMM_DBG				Serial6		// serial port for debugging

// public
extern void communication_setup (void);
extern int16_t communication_loop (void);

#define APP_PRE_SETTARGETPOS        "stgtp"			// doc
#define APP_PRE_GETONEWIRECNT       "gonec"			// doc
#define APP_PRE_GETONEWIREDATA      "goned"			// doc
#define APP_PRE_SET1STSENSORINDEX   "stsnx"			// doc
#define APP_PRE_SET2NDSENSORINDEX   "stsny"			// doc
#define APP_PRE_SETALLVLVOPEN       "staop"			// doc
#define APP_PRE_GETONEWIRESETT      "gvlon"			// doc
#define APP_PRE_GETVLVDATA          "gvlvd"			// doc
#define APP_PRE_GETVLSTATUS         "gvlst"			// new

#define APP_PRE_SETONEWIRESEARCH    "stons"		  // doc
#define APP_PRE_GETVERSION			    "gvers"     // doc
#define APP_PRE_GETHWINFO 			    "ghwin"     // doc
#define APP_PRE_GETTARGETPOS       	"gtgtp"	    // doc 
#define APP_PRE_SETLEARNTIME        "stlnt"		  // doc
#define APP_PRE_SETLEARNMOVEM       "stlnm"     // doc
#define APP_PRE_GETLEARNMOVEM       "gtlnm"     // new
#define APP_PRE_SETVLLEARN          "staln"		  // doc		
#define APP_PRE_SETMOTCHARS         "smotc"		  // doc	
#define APP_PRE_GETMOTCHARS         "gmotc"		  // doc

#define APP_PRE_SETVLVSENSOR        "stvls"     
#define APP_PRE_SETDETECTVLV        "stdet"     

#define APP_PRE_MATCHSENS           "masns"     
#define APP_PRE_SOFTRESET           "reset"     
#define APP_PRE_EEPSTATE            "eepst"     // new eeprom state : 1 = ready, 0 = write pending

#define APP_PRE_GETACTUALPOS       	"gactp"     // not implemented
#define APP_PRE_GETMEANCURR        	"gmenc"     // not implemented

#define NO_OF_ARGS                   5
#define ARG_SIZE                     30

#endif //_COMMUNICATION_H


