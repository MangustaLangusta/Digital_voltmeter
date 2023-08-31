#ifndef STM32_UART_H
#define STM32_UART_H

#include <string>
#include <list>

#include "stm32uartConfig.h"


namespace stm32uart {
  
//=========================  TYPES  ===============================//

typedef enum {  kOk, 
                kError,
                kNoPendingMessages,
                kBufferFull,
                kBufferEmpty,
                kBufferOverfill,
                kMessageBoxOverfill,
                kUartNotInitialised
                                          }             ReturnState;

typedef enum {kUart1, kUart2, kUart3, kUart4, kUart5}   UartHardwareNumber;
typedef enum {kParityNone, kParityOdd, kParityEven}     Parity;
typedef enum {kStopBitsOne, kStopBitsTwo}               StopBits;
typedef enum {kPocketSizeEight, kPocketSizeNine}        PocketSize;
typedef int                                             Speed;
  
typedef std::string                                     String;

typedef int                                             BufferSize;
typedef unsigned char                                   BufferElement;

struct UartSettings {
  Speed         speed                   = uart_configDEFAULT_SPEED;
  StopBits      stop_bits               = kStopBitsOne;
  Parity        parity                  = kParityNone;
  PocketSize    pocket_size             = kPocketSizeEight;
  BufferSize    rx_buffer_size          = uart_configDEFAULT_RX_BUFFER_SIZE;
  BufferSize    tx_buffer_size          = uart_configDEFAULT_TX_BUFFER_SIZE;
  int           max_message_length      = uart_configDEFAULT_MAX_MESSAGE_LENGTH;
  int           max_messages_stored     = uart_configDEFAULT_MAX_MESSAGES_STORED;
  String        eol_symbol              = uart_configDEFAULT_EOL_SYMBOL;
};

class GeneralSettings {
private:
  static unsigned long cpu_frequency_;
  GeneralSettings();
public:
  static void SetCpuFrequency(long new_frequency);
  static unsigned long GetCpuFrequency();
};

class MessageBox;
class CircularBuffer;
class UartManager;


const UartSettings kDefaultSettings;


//========================= INTERFACE METHODS ==================//



  //Initialisation of Uart
  //-manager_settings- and -protocol_setitngs- can be replaced by default constants
  //kDefaultProtocolSettings and kDefaultUartManagerSettings respectively
  //            Possible returns:
  //    kOk                     : initialised successfuly
  //    kError                  : not initialised due to some error
ReturnState InitUart(const UartHardwareNumber uart_number, const UartSettings &uart_settings);

  //Adds -message- to outbox of uart -uart_number-
  //            Possible returns:
  //    kOk                     : message successfuly added to outbox
  //    kMessageBoxOverfill     : message is not added due to exceeded messages amount in outbox
  //    kUartNotInitialised     : requested uart does not exist
ReturnState SendMessage(const UartHardwareNumber uart_number, const String message);

  //Checks inbox of uart -uart_number-
  //If inbox empty, -*rx_message- sets equal to nullptr
  //If inbox not empty, first message in line (FIFO) to be shifted to -*rx_message- address
  //after this message is being removed from inbox
  //            Possible returns:
  //    kOk                     : message relocated to -*rx_message- address
  //    kNoPendingMessages      : inbox empty, -*rx_message- set to nullptr
  //    kUartNotInitialised     : requested uart does not exist
ReturnState GetPendingMessage(const UartHardwareNumber uart_number, String *rx_message);

  //To be executed as frequently as deemed reasonable taking into account uart speed, buffers' sizes and desired response time
  //            Possible returns:
  //    kOk                     : executed normally
  //    kUartNotInitialised     : requested uart does not exist
  //    kError                  : other error
ReturnState TxRoutine(const UartHardwareNumber uart_number);

  //To be executed as frequently as deemed reasonable taking into account uart speed, buffers' sizes and desired response time
  //            Possible returns:
  //    kOk                     : executed normally
  //    kUartNotInitialised     : requested uart does not exist
  //    kError                  : other error
ReturnState RxRoutine(const UartHardwareNumber uart_number);
  
}       //namespace stm32uart

#endif  //STM32_UART_H