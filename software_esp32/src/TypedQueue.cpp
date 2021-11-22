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

