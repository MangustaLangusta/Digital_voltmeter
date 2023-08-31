#include <map>

#include "stm32uartConfig.h"
#include "stm32uart.h"
#include "stm32uart_manager.h"

namespace stm32uart {
  
extern ReturnState portInitUart(const UartHardwareNumber uart_number, 
                                const UartSettings &uart_settings, 
                                const CircularBuffer *rx_buffer, 
                                const CircularBuffer *tx_buffer);

extern ReturnState portStopTx(const UartHardwareNumber uart_number);
extern BufferSize portTxBytesToSend(const UartHardwareNumber uart_number);
extern ReturnState portSetupTx(const UartHardwareNumber uart_number, const BufferSize length);
extern ReturnState portResumeTx(const UartHardwareNumber uart_number);
extern BufferSize portGetCurrentRxBufferIndex(const UartHardwareNumber uart_number);

unsigned long GeneralSettings::cpu_frequency_ = uart_configDEFAULT_CPU_FREQUENCY;
  
static std::map<UartHardwareNumber, UartManager> active_uarts = {};

void GeneralSettings::SetCpuFrequency(long new_frequency){
  cpu_frequency_ = new_frequency;
}

unsigned long GeneralSettings::GetCpuFrequency(){
  return cpu_frequency_;
}

static UartManager* GetUartManager(const UartHardwareNumber uart_number){
  
  auto active_uarts_it = active_uarts.find(uart_number);
  if(active_uarts_it == active_uarts.end())
    return nullptr;
  
  return &(active_uarts_it->second);
}

ReturnState InitUart(const UartHardwareNumber uart_number, const UartSettings &uart_settings) {
  
  active_uarts.erase(uart_number);
  active_uarts.emplace(std::make_pair(  uart_number, uart_settings ) );
  
  UartManager *new_uart_manager = GetUartManager(uart_number);
  
  if(new_uart_manager == nullptr)
    return kError;
  
  CircularBuffer const *rx_buffer = new_uart_manager->GetRxBufferAddress();
  CircularBuffer const *tx_buffer = new_uart_manager->GetTxBufferAddress();
  
  if( portInitUart( uart_number, uart_settings, rx_buffer, tx_buffer ) == kError ){
    active_uarts.erase(uart_number);
    return kError;
  }
  return kOk;
}
  

ReturnState SendMessage(const UartHardwareNumber uart_number, const String message){
  
  UartManager* uart_manager = GetUartManager(uart_number);
  
  if(uart_manager == nullptr)
    return kUartNotInitialised;
  
  return uart_manager->AddMessageToOutbox(message + uart_configDEFAULT_EOL_SYMBOL);  
}


ReturnState GetPendingMessage(const UartHardwareNumber uart_number, String *rx_message){
  
  UartManager* uart_manager = GetUartManager(uart_number);
  
  if(uart_manager == nullptr){
    rx_message->clear();
    return kUartNotInitialised;
  }
  
  return uart_manager->TakeMessageFromInbox(rx_message);
}
  

ReturnState TxRoutine(const UartHardwareNumber uart_number){
  UartManager* uart_manager = GetUartManager(uart_number);
  
  if(uart_manager == nullptr)
    return kUartNotInitialised;
  
  if(uart_manager->OutboxEmpty())
    return kNoPendingMessages;
  
  portStopTx(uart_number);
  
  BufferSize bytes_to_send = portTxBytesToSend(uart_number);
  
  if(bytes_to_send == 0){
    CircularBuffer* tx_buffer = uart_manager->GetTxBufferAddress();
    tx_buffer->Reset();
    uart_manager->FillTxBuffer();
    portSetupTx(uart_number, tx_buffer->Length());
  }
  
  portResumeTx(uart_number);

  return kOk;
}



ReturnState RxRoutine(const UartHardwareNumber uart_number){
  UartManager* uart_manager = GetUartManager(uart_number);
  
  if(uart_manager == nullptr)
    return kUartNotInitialised;
 
  CircularBuffer* rx_buffer = uart_manager->GetRxBufferAddress();
  
  rx_buffer->SetHeadIndex( rx_buffer->AllocatedSize() - portGetCurrentRxBufferIndex(uart_number) );
    
  return uart_manager->TakeDataFromRxBuffer();
}

  /*
12 13 14 15 16 17
 t     h    hh
*/


  
  
}               //namespace stm32uart