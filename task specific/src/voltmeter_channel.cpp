//file voltmeter_channel.cpp

#include "voltmeter_channel.h"

namespace voltmeter{

  
// ===============================================================================================//
/*            VOLTAGE ADC RANGE MAP                                                              */
//===============================================================================================//


VoltageAdcRangeMap::VoltageAdcRangeMap(const AdcBounds &new_adc_bounds, const VoltageBounds &new_voltage_bounds) {
  adc_bounds_ = new_adc_bounds;
  voltage_bounds_ = new_voltage_bounds;
}


VoltageAdcRangeMap::VoltageAdcRangeMap(){
  //nothing
}


VoltageAdcRangeMap::VoltageAdcRangeMap(const VoltageAdcRangeMap &map_to_copy){
  adc_bounds_ = map_to_copy.adc_bounds_;
  voltage_bounds_ = map_to_copy.voltage_bounds_;
}


ReturnState VoltageAdcRangeMap::GetVoltageByAdc(const AdcValue input_adc, Voltage* result_voltage){
  
  AdcValue adc_bounds_diff = adc_bounds_.second - adc_bounds_.first;
  Voltage voltage_bounds_diff = voltage_bounds_.second - voltage_bounds_.first;
  
  if((adc_bounds_diff == 0) || (voltage_bounds_diff == 0))
    return kError;
  
  double coeff = (double) (input_adc - adc_bounds_.first) / adc_bounds_diff;
  *result_voltage = voltage_bounds_.first + coeff * voltage_bounds_diff;
  
  if(*result_voltage < std::min(voltage_bounds_.first, voltage_bounds_.second)) 
    return kOutOfRange;
  if(*result_voltage > std::max(voltage_bounds_.first, voltage_bounds_.second))
    return kOutOfRange;
  
  return kOk;
}

// ===============================================================================================//
/*            I VOLTMETER CHANNEL                                                                  */
//===============================================================================================//
 
  
IVoltmeterChannel::IVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                                      const stm32adc::AdcHardwareNumber adc_number, 
                                      const stm32adc::AdcChannel channel) {
  voltage_adc_range_map_ = new_voltage_adc_map;
  adc_number_ = adc_number;
  channel_ = channel;
}


IVoltmeterChannel::~IVoltmeterChannel(){
  //nothing
}

ReturnState IVoltmeterChannel::TakeMeasurement(AdcValue *measurement){
  
  if( stm32adc::GetCurrentValue(adc_number_, channel_, measurement) != stm32adc::kOk )
      return kError;
  
  return kOk;
}

void IVoltmeterChannel::DumpValues(){
  stm32uart::SendMessage(stm32uart::kUart1, "Nothing to dump");
}
// ===============================================================================================//
/*            I FREE RTOS TIMER USAGE                                                          */
//===============================================================================================//

std::map<TimerHandle_t, IVoltmeterChannel*> IFreeRtosTimerUsage::timers_map_ = {};


IFreeRtosTimerUsage::IFreeRtosTimerUsage(){
  timer_ = nullptr;
}


IFreeRtosTimerUsage::~IFreeRtosTimerUsage(){
  ClearFreeRtosTimer();
}

void IFreeRtosTimerUsage::SetTimerHandle( const TimerHandle_t new_timer ){
  timer_ = new_timer;
}


void IFreeRtosTimerUsage::FreeRtosTimerCallback( TimerHandle_t timer ){
  
  auto timer_it = timers_map_.find(timer);
  
  if(timer_it == timers_map_.end()){
    return;
  }
  
  IVoltmeterChannel* channel = timer_it->second;
  
  AdcValue new_measurement;
  if(channel->TakeMeasurement(&new_measurement) != kOk){
    return;
  }
  
  if(channel->DropMeasurement(new_measurement) != kOk){
  }
  
}


void IFreeRtosTimerUsage::ClearFreeRtosTimer(){
  
  if( timers_map_.find(timer_) != timers_map_.end())
    timers_map_.erase(timer_);
  
  if(timer_ != nullptr)
    xTimerDelete(timer_, portMAX_DELAY);  
  
  timer_ = nullptr;
}

// ===============================================================================================//
/*            INSTANT VOLTMETER CHANNEL                                                          */
//===============================================================================================//

InstantVoltmeterChannel::InstantVoltmeterChannel(const VoltageAdcRangeMap &volt_adc_map,
                                                 const stm32adc::AdcHardwareNumber adc_number, 
                                                 const stm32adc::AdcChannel channel) : IVoltmeterChannel(volt_adc_map, adc_number, channel) {
  //nothing
}


InstantVoltmeterChannel::~InstantVoltmeterChannel(){
  //nothing
}


ReturnState InstantVoltmeterChannel::GetValue(std::string *value){
  
  Voltage current_measurement;
  AdcValue adc_value;
  
  if( TakeMeasurement(&adc_value) != kOk )
      return kError;
  
  voltage_adc_range_map_.GetVoltageByAdc(adc_value, &current_measurement);
  
  *value = std::to_string(current_measurement);
  return kOk;
}


ReturnState InstantVoltmeterChannel::DropMeasurement(const AdcValue new_measurement){
  return kError;
}

// ===============================================================================================//
/*            RMS VOLTMETER CHANNEL                                                               */
//===============================================================================================//

RMSVoltmeterChannel::RMSVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                                          const stm32adc::AdcHardwareNumber adc_number, 
                                          const stm32adc::AdcChannel channel, 
                                          const int measurements_amount,
                                          const TimeMs measurements_period ) : IVoltmeterChannel(new_voltage_adc_map, adc_number, channel) {
  measurements_ = {};
  required_measurements_amount_ = measurements_amount;
  measurements_period_ = measurements_period;
  timer_ = xTimerCreate (  "",
                           measurements_period_,
                           pdTRUE,
                           (void *) 0,
                           FreeRtosTimerCallback );
  
  IFreeRtosTimerUsage::timers_map_.emplace(timer_, static_cast<IVoltmeterChannel*> (this));
  xTimerStart(timer_, portMAX_DELAY);
}


RMSVoltmeterChannel::~RMSVoltmeterChannel(){
  //nothing to do
}


ReturnState RMSVoltmeterChannel::GetValue(std::string *value){
  value->clear();
  
  xTimerStop(timer_, portMAX_DELAY);
  
  if(measurements_.size() < required_measurements_amount_){
    xTimerStart(timer_, portMAX_DELAY);
    return kNotEnoughMeasurements;
  }
    
 int max_val = 0;
  int min_val = stm32adc::kMaxAdcValue;
  
  for(auto measurement_it : measurements_){
    if(measurement_it > max_val)
      max_val = measurement_it;
    if(measurement_it < min_val)
      min_val = measurement_it;    
  }
  
  int mid_val = (max_val + min_val) / 2;
  int rms_val = (int) ((max_val - mid_val) * 0.7071);
  Voltage result = 0;
  
  if(voltage_adc_range_map_.GetVoltageByAdc(rms_val, &result) != kOk)
      return kError;

  *value = std::to_string(result);
  
  xTimerReset(timer_, portMAX_DELAY);
  
  return kOk;  
}


ReturnState RMSVoltmeterChannel::DropMeasurement(const AdcValue new_measurement){
  
  while(measurements_.size() >= required_measurements_amount_)
    measurements_.pop_front();
  
  measurements_.push_back(new_measurement);
  
  return kOk;  
}

void RMSVoltmeterChannel::DumpValues(){
  int i = 0;
  stm32uart::SendMessage(stm32uart::kUart1, "Ch" + std::to_string(channel_) + "dump:");
  Voltage current_measurement = 0;
  
  xTimerStop(timer_, portMAX_DELAY);
  
  for(auto it : measurements_){
    voltage_adc_range_map_.GetVoltageByAdc(it, &current_measurement);
    stm32uart::SendMessage(stm32uart::kUart1, "[" + std::to_string(i) + "] = " + std::to_string(it)); 
    i++;
  }
  
  xTimerReset(timer_, portMAX_DELAY);
}

// ===============================================================================================//
/*            AVERAGE VOLTMETER CHANNEL                                                           */
//===============================================================================================//

AverageVoltmeterChannel::AverageVoltmeterChannel( const VoltageAdcRangeMap &new_voltage_adc_map, 
                                                  const stm32adc::AdcHardwareNumber adc_number, 
                                                  const stm32adc::AdcChannel channel, 
                                                  const int measurements_amount,
                                                  const TimeMs measurements_period ) : IVoltmeterChannel(new_voltage_adc_map, adc_number, channel) {
  required_measurements_amount_ = measurements_amount;
  measurements_period_ = measurements_period;
  timer_ = xTimerCreate (  "",
                           measurements_period_,
                           pdTRUE,
                           (void *) 0,
                           FreeRtosTimerCallback );
  
  IFreeRtosTimerUsage::timers_map_.emplace(timer_, static_cast<IVoltmeterChannel*> (this));
  xTimerStart(timer_, portMAX_DELAY);
}


AverageVoltmeterChannel::~AverageVoltmeterChannel(){
  //nothing
}


ReturnState AverageVoltmeterChannel::GetValue(std::string *value){
  value->clear();
  
  xTimerStop(timer_, portMAX_DELAY);

  //DumpMeasurements();

  if(measurements_.size() < required_measurements_amount_){
    xTimerStart(timer_, portMAX_DELAY);
    return kNotEnoughMeasurements;
  }
  
  int max_val = 0;
  int min_val = stm32adc::kMaxAdcValue;
  
  for(auto measurement_it : measurements_){
    if(measurement_it > max_val)
      max_val = measurement_it;
    if(measurement_it < min_val)
      min_val = measurement_it;    
  }
  
  int mid_val = (max_val + min_val) / 2;
  Voltage result = 0;
  
  if(voltage_adc_range_map_.GetVoltageByAdc(mid_val, &result) != kOk)
      return kError;

  *value = std::to_string(result);
  
  xTimerReset(timer_, portMAX_DELAY);
  
  return kOk;    
}


ReturnState AverageVoltmeterChannel::DropMeasurement(const AdcValue new_measurement){
  while(measurements_.size() >= required_measurements_amount_)
    measurements_.pop_front();
  
  measurements_.push_back(new_measurement);
  
  return kOk;    
}


void AverageVoltmeterChannel::DumpValues(){
  int i = 0;
  stm32uart::SendMessage(stm32uart::kUart1, "Ch" + std::to_string(channel_) + "dump:");
  Voltage current_measurement = 0;
  
  xTimerStop(timer_, portMAX_DELAY);
  
  for(auto it : measurements_){
    voltage_adc_range_map_.GetVoltageByAdc(it, &current_measurement);
    stm32uart::SendMessage(stm32uart::kUart1, "[" + std::to_string(i) + "] = " + std::to_string(it)); 
    i++;
  }
  
  xTimerReset(timer_, portMAX_DELAY);
}
  
  
}               //namespace voltmeter