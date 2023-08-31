#include <set>

#include "stm32f1xx.h"
#include "stm32uart.h"
#include "stm32uart_buffer.h"

namespace stm32uart {
  
static const std::set<UartHardwareNumber> available_uarts = {kUart1, kUart2, kUart3};


#ifdef uart_configENABLE_UART1

  //all inputs should be valid
static ReturnState portInitUart1(const UartSettings &uart_settings, const CircularBuffer *rx_buffer, const CircularBuffer *tx_buffer){
  
    //Enable UART1 clock source
  RCC->APB2ENR |= (1 << RCC_APB2ENR_USART1EN_Pos);     
    //Allow clock source of GPIOA
  RCC->APB2ENR |= (1 << RCC_APB2ENR_IOPAEN_Pos);      
    //Enable DMA1 clock source
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;           
  
    //Set PA9 to alternate function (TX1)
    // CNF = 10    MODE = X1
  GPIOA->CRH &= (~GPIO_CRH_CNF9_0);
  GPIOA->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9);
  
  //Set PA10 to input-pullup
  // MODE = 00, CNF = 10, ODR = 1
  GPIOA->CRH  &= (~(GPIO_CRH_MODE10));
  GPIOA->CRH &= (~(GPIO_CRH_CNF10_0));
  GPIOA->CRH |= GPIO_CRH_CNF10_1;
  GPIOA->BSRR |= GPIO_ODR_ODR10;
  
  //===   Configure UART1  ====
  
  //allow USART1, clear other bits
  USART1->CR1 = USART_CR1_UE;   
  
  //Prohibit interrupts
  USART1->CR2 = 0;
  USART1->CR3 = 0;
  
  //Uart speed
  //USART1->BRR = Fcpu / BAUD
  USART1->BRR = GeneralSettings::GetCpuFrequency() / uart_settings.speed;         
  
  //=== Configure DMA1 ====
  //UART1 TX - CH4
  //UART1 RX - CH5
  
  //Ensure that DMA channels are off
  DMA1_Channel4->CCR &= ~DMA_CCR_EN;         
  DMA1_Channel5->CCR &= ~DMA_CCR_EN;
  
  //peripheral addresses
  DMA1_Channel4->CPAR = (uint32_t) (&USART1->DR);     
  DMA1_Channel5->CPAR = (uint32_t) (&USART1->DR);
  
  //receiver and transmitter buffers
  DMA1_Channel4->CMAR = (uint32_t) (tx_buffer->StartAddress());        
  DMA1_Channel5->CMAR = (uint32_t) (rx_buffer->StartAddress());
  
  //amount of data to be transferred
  DMA1_Channel4->CNDTR = 0;                           
  DMA1_Channel5->CNDTR = rx_buffer->AllocatedSize();
  
  //memory address increment
  //direction: memory -> peripheral
  DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_DIR;  
  
  //mempry address increment
  //circular mode
  DMA1_Channel5->CCR = DMA_CCR_MINC | DMA_CCR_CIRC;  
  
  //Uart -> DMA transfer enabled
  DMA1_Channel5->CCR |= DMA_CCR_EN; 
  //DMA -> Uart transfer (Tx) will be enabled later, when needed
 
  //Enabled work with DMA in UART
  USART1->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;   
  //Enable UART Rx (Tx will be allowed later, when needed)
  USART1->CR1 |= USART_CR1_RE;  
  
  return kOk;
}

#endif  //uart_configENABLE_UART1
  
static ReturnState portIsUartAvailable(const UartHardwareNumber uart_number){
  if(available_uarts.find(uart_number) != available_uarts.end())
    return kOk;
  return kError;
}
  
ReturnState portInitUart(const UartHardwareNumber uart_number, 
                         const UartSettings &uart_settings, 
                         const CircularBuffer *rx_buffer, 
                         const CircularBuffer *tx_buffer){
  
  //check that intended uart is available
  if(portIsUartAvailable(uart_number) == kError)
    return kError;
  
  //check that cpu frequency is set
  if(!GeneralSettings::GetCpuFrequency())
    return kError;
  
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
    return portInitUart1( uart_settings, rx_buffer, tx_buffer );
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    return portInitUart2( uart_settings );
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    return portInitUart3( uart_settings );
#endif  //uart_configENABLE_UART3
    
  default:
    return kError;
  }
}


ReturnState portStopTx(const UartHardwareNumber uart_number){
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
      //Disable UART Tx
    USART1->CR1 &= ~USART_CR1_TE; 
    break;
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    break;
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    break;
#endif  //uart_configENABLE_UART3
    
  default:
    return kError;
  }  
  return kOk;
}


BufferSize portTxBytesToSend(const UartHardwareNumber uart_number){
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
    return DMA1_Channel4->CNDTR;
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    break;
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    break;
#endif  //uart_configENABLE_UART3
    
  default:
    return 0;
  }  
}


ReturnState portSetupTx(const UartHardwareNumber uart_number, const BufferSize length){
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
    DMA1_Channel4->CCR &= ~DMA_CCR_EN; 
    DMA1_Channel4->CNDTR = length;
    DMA1_Channel4->CCR |= DMA_CCR_EN; 
    break;
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    break;
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    break;
#endif  //uart_configENABLE_UART3
    
  default:
    return kError;
  }  
  
  return kOk;
}


ReturnState portResumeTx(const UartHardwareNumber uart_number){
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
    //Enable UART Tx
    
    USART1->CR1 |= USART_CR1_TE; 
    break;
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    break;
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    break;
#endif  //uart_configENABLE_UART3
    
  default:
    return kError;
  }  
  
  return kOk;
}  


int portGetCurrentRxBufferIndex(const UartHardwareNumber uart_number){
  //choose correct function
  //only uarts, enabled in config file, are available
  switch (uart_number){
    
#ifdef uart_configENABLE_UART1
  case kUart1:
    return DMA1_Channel5->CNDTR;
#endif  //uart_configENABLE_UART1
    
#ifdef uart_configENABLE_UART2   
  case kUart2:
    break;
#endif  //uart_configENABLE_UART2
    
#ifdef uart_configENABLE_UART3
  case kUart3:
    break;
#endif  //uart_configENABLE_UART3
    
  default:
    return 0;
  }  
 
}
  
}               //namespace stm32uart
















 
 


