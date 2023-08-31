#ifndef STM32UART_BUFFER_H
#define STM32UART_BUFFER_H

#include "stm32uartConfig.h"
#include "stm32uart.h"

namespace stm32uart{
  
class CircularBuffer{
private:
  BufferSize allocated_size_;
  BufferElement *start_address_;
  BufferSize head_index_;
  BufferSize tail_index_;
  bool valid_;
public:
  CircularBuffer(const BufferSize size);
  ~CircularBuffer();
  
  bool IsValid() const;
  
  BufferElement* StartAddress() const;
  BufferSize AllocatedSize() const;  
  
  long HeadIndex() const;
  long TailIndex() const;
  BufferSize Length() const;
  BufferSize FreeSpace() const;
  
  void SetHeadIndex(const BufferSize new_head_index);
  void IncrementHeadIndex(const BufferSize increment);
  void IncrementTailIndex(const BufferSize increment);
  
  ReturnState PushBack(const BufferElement &element);  
  ReturnState PopFront(BufferElement *element);
  ReturnState Reset();
  
  ReturnState PushChunk(const String &new_chunk);
  
  ReturnState GetData(BufferElement* data);

};  

}               //namespace stm32uart

#endif          //STM32UART_BUFFER_H