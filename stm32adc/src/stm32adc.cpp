//file stm32adc.cpp

#include <map>
#include <utility>

#include "stm32adc.h"
#include "stm32adc_manager.h"

namespace stm32adc {
  
static std::map<AdcHardwareNumber, AdcManager> active_adc_map = {};  

static AdcManager* GetAdcManager(const AdcHardwareNumber adc_number){
  auto active_adc_it = active_adc_map.find(adc_number);
  if(active_adc_it == active_adc_map.end())
    return nullptr;
  
  return &(active_adc_it->second);
}
  

ReturnState InitAdc( const AdcHardwareNumber adc_number, const AdcConfiguration &configuration){
  
  active_adc_map.erase(adc_number);
  active_adc_map.emplace( std::piecewise_construct, std::forward_as_tuple(adc_number), std::forward_as_tuple(adc_number, configuration) );
  
  AdcManager *new_adc_manager = GetAdcManager(adc_number);
  
  if(new_adc_manager == nullptr)
    return kError;
  
  if( new_adc_manager->Init( adc_number ) == kError ){
    active_adc_map.erase(adc_number);
    return kError;
  }
  return kOk;  
}
  
ReturnState AddChannelToScanList( const AdcHardwareNumber adc_number, const AdcChannel new_channel ){
  
  AdcManager* adc_manager = GetAdcManager(adc_number);
  
  if(adc_manager == nullptr)
    return kAdcNotInitialised;

  return adc_manager->AddChannelToScanList( new_channel );
}

ReturnState GetCurrentValue( const AdcHardwareNumber adc_number, const AdcChannel channel, AdcValue *value ){
  
  AdcManager* adc_manager = GetAdcManager(adc_number);
  
  if(adc_manager == nullptr)
    return kAdcNotInitialised;
  
  return adc_manager->GetChannelValue( channel, value ); 
}

ReturnState RemoveChannelFromScanList( const AdcHardwareNumber adc_number, const AdcChannel channel_to_remove ){ 
   
  AdcManager* adc_manager = GetAdcManager(adc_number);
  
  if(adc_manager == nullptr)
    return kAdcNotInitialised;
  
  return adc_manager->RemoveChannelFromScanList( channel_to_remove );
}

}                       //namespace stm32adc