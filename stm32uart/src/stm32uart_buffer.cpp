//file stm32uart_buffer.cpp

#include "stm32uart_buffer.h"

namespace stm32uart{
  
CircularBuffer::CircularBuffer(const BufferSize size){
  allocated_size_ = size;
  valid_ = true;
  start_address_ = new BufferElement[allocated_size_];
  if(start_address_ == nullptr){
    valid_ = false;
    allocated_size_ = 0;
  }
  head_index_ = 0;
  tail_index_ = 0;
}

CircularBuffer::~CircularBuffer(){
  if(valid_ == false)
    return;
  delete start_address_;
}

bool CircularBuffer::IsValid() const{
  return valid_;
}

BufferElement* CircularBuffer::StartAddress() const{
  return start_address_;  
}

BufferSize CircularBuffer::AllocatedSize() const{
  return allocated_size_;
}

long CircularBuffer::HeadIndex() const{
  return head_index_;
}

long CircularBuffer::TailIndex() const{
  return tail_index_;
}

void CircularBuffer::SetHeadIndex(const BufferSize new_head_index){
  BufferSize tmp_head_index = new_head_index;
  while(tmp_head_index < 0)
    tmp_head_index += allocated_size_;
  while(tmp_head_index > (allocated_size_ - 1))
    tmp_head_index -= allocated_size_;
  head_index_ = tmp_head_index;
}

BufferSize CircularBuffer::Length() const{
  BufferSize diff = head_index_ - tail_index_;
  if(diff >= 0)
    return diff;
  return allocated_size_ + diff;
}

BufferSize CircularBuffer::FreeSpace() const{
  return allocated_size_ - Length();  
}

void CircularBuffer::IncrementHeadIndex(const BufferSize increment){
  BufferSize tmp_head_index = head_index_;
  tmp_head_index += increment;
  
  if(tmp_head_index > (allocated_size_))
    tmp_head_index = 0;
        
  head_index_ = tmp_head_index;
}
  
void CircularBuffer::IncrementTailIndex(const BufferSize increment){
  BufferSize tmp_tail_index = tail_index_;
  tmp_tail_index += increment;
  
  while(tmp_tail_index > (allocated_size_ - 1))
    tmp_tail_index -= allocated_size_;
        
  tail_index_ = tmp_tail_index;
}
        
ReturnState CircularBuffer::PushBack(const BufferElement &element){
  start_address_[head_index_] = element;
  
  if(head_index_ == (allocated_size_ - 1))
    head_index_ = 0;
  else
    head_index_++;
  
  if(head_index_ == tail_index_)
    return kBufferFull;
  
  return kOk;
}
        
ReturnState CircularBuffer::PopFront(BufferElement *element){       
  *element = 0;
  
  if(tail_index_ == head_index_)
    return kBufferEmpty;
  
  *element = start_address_[tail_index_];
  IncrementTailIndex(1);
  
  return kOk;
}    

ReturnState CircularBuffer::Reset(){
  head_index_ = 0;
  tail_index_ = 0;
  return kOk;
}

ReturnState CircularBuffer::GetData(BufferElement* data){
  for(int i = 0; i < Length(); i++){
    data[i] = start_address_[tail_index_];
    IncrementTailIndex(1);
  }
  return kOk;
}

ReturnState CircularBuffer::PushChunk(const String &new_chunk){
  
  if(new_chunk.length() > allocated_size_)
    return kError;
  
  for(BufferSize i = 0; i < new_chunk.length(); i++){
    start_address_[i] = new_chunk[i];
  }
  
  IncrementHeadIndex(new_chunk.length());
  return kOk;
}



}//namespace stm32uart
  /*
12 13 14 15 16 17
 h     t
*/