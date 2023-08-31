#ifndef STM32_ADC_MANAGER_H
#define STM32_ADC_MANAGER_H

#include <list> 

#include "stm32adcConfig.h"
#include "stm32adc.h"

namespace stm32adc{

class AdcManager{
private:
  
  AdcHardwareNumber adc_number_;
  
  int allocated_buffer_size_;
  AdcValue* buffer_;
  
  std::list< AdcChannel > channels_;
  
  bool initialised_;
  
  AdcManager() = delete;
  AdcManager(const AdcManager&) = delete;
  
  bool HaveChannelInScanList(const AdcChannel channel);
  int GetChannelIndex(const AdcChannel channel);
  
  void InvalidateBufferValues();
public:
  
  AdcManager( const AdcHardwareNumber adc_number, const AdcConfiguration &configuration );
  ~AdcManager();
  
  ReturnState Init( const AdcHardwareNumber adc_number );
  
  ReturnState AddChannelToScanList( const AdcChannel new_channel );
  
  ReturnState GetChannelValue( const AdcChannel channel, AdcValue* value );
  
  ReturnState RemoveChannelFromScanList( const AdcChannel channel_to_remove );
};

}               //namespace stm32adc

#endif                  //STM32_ADC_MANAGER_H