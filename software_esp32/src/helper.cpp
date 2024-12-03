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


#include "helper.h"
#include "VdmSystem.h"

String ip2String (IPAddress ipv4addr)
{
  return ipv4addr.toString();
}

String ConvBinUnits(size_t bytes, byte resolution) 
{
  if  (bytes < 1024) {
    return String(bytes) + " B";
  }
  else if (bytes < 1024 * 1024) {
    return String(bytes / 1024.0, (int)(resolution)) + " KB";
  }
  else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0, (int)(resolution)) + " MB";
  }
  else return "";
}

bool isNumber(const String& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}


bool isFloat(const std::string& str)
{
    if (str.empty())
        return false;

    char* ptr;
    strtof(str.c_str(), &ptr);
    return (*ptr) == '\0';
}


void replace (char* buffer,uint16_t size,char find, char with)
{
  for(int i = 0; i<size;i++)
	{
		if(buffer[i] == find)
		{
			buffer[i] = with;
		}
	}
}

uint32_t versionExplode (String sv)
{
  uint8_t arr[3]={0};
  int8_t pos;
  String s;
  uint32_t result=0;
  
  for (uint8_t i=0;i<3;i++) {
    pos=sv.indexOf('.');
    if (pos>=0) memcpy(&s,sv.c_str(),pos); else s=sv;
    arr[2-i]=s.toInt();
    sv.remove(0,pos+1);
   // UART_DBG.println("versionExplode pos "+String(pos)+":"+s+","+sv);
  }
 
  for (uint8_t i=0;i<3;i++) {
    result|=(uint32_t)arr[i]<<(8*i);
  }
  // UART_DBG.println("versionExplode "+sv+": result="+String(result,16));
  return result;
}

 char* findCharInString (char c,char* pString,size_t size)
 {
  char* p = pString;
  size_t len = 0;
  while ((*p!=c) && (*p!=0) && (len<size)) {
    p++;
    len++;
  }
  if ((*p==0) || (len==size)) p=NULL;
  return (p);
 }

 char* copyStringUntilChar (char* pString,char* buffer,char c,size_t pSize,size_t bSize)
 {
  char* p = pString;
  char* b = buffer;
  size_t len = 0;
  while ((*p!=c) && (*p!=0) && (len<pSize) && (len<bSize)) {
    *b++=*p++;
    len++;
  }
  if (len>=bSize) return NULL;
  return p;
 }

size_t strlcat(char *dst, const char *src, size_t dsize)
{
	const char *odst = dst;
	const char *osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end. */
	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	return(dlen + (src - osrc));	/* count does not include NUL */
}
