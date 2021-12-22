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

// public
extern void communication_setup (void);
extern int16_t communication_loop (void);

#define APP_PRE_SETTARGETPOS       	"stgtp"			// doc
#define APP_PRE_GETONEWIRECNT		"gonec"			// doc
#define APP_PRE_GETONEWIREDATA		"goned"			// doc
#define APP_PRE_SET1STSENSORINDEX	"stsnx"			// doc
#define APP_PRE_SET2NDSENSORINDEX	"stsny"			// doc
#define APP_PRE_SETALLVLVOPEN		"staop"			// doc
#define APP_PRE_GETONEWIRESETT		"gvlon"			// doc
#define APP_PRE_GETVLVDATA			"gvlvd"			// doc

#define APP_PRE_GETVERSION			"gvers"
#define APP_PRE_GETTARGETPOS       	"gtgtp"			

#define APP_PRE_GETACTUALPOS       	"gactp"
#define APP_PRE_GETMEANCURR        	"gmenc"
#define APP_PRE_GETSTATUS          	"gstat"


#endif //_COMMUNICATION_H


