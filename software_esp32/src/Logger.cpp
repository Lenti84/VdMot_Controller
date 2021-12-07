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

Logger::Logger(byte bufferSize) {
  m_enabled = true;
  m_bufferSize = bufferSize;
}

void Logger::Clear() {
  while (Available()) {
    Pop();
  }
}

void Logger::SetBufferSize(byte size) {
  m_bufferSize = size;
}

void Logger::Disable() {
  m_enabled = false;
  Clear();
}

void Logger::Enable() {
  m_enabled = true;
}

bool Logger::IsEnabled() {
  return m_enabled;
}

void Logger::println(LogType type) {
  println("", type);
}

void Logger::print(uint32_t data, LogType type) {
  print(String(data), type);
}
void Logger::println(uint32_t data, LogType type) {
  println(String(data), type);
}


void Logger::logData(String data, LogType type) {
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    m_queue.Push("DATA:" + data);
  }
}

void Logger::print(String data, LogType type) {
  if (m_enabled && type != LogType::ONLYSYS) {
    Serial.print(data);
  }
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    m_currentLine += data;
  }
}

void Logger::println(String data, LogType type) {
  if (m_enabled && type != LogType::ONLYSYS) {
    Serial.println(data);
  }
  String line = m_currentLine + data;
  if (m_enabled &&  m_queue.Count() <= m_bufferSize) {
    switch (type) {
      case Logger::SYS:
        line = "SYS: " + line;
        break;
      case Logger::DATA:
        line = "DATA: " + line;
        break;
      case Logger::PCA301:
        line = "PCA301: " + line;
        break;
      default:
        break;
    }
    m_queue.Push(line);
  }
  m_currentLine = "";
}

int Logger::Available()  {
  return m_queue.Count();
}

String Logger::Pop() {
  return m_queue.Pop();
}