//CMSIS
#include "stm32f1xx.h"

//FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "portmacro.h"
#include "timers.h"

//task specific
#include "stm32uart.h"
#include "stm32adc.h"
#include "led_blinker.h"
#include "voltmeter.h"


void LEDBlinkTask                       (void * parameters);
void UartRxTask                         (void * parameters);
void VoltmeterRoutineTask               (void * parameters);
void UartTxTask                         (void * parameters);
void AdcTestTask                        (void * parameters);

bool InitRCC();

int main(){
  
  //Initialisation of RCC
  InitRCC();

  //Initialisation of UART1
  stm32uart::InitUart( stm32uart::kUart1, stm32uart::kDefaultSettings );
  
  //Initialisation of ADC1
  stm32adc::InitAdc( stm32adc::kAdc1, stm32adc::kDefaultAdcConfiguration );

  
  xTaskCreate(LEDBlinkTask, 
              "", 
              configMINIMAL_STACK_SIZE, 
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);

  xTaskCreate(UartRxTask,
              "",
              256,
              NULL,
              tskIDLE_PRIORITY + 2,
              NULL);    

  xTaskCreate(UartTxTask,
              "",
              256,
              NULL,
              tskIDLE_PRIORITY + 2,
              NULL);    

  xTaskCreate(VoltmeterRoutineTask,
              "",
              512,
              NULL,
              tskIDLE_PRIORITY + 1,
              NULL);

  vTaskStartScheduler();
  
  return 0;
}

void LEDBlinkTask(void * parameters){
  using namespace LedBlinker;
  
  const int kIdlePeriod = 500;
  const int kErrorPeriod = 500;
  const int kMeasuringPeriod = 100;
  Led::Init();
  Led::SetPeriod(kIdlePeriod);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  voltmeter::VoltmeterState current_state = voltmeter::Voltmeter::GetState();
  
  for( ; ; ){
    voltmeter::VoltmeterState new_state = voltmeter::Voltmeter::GetState();
    
    if(new_state != current_state){
      switch(new_state){
      case voltmeter::kVoltmeterIdle:
        Led::SetPeriod(kIdlePeriod);
        break;
      case voltmeter::kVoltmeterError:
        Led::LedOn();
        Led::SetPeriod(kErrorPeriod);
        break;
      case voltmeter::kVoltmeterMeasuring:
        Led::SetPeriod(kMeasuringPeriod);
        break;  
      
      }
      current_state = new_state;
    }
    
    vTaskDelayUntil( &xLastWakeTime, Led::GetPeriod() );
    if(current_state != voltmeter::kVoltmeterError)
      Led::Toggle();
  };
}

void UartRxTask( void * parameters){
  const int kUartRxPeriod = 100;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for( ; ; ){
    RxRoutine( stm32uart::kUart1 );
    vTaskDelayUntil( &xLastWakeTime, kUartRxPeriod ); 
  }
}

void UartTxTask( void * parameters){
  const int kUartTxPeriod = 50;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for( ; ; ){
    TxRoutine( stm32uart::kUart1 );
    vTaskDelayUntil( &xLastWakeTime, kUartTxPeriod ); 
  }
}


void VoltmeterRoutineTask(void * parameters){
  const int kVoltmeterTaskPeriod = 100;  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  stm32uart::SendMessage(stm32uart::kUart1, "Voltmeter started");
  for( ; ; ){
    std::string new_message = "";

    if(GetPendingMessage( stm32uart::kUart1 , &new_message ) == stm32uart::kOk){
      voltmeter::Voltmeter::IncomingMessage(new_message);
    }
    
    voltmeter::Voltmeter::UpdateState();
    
    vTaskDelayUntil( &xLastWakeTime, kVoltmeterTaskPeriod );
  }
}





bool InitRCC(){

  RCC->CR |= (1<<RCC_CR_HSEON_Pos); //Start HSE
  
  for(int i = 0; ; i++)
  {
    //break on success
    if(RCC->CR & (1<<RCC_CR_HSERDY_Pos))
      break;

    if(i > 10'000)
    {
      RCC->CR &= ~(1<<RCC_CR_HSEON_Pos); //Stop HSE
      return false;
    }
  }
  
  //Setup PLL
  RCC->CFGR = 0;
  RCC->CFGR |= (0x07<<RCC_CFGR_PLLMULL_Pos) //PLL multilier = 9
            | (0x01<<RCC_CFGR_PLLSRC_Pos); // PLL source from HSE
  
  RCC->CR |= (1<<RCC_CR_PLLON_Pos); //Start PLL
  
  for(int i = 0; ; i++)
  {
    //break on success
    if(RCC->CR & (1<<RCC_CR_PLLRDY_Pos))
      break;
    
    if(i > 10'000)
    {
      RCC->CR &= ~(1<<RCC_CR_HSEON_Pos); //Stop HSE
      RCC->CR &= ~(1<<RCC_CR_PLLON_Pos); //Stop PLL
      return false;
    }
  }
  
  //2 wait cycles for Flash
  FLASH->ACR |= (0x02<<FLASH_ACR_LATENCY_Pos); 
  
  //divisors of buses
  RCC->CFGR |= (0x00<<RCC_CFGR_PPRE2_Pos) //APB2 = 1
            | (0x04<<RCC_CFGR_PPRE1_Pos) //APB1 = 2
            | (0x00<<RCC_CFGR_HPRE_Pos); //AHB = 1
  
  
  RCC->CFGR |= (0x02<<RCC_CFGR_SW_Pos); //Switch to PLL
  
  //wait for switch
  while((RCC->CFGR & RCC_CFGR_SWS_Msk) != (0x02<<RCC_CFGR_SWS_Pos)) {
    ;
  }
  
  //HSI off
  RCC->CR &= ~(1<<RCC_CR_HSION_Pos);
  
  return true;
}
