#ifndef STM32UART_MANAGER_H
#define STM32UART_MANAGER_H

#include "stm32uartConfig.h"
#include "stm32uart.h"
#include "stm32uart_buffer.h"
#include "stm32uart_messages.h"

namespace stm32uart {

class UartManager{
private:
  MessageBox inbox_;             
  MessageBox outbox_;                                        
  
  CircularBuffer* rx_buffer_;
  CircularBuffer* tx_buffer_;
  
  bool valid_;
  
  UartManager();
public:
  UartManager(const UartSettings &uart_settings);
  ~UartManager();
  
  bool IsValid() const;
  
  bool OutboxEmpty();
  
  ReturnState FillTxBuffer();
  ReturnState TakeDataFromRxBuffer();
  
  CircularBuffer* GetRxBufferAddress();
  CircularBuffer* GetTxBufferAddress();
     
  ReturnState AddMessageToOutbox(const String message);
  ReturnState TakeMessageFromInbox(String *message);
};

}               //namespace stm32uart


#endif                  //STM32UART_MANAGER_H