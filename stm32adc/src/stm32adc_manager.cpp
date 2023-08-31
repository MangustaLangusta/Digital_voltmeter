//file stm32adc_manager.cpp
#include <list>

#include "stm32adc.h"
#include "stm32adc_manager.h"

namespace stm32adc {
  
extern const int port_kAvailableAdcChannelsAmount;

extern ReturnState portChannelAvailable(const AdcChannel channel_to_add);
extern ReturnState portInitAdc(const AdcHardwareNumber adc_number, AdcValue* buffer_address);
extern ReturnState portPerformScanning(const AdcHardwareNumber adc_number, const std::list<AdcChannel> &channels);
  
static const int kInvalidIndex = -1;


bool AdcManager::HaveChannelInScanList(const AdcChannel channel){
  for(auto it = channels_.begin(); it != channels_.end(); it++){
    if(*it == channel)
      return true;
  }
  return false;
}


int AdcManager::GetChannelIndex(const AdcChannel channel){
  int index = 0;
  for(auto it = channels_.begin(); it != channels_.end(); it++){
    if(*it == channel)
      return index;
    index++;
  }
  return kInvalidIndex;
}


void AdcManager::InvalidateBufferValues(){
  if(!initialised_)
    return;
  for(int i = 0; i < allocated_buffer_size_; i++)
    buffer_[i] = kInvalidValue;
}


AdcManager::AdcManager( const AdcHardwareNumber adc_number, const AdcConfiguration &configuration ) {
  adc_number_ = adc_number;
  channels_ = {};
  initialised_ = false;
  
  if( configuration.max_simultaneously_scanned_channels > port_kAvailableAdcChannelsAmount )
    allocated_buffer_size_ = port_kAvailableAdcChannelsAmount;
  else
    allocated_buffer_size_ = configuration.max_simultaneously_scanned_channels;
  
  buffer_ = new AdcValue[allocated_buffer_size_]; 
}


AdcManager::~AdcManager() {
  delete buffer_;
}


ReturnState AdcManager::Init( const AdcHardwareNumber adc_number ) {
  
  if(buffer_ == nullptr)
    return kError;
  
  ReturnState init_status = portInitAdc( adc_number, buffer_ );
  
  if(init_status == kOk)
    initialised_ = true;
  
  return init_status;
}


ReturnState AdcManager::AddChannelToScanList( const AdcChannel new_channel ) {
  
  if(!initialised_)
    return kError;
  
  if( HaveChannelInScanList(new_channel) )
    return kChannelAlreadyActive;
  
  if( portChannelAvailable(new_channel) != kOk)
    return kError;
  
  if( channels_.size() >= allocated_buffer_size_ )
    return kChannelsLimitReached;
    
  channels_.push_back(new_channel);
  
  InvalidateBufferValues();
  
  if( portPerformScanning( adc_number_, channels_ ) != kOk ) {
    channels_.pop_back();
    portPerformScanning( adc_number_, channels_ ); 
    return kError;
  }
  
  return kOk;
}


ReturnState AdcManager::GetChannelValue( const AdcChannel channel, AdcValue* value ) {
  *value = kInvalidValue;
  
  if(!initialised_)
    return kError;
  
  int index = GetChannelIndex( channel );
  
  if(index == kInvalidIndex)
    return kChannelNotActive;
  
  *value = buffer_[index];

  return kOk;
}


ReturnState AdcManager::RemoveChannelFromScanList( const AdcChannel channel_to_remove ) {
  
  if(!initialised_)
    return kError;
  
  for(auto it = channels_.begin(); it != channels_.end(); it++){
    if(*it == channel_to_remove){
      channels_.erase(it);
      portPerformScanning( adc_number_, channels_ );
      break;
    }
  }
  
  return kOk;
}


}               //namespace stm32adc