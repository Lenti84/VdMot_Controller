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



#include "TypedQueue.h"

template<typename T>
TypedQueue<T>::TypedQueue() {
  m_count = 0;
  m_head = NULL;
  m_tail = NULL;
}

template<typename T>
TypedQueue<T>::~TypedQueue() {
  for (node* n = m_head; n != NULL; m_head = n) {
    n = m_head->next; 
    delete m_head;
  }

  m_count = 0; 
  m_tail = NULL; 
}


template<typename T>
void TypedQueue<T>::Push(T data) {
  node* t = m_tail;
  m_tail = (node*) new node;
  m_tail->next = NULL;
  m_tail->item = data;

  if (IsEmpty()) {
    m_head = m_tail;
  }
  else {
    t->next = m_tail;
  }

  m_count++;
}

template<typename T>
T TypedQueue<T>::Pop() {
  T data = m_head->item;

  node* t = m_head->next;
  delete m_head;
  m_head = t;

  m_count--;

  return data;
}

template<typename T>
bool TypedQueue<T>::IsEmpty() {
  return m_head == NULL;
}

template<typename T>
int TypedQueue<T>::Count() {
  return m_count;
}

template class TypedQueue<String>;

