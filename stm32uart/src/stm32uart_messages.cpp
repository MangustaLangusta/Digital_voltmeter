//file stm32uart_messages.cpp

#include "stm32uartConfig.h"
#include "stm32uart.h"
#include "stm32uart_messages.h"

namespace stm32uart{
  

// ==== Definitions ====
  
MessageBox::MessageBox(){
  //default constructor
}  
  
MessageBox::MessageBox(const UartSettings &uart_settings){
  messages_ = {};
  temp_message_ = "";
  max_messages_ = uart_settings.max_messages_stored;
  max_message_length_ = uart_settings.max_message_length;
  eol_symbol_ = uart_settings.eol_symbol;
  overfill_flag_ = false;
}

MessageBox::~MessageBox(){
  //nothing to do in destructor
}

bool MessageBox::Empty(){
  return messages_.empty();
}

int MessageBox::Size(){
  return messages_.size();
}

bool MessageBox::ReadOverfillFlag(){
  return overfill_flag_;
}

void MessageBox::ClearOverfillFlag(){
  overfill_flag_ = false;
}

ReturnState MessageBox::DropRawString(String &new_string){
  
  ClearOverfillFlag();
  
  while(!new_string.empty()){
    std::string::size_type eol_position = new_string.find(eol_symbol_);      
    String new_piece = new_string.substr(0, eol_position);
    
    if( (temp_message_.length() + new_piece.length()) > max_message_length_){
      temp_message_.clear();
    }
    else{
      temp_message_ += new_piece;
    }
    
    if(eol_position != std::string::npos){
        Put(temp_message_);
        temp_message_.clear();
    }
    
    if(eol_position == std::string::npos)
      break;    
    
    new_string.erase(0, eol_position + 1);
  }
  new_string.erase();
  if(ReadOverfillFlag())
    return kMessageBoxOverfill;
  return kOk;
}

ReturnState MessageBox::Put(const String &new_message){
  if(new_message.empty())
    return kOk;
  
  bool overfill = false;
  if (messages_.size() >= max_messages_){
    overfill_flag_ = true;
    overfill = true;
  }
  
  while(messages_.size() >= max_messages_)
    messages_.pop_front();
    
  messages_.emplace_back(new_message);
  
  if(overfill)
    return kMessageBoxOverfill;
  return kOk;  
}

ReturnState MessageBox::GetNext(String *message){
  message->clear();
  
  if(messages_.empty())
    return kNoPendingMessages;
  
  *message = messages_.front();
  messages_.pop_front();
  return kOk;
}

ReturnState MessageBox::GetNextElement(BufferElement *element){
  *element = 0;
  
  if(messages_.empty())
    return kNoPendingMessages;
  
  String tmp_string = messages_.front();
  
  *element = tmp_string[0];
  tmp_string.erase(0,1);
  
  if(tmp_string.empty())
    messages_.pop_front();
  else
    messages_.front() = tmp_string;
 
  return kOk;
}

bool MessageBox::GetNextChunk(const BufferSize chunk_length, String *message){
  message->clear();
  
  if(messages_.empty() || (chunk_length <= 0))
    return false;
  
  int counter = chunk_length;
  
  while(counter > 0){
    
    String tmp_message = messages_.front();
    
    if(tmp_message.length() <= counter){
      *message += tmp_message;
      counter -= tmp_message.length();
      messages_.pop_front();
    }
    
    else {
      *message += tmp_message.substr(0, counter);
      tmp_message.erase(0, counter);
      messages_.front() = tmp_message;
      break;
    }
    
    if(messages_.empty())
      break;
  }
  
  return true;  
}

  
}               //namespace stm32uart