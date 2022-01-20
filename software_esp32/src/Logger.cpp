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


#include "Logger.h"

CLogger logger;      // web service logger

CLogger::CLogger(byte bufferSize) 
{
  m_enabled = true;
  m_bufferSize = bufferSize;
}

void CLogger::Clear() 
{
  while (Available()) {
    Pop();
  }
}

void CLogger::SetBufferSize(byte size) 
{
  m_bufferSize = size;
}

void CLogger::Disable() 
{
  m_enabled = false;
  Clear();
}

void CLogger::Enable() 
{
  m_enabled = true;
}

bool CLogger::IsEnabled() 
{
  return m_enabled;
}

void CLogger::println(LogType type) 
{
  println("", type);
}

void CLogger::print(uint32_t data, LogType type) 
{
  print(String(data), type);
}

void CLogger::println(uint32_t data, LogType type) 
{
  println(String(data), type);
}


void CLogger::logData(String data, LogType type) 
{
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    m_queue.Push("DATA:" + data);
  }
}

void CLogger::print(String data, LogType type) 
{
  if (m_enabled && type != LogType::ONLYSYS) {
    Serial.print(data);
  }
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    m_currentLine += data;
  }
}

void CLogger::println(String data, LogType type) 
{
  if (m_enabled && type != LogType::ONLYSYS) {
    Serial.println(data);
  }
  String line = m_currentLine + data;
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    switch (type) {
      case logger.SYS:
        line = "SYS: " + line;
        break;
      case logger.DATA:
        line = "DATA: " + line;
        break;
      case logger.PCA301:
        line = "PCA301: " + line;
        break;
      default:
        break;
    }
    m_queue.Push(line);
  }
  m_currentLine = "";
}

int CLogger::Available()  
{
  return m_queue.Count();
}

String CLogger::Pop() 
{
  return m_queue.Pop();
}