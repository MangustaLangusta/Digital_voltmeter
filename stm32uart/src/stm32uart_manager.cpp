//file stm32uart_manager.cpp

#include "stm32uartConfig.h"
#include "stm32uart.h"
#include "stm32uart_manager.h"

namespace stm32uart {

  
UartManager::UartManager(){
  //default constructor
}

UartManager::UartManager(const UartSettings &uart_settings){
  
  inbox_ = MessageBox(uart_settings);
  outbox_ = MessageBox(uart_settings);
    
  rx_buffer_ = new CircularBuffer(uart_settings.rx_buffer_size);
  tx_buffer_ = new CircularBuffer(uart_settings.tx_buffer_size);
  
  if( (rx_buffer_ == nullptr) || (tx_buffer_ == nullptr) || (!tx_buffer_->IsValid()) || (!rx_buffer_->IsValid()) ){
    delete tx_buffer_;
    delete rx_buffer_;
    valid_ = false;
  }
  else 
    valid_ = true;
}

UartManager::~UartManager(){
  if(valid_){
    delete tx_buffer_;
    delete rx_buffer_;
  }
}
  
bool UartManager::IsValid() const{
  return valid_;
}

bool UartManager::OutboxEmpty(){
  return outbox_.Empty();
}

ReturnState UartManager::FillTxBuffer(){

  String new_tx_buffer_content = "";
  
  if( !outbox_.GetNextChunk( tx_buffer_->AllocatedSize(), &new_tx_buffer_content ) )
    return kOk;
  
  tx_buffer_->PushChunk(new_tx_buffer_content);
  return kOk;
}

ReturnState UartManager::TakeDataFromRxBuffer(){
  
  BufferElement element = 0;
  String new_string = "";
  
  while(rx_buffer_->PopFront(&element) == kOk){
    new_string += element;
  }
  
  return inbox_.DropRawString(new_string);
}

CircularBuffer* UartManager::GetRxBufferAddress(){
  return rx_buffer_;
}
                                                                                        
CircularBuffer* UartManager::GetTxBufferAddress(){
  return tx_buffer_;
}

      
ReturnState UartManager::AddMessageToOutbox(const String message){
  return outbox_.Put(message);
}
                                                                                        
ReturnState UartManager::TakeMessageFromInbox(String *message){
  return inbox_.GetNext(message);
}


}               //namespace stm32uart