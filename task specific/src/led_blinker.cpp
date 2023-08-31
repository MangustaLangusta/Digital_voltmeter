#include "stm32f1xx.h"

#include "led_blinker.h"

namespace LedBlinker {
  
  const int DEFAULT_BLINK_PERIOD = 1000;
  LedState Led::current_state = LED_IS_OFF;
  int Led::blink_period = DEFAULT_BLINK_PERIOD;

ReturnStatus Led::Init(){
  
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;   //Allow clock source of GPIOC
  
  GPIOC->CRH |= GPIO_CRH_MODE13;
  GPIOC->CRH &= (~(GPIO_CRH_CNF13));
  
  LedOff();
  return RETURN_OK;
}

void Led::LedOn(){
  GPIOC->BRR |= GPIO_ODR_ODR13;
  current_state = LED_IS_ON;
}

void Led::LedOff(){
  GPIOC->BSRR |= GPIO_ODR_ODR13;
  current_state = LED_IS_OFF;
}

LedState Led::Toggle(){
  switch (current_state) {
  case LED_IS_ON:
    LedOff();
    break;
  case LED_IS_OFF:
    LedOn();
    break;
  }
  return current_state;
}

void Led::SetPeriod(int new_period){
  blink_period = new_period;
}

int Led::GetPeriod() {
  return blink_period;
}

}       //namespace LedBlinker