#ifndef STM32UART_MESSAGES_H
#define STM32UART_MESSAGES_H

namespace stm32uart{
  
class MessageBox{
private:
  std::list<String> messages_;
  String temp_message_;
  int max_messages_;    
  int max_message_length_;
  String eol_symbol_;
  bool overfill_flag_;
  
  
public:
  MessageBox();
  MessageBox(const UartSettings &uart_settings);
  ~MessageBox();

  bool Empty();
  int Size();
  
  bool ReadOverfillFlag();
  void ClearOverfillFlag();
  
  ReturnState DropRawString(String &new_string);
  
  ReturnState Put(const String &new_message);
  ReturnState GetNext(String *message);
  ReturnState GetNextElement(BufferElement *element);
  bool GetNextChunk(const BufferSize chunk_length, String *message);
};

}               //namespace stm32uart

#endif          //STM32UART_MESSAGES_H