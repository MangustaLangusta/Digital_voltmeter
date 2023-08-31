//file stm32adc_port.cpp
//port for stm32f103c8 (BluePill)

#include <list>
#include <set>

#include "stm32f1xx.h"

#include "stm32adcConfig.h"
#include "stm32adc.h"

namespace stm32adc {
  
static const std::set<AdcChannel> port_available_channels = { kCh0, kCh1, kCh2, kCh3, kCh4, kCh5, kCh6, kCh7, kCh8, kCh9 };
  
extern const int port_kAvailableAdcChannelsAmount = 16;
  
static std::set<AdcHardwareNumber> active_adc = {};

static bool IsAdcActive(const AdcHardwareNumber adc_number){
  return active_adc.find(adc_number) != active_adc.end();
}
  
static ADC_TypeDef* GetAdcBase(const AdcHardwareNumber adc_number){
  switch(adc_number){
  case kAdc1:
    return ADC1;
  case kAdc3:
    return nullptr;
  default:
    return nullptr;
  }
}

static DMA_Channel_TypeDef* GetDmaChannelBase(const AdcHardwareNumber adc_number){
  switch(adc_number){
  case kAdc1:
    return DMA1_Channel1;
  case kAdc3:
    return nullptr;
  default:
    return nullptr;
  }  
}

static ReturnState EnableAdcClock(const AdcHardwareNumber adc_number){
  switch(adc_number){
  case kAdc1:
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;         //clock source ADC1
    return kOk;
  case kAdc3:
    return kError;
  default:
    return kError;
  }  
}  

  
static ReturnState portCheckChannelsValidity(const std::list<AdcChannel> &channels){
  for(auto it : channels){
    if(port_available_channels.find(it) == port_available_channels.end())
      return kError;
  }
  return kOk;
}


static ReturnState portUpdateChannelsSequence(const AdcHardwareNumber adc_number, const std::list<AdcChannel> &channels_list){
  
  if( !IsAdcActive(adc_number))
    return kAdcNotInitialised;
  
  ADC_TypeDef* selected_adc = GetAdcBase(adc_number);
  DMA_Channel_TypeDef* selected_dma = GetDmaChannelBase(adc_number);
  
  if( (selected_adc == nullptr) || (selected_dma == nullptr) )
    return kError;
  
  selected_adc->SQR1 = 0;
  selected_adc->SQR2 = 0;
  selected_adc->SQR3 = 0;
  
  int ch_order = 0;
  for(auto it = channels_list.begin(); it != channels_list.end(); it++){
    if(ch_order < 8)
      selected_adc->SQR3 |= (*it << (ch_order * 5));
    else if (ch_order < 13)
      selected_adc->SQR2 |= (*it << ((ch_order-8) * 5));
    else if (ch_order < 17)
      selected_adc->SQR1 |= (*it << ((ch_order-13) * 5));
    ch_order++;
  }
  
  int channels_amount = channels_list.size() > 0 ? channels_list.size() - 1 : 0;
  selected_adc->SQR1 |= (channels_amount << ADC_SQR1_L_Pos);
  selected_dma->CNDTR = channels_list.size();
  return kOk;
}


static ReturnState portSuspendAdcConversion(const AdcHardwareNumber adc_number){
  
  if( !IsAdcActive(adc_number))
    return kAdcNotInitialised;
  
  ADC_TypeDef* selected_adc = GetAdcBase(adc_number);
  DMA_Channel_TypeDef* selected_dma_channel = GetDmaChannelBase(adc_number);
  
  if( (selected_adc == nullptr) || (selected_dma_channel == nullptr) )
    return kError;
  
  selected_adc->CR2 &= ~ADC_CR2_CONT;    //ifnish continious mode
  selected_adc->CR2 &= ~ADC_CR2_DMA;     //disable DMA request  
  selected_dma_channel->CCR &= ~DMA_CCR_EN;    //Switch off adc1 channel DMA  
  
  return kOk;
}

static ReturnState portResumeAdcConversion(const AdcHardwareNumber adc_number){
  if( !IsAdcActive(adc_number))
    return kAdcNotInitialised;
  
  ADC_TypeDef* selected_adc = GetAdcBase(adc_number);
  DMA_Channel_TypeDef* selected_dma_channel = GetDmaChannelBase(adc_number);
  
  if( selected_dma_channel->CNDTR == 0 )
    return kOk;
  
  if( (selected_adc == nullptr) || (selected_dma_channel == nullptr) )
    return kError;
  
  selected_dma_channel->CCR |= DMA_CCR_EN;    //Switch on adc1 channel DMA  
  selected_adc->CR2 |= ADC_CR2_DMA;     //enable DMA request  
  selected_adc->CR2 |= ADC_CR2_CONT;    //start continious mode
  selected_adc->CR2 |= ADC_CR2_ADON;    //start conversion;
  return kOk;
}


static ReturnState portPrepareChannelGpioPin(const AdcChannel channel){

  if(channel <= kCh7){
    RCC->APB2ENR |= (1 << RCC_APB2ENR_IOPAEN_Pos);  //Allow clock source of GPIOA
    GPIOA->CRL &= ( ~(GPIO_CRL_MODE0_Pos + (channel * 4)) );    //mode = 00 (input)
    GPIOA->CRL &= ( ~(GPIO_CRL_CNF0_Pos + (channel * 4)) );      //cnf = 00 (analog input)
  }
  else if(channel <= kCh9){
    RCC->APB2ENR |= (1 << RCC_APB2ENR_IOPBEN_Pos);
    GPIOB->CRH &= ( ~(GPIO_CRH_MODE8_Pos + ((channel-kCh8) * 4)) );    //mode = 00 (input)
    GPIOB->CRH &= ( ~(GPIO_CRH_CNF8_Pos + ((channel-kCh8) * 4)) );      //cnf = 00 (analog input)
  }
  else if(channel <= kCh15){
    RCC->APB2ENR |= (1 << RCC_APB2ENR_IOPCEN_Pos);
    GPIOC->CRL &= ( ~(GPIO_CRL_MODE0_Pos + ((channel-kCh10) * 4)) );    //mode = 00 (input)
    GPIOC->CRL &= ( ~(GPIO_CRL_CNF0_Pos + ((channel-kCh10) * 4)) );      //cnf = 00 (analog input)
  }
  else {
    return kError;
  }
  
  return kOk;
}


ReturnState portChannelAvailable(const AdcChannel channel_to_add){
  if(port_available_channels.find(channel_to_add) == port_available_channels.end())
    return kError;
  return kOk;
}


ReturnState portInitAdc(const AdcHardwareNumber adc_number, AdcValue* buffer_address){
  
  ADC_TypeDef* selected_adc = GetAdcBase(adc_number);
  DMA_Channel_TypeDef* selected_dma_channel = GetDmaChannelBase(adc_number);
  
  if( (selected_adc == nullptr) || (selected_dma_channel == nullptr) )
    return kError;
  
  //prescaler ADC
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;      
  
  //Enable clock source for selected adc
  if(EnableAdcClock(adc_number) != kOk)
    return kError;
  
  // prohibit everything in control registers
  selected_adc->CR1 = 0;      
  selected_adc->CR2 = 0;
  
  //setup adc cycle duration    (239.5)
  selected_adc->SMPR2 |= ADC_SMPR2_SMP0 | ADC_SMPR2_SMP1 
  | ADC_SMPR2_SMP2 | ADC_SMPR2_SMP3 | ADC_SMPR2_SMP4 
  | ADC_SMPR2_SMP5 | ADC_SMPR2_SMP6 | ADC_SMPR2_SMP7
  | ADC_SMPR2_SMP8 | ADC_SMPR2_SMP9;       
  
  selected_adc->SMPR1 |= ADC_SMPR1_SMP10 | ADC_SMPR1_SMP11 
  | ADC_SMPR1_SMP12 | ADC_SMPR1_SMP13 | ADC_SMPR1_SMP14 
  | ADC_SMPR1_SMP15 | ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;   

  //all zeros mean that one regular channel and ch0 is the first (and the only one)
  selected_adc->SQR1 = 0; // 1 регулярный канал
  selected_adc->SQR2 = 0;
  selected_adc->SQR3 = 0; // 1е преобразование - канал 0
  
  
  //DMA initialization
  selected_dma_channel->CCR &= ~DMA_CCR_EN; 
  
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;           //DMA1 clock source
  
  //////////
  selected_dma_channel->CPAR = 0;
  selected_dma_channel->CPAR = (uint32_t) &(selected_adc->DR);     //peripheral address
  
  selected_dma_channel->CMAR = 0;
  selected_dma_channel->CMAR = (uint32_t) buffer_address;     //AdcManager buffer address
    
  
  selected_dma_channel->CNDTR = 0;                           //amount of data to be transferred
    
  selected_dma_channel->CCR = DMA_CCR_MINC                   //mem increment
    | DMA_CCR_CIRC;                                           //circular mode 
  
  selected_dma_channel->CCR |= (1 << DMA_CCR_MSIZE_Pos);       //Size of memory cell = 16bit
  selected_dma_channel->CCR |= (1 << DMA_CCR_PSIZE_Pos);       //Size of periph cell = 16 bit
    
  selected_dma_channel->CCR |= DMA_CCR_EN;                  //Enable DMA for ADC
  ///////
          
  selected_adc->CR1 |= ADC_CR1_SCAN;
  
  selected_adc->CR2 |= ADC_CR2_ADON; // prepare ADC (for starting need to be done one more time -- it will be done later)  
  
  active_adc.insert(adc_number);        //add to active adc set
  
  return kOk;
}

ReturnState portPerformScanning(const AdcHardwareNumber adc_number, const std::list<AdcChannel> &channels){
  
  if( portCheckChannelsValidity( channels ) != kOk )
    return kError;
   
  if( portSuspendAdcConversion( adc_number ) != kOk )
    return kError;
 
  if(portUpdateChannelsSequence( adc_number, channels ) != kOk)
    return kError;
  
  for(auto it = channels.begin(); it != channels.end(); it++)
    portPrepareChannelGpioPin( *it );

  if( portResumeAdcConversion( adc_number ) != kOk )
    return kError;

  return kOk;  
}

}               //namespace stm32adc