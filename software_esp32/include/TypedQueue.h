#ifndef _TYPEDQUEUE_H
#define _TYPEDQUEUE_H

#include <Arduino.h>


template<typename T>
class TypedQueue {
  public:
    TypedQueue();
    ~TypedQueue();

    void Push(T data);
    T Pop();
    bool IsEmpty();
    int Count();

  private:
    typedef struct node {
      T item;      
      node * next; 
    } node;

    int m_count;  
    node *m_head;
    node *m_tail;
};




#endif
