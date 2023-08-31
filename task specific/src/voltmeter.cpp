//file voltmeter.cpp
#include "stm32adc.h"
#include "voltmeter.h"
#include "voltmeter_channel.h"
#include "parser.h"

namespace voltmeter{
  
constexpr stm32uart::UartHardwareNumber kDefaultUart = stm32uart::kUart1;
constexpr stm32adc::AdcHardwareNumber kDefaultAdc = stm32adc::kAdc1;
constexpr Voltage kDefaultMinVoltage = 0;
constexpr Voltage kDefaultMaxVoltage = 3.3;
constexpr int kDefaultMeasurementsAmount = 20;
constexpr TimeMs kDefaultMeasurementsPeriod = 2;



stm32uart::UartHardwareNumber Voltmeter::assigned_uart_ = kDefaultUart;
stm32adc::AdcHardwareNumber Voltmeter::assigned_adc_ = kDefaultAdc;
VoltmeterState Voltmeter::state_ = kVoltmeterIdle;
std::map <stm32adc::AdcChannel, VoltmeterChannelPtr> Voltmeter::active_channels_ = {};
std::list<std::string> Voltmeter::errors_list_ = {};


const VoltageAdcRangeMap kDefaultVoltageAdcRangeMap = VoltageAdcRangeMap( {0, stm32adc::kMaxAdcValue}, {kDefaultMinVoltage, kDefaultMaxVoltage} );


static CommandDescriptor CommandDescriptorFromString(const std::string &string){
  if(string == "start")
    return kStartCommand;
  if(string == "stop")
    return kStopCommand;
  if(string == "result")
    return kResultCommand;
  if(string == "status")
    return kStatusCommand;
  return kNoCommand;
}


static stm32adc::AdcChannel AdcChannelFromString(const std::string &string){
  using namespace stm32adc;
  if(string == "ch0")
    return kCh0;
  if(string == "ch1")
    return kCh1;
  if(string == "ch2")
    return kCh2;
  if(string == "ch3")
    return kCh3;
  if(string == "ch4")
    return kCh4;
  if(string == "ch5")
    return kCh5;
  if(string == "ch6")
    return kCh6;
  if(string == "ch7")
    return kCh7;
  if(string == "ch8")
    return kCh8;
  if(string == "ch9")
    return kCh9;
  if(string == "ch10")
    return kCh10;
  if(string == "ch11")
    return kCh11;
  if(string == "ch12")
    return kCh12;
  if(string == "ch13")
    return kCh13;
  if(string == "ch14")
    return kCh14;
  if(string == "ch15")
    return kCh15;
  
  return kNoChannel;
}


static ChannelMode ChannelModeFromString(const std::string &string){
  if(string == "none")
    return kModeInstant;
  if(string == "rms")
    return kModeRMS;
  if(string == "avg")
    return kModeAverage;
  
  return kNoMode;
}

    
void Voltmeter::ProcessStartCommand(const ParamsList &parsed_message){
  ChannelMode new_channel_mode = kNoMode;
  stm32adc::AdcChannel new_channel = stm32adc::kNoChannel;
  
  for(auto it : parsed_message){
    new_channel = AdcChannelFromString(it);
    if(new_channel != stm32adc::kNoChannel)
      break;
  }
  
  for(auto it : parsed_message){
    new_channel_mode = ChannelModeFromString(it);
    if(new_channel_mode != kNoMode)
      break;
  }
  
  if((new_channel == stm32adc::kNoChannel) || (new_channel_mode == kNoMode)){
    stm32uart::SendMessage(assigned_uart_, "wrong parameters of start command");
    return;
  }
  
  if(active_channels_.size() >= kMaxSimultaneouslyWorkingChannels){
    stm32uart::SendMessage(assigned_uart_, "working channels limit reached (limit = " + std::to_string(kMaxSimultaneouslyWorkingChannels) + ")");
    return;
  }
  
  stm32adc::ReturnState add_channel_status = stm32adc::AddChannelToScanList(assigned_adc_, new_channel);
  
  if(add_channel_status == stm32adc::kChannelAlreadyActive){
    stm32uart::SendMessage(assigned_uart_, "ch" + std::to_string(new_channel) + " is already active");
    return;
  }
  
  if(add_channel_status != stm32adc::kOk){
    stm32uart::SendMessage(assigned_uart_, "unable to start ch" + std::to_string(new_channel));
    return;
  }

  VoltmeterChannelPtr channel_instance = nullptr;
    
  switch(new_channel_mode){
  case kModeInstant:
    channel_instance = std::make_unique<InstantVoltmeterChannel>( kDefaultVoltageAdcRangeMap, 
                                                                  assigned_adc_, 
                                                                  new_channel);
    break;
  case kModeRMS:
    channel_instance = std::make_unique<RMSVoltmeterChannel>    ( kDefaultVoltageAdcRangeMap, 
                                                                  assigned_adc_, 
                                                                  new_channel,
                                                                  kDefaultMeasurementsAmount,
                                                                  kDefaultMeasurementsPeriod);
                                                        
    break;
  case kModeAverage:
    channel_instance = std::make_unique<AverageVoltmeterChannel>( kDefaultVoltageAdcRangeMap, 
                                                                  assigned_adc_, 
                                                                  new_channel,
                                                                  kDefaultMeasurementsAmount,
                                                                  kDefaultMeasurementsPeriod);
    break;
  default:
    return;
  }
    
  active_channels_.emplace(new_channel, std::move(channel_instance));
  
  stm32uart::SendMessage(assigned_uart_, "started ch " + std::to_string(new_channel));
}


void Voltmeter::ProcessStopCommand(const ParamsList &parsed_message){
  
  stm32adc::AdcChannel new_channel = stm32adc::kNoChannel;
  for(auto it : parsed_message){
    new_channel = AdcChannelFromString(it);
    if(new_channel != stm32adc::kNoChannel)
      break;
  }
  
  if(new_channel == stm32adc::kNoChannel)
    return;
  
  auto ch_it = active_channels_.find(new_channel);
  if( ch_it != active_channels_.end() ){
    active_channels_.erase(new_channel);
  }
  
  if( stm32adc::RemoveChannelFromScanList(assigned_adc_, new_channel) != stm32adc::kOk )
    return;
  
  stm32uart::SendMessage(assigned_uart_, "ch " + std::to_string(new_channel) + " stopped");
}


void Voltmeter::ProcessResultCommand(const ParamsList &parsed_message){
  
  stm32adc::AdcChannel channel = stm32adc::kNoChannel;
  
  for(auto it : parsed_message){
    channel = AdcChannelFromString(it);
    if(channel != stm32adc::kNoChannel)
      break;
  }
  
  if(channel == stm32adc::kNoChannel){
    stm32uart::SendMessage(assigned_uart_, "wrong parameters of command result");
    return;
  }
  
  //"dump" request handling
  bool dump_requested = false;
  for(auto it : parsed_message){
    if(it == "dump"){
      dump_requested = true;
      break;
    }
  }
  
  std::string value = "";
  const ReturnState get_value_status = GetChannelValue(channel, &value);
  if(get_value_status == kError ){
    stm32uart::SendMessage(assigned_uart_, "Ch" + std::to_string(channel) + " is not running");
    return;
  }
  
  if(get_value_status == kNotEnoughMeasurements ){
    stm32uart::SendMessage(assigned_uart_, "Ch" + std::to_string(channel) + " result is not yet ready");
    return;
  }
  
  stm32uart::SendMessage(assigned_uart_, "ch" + std::to_string(channel) + " value = " + value);
  if(dump_requested)
    DumpChannelValues(channel);
}


void Voltmeter::ProcessStatusCommand(const ParamsList &parsed_message){
  stm32uart::SendMessage(assigned_uart_, "* * *");
  if(active_channels_.empty()){
    stm32uart::SendMessage(assigned_uart_, "Status: idle");
  }
  else{
    std::string channels = "running channels:\n";
    for(auto it = active_channels_.begin(); it != active_channels_.end(); it++){
      channels += "ch" + std::to_string(it->first) + " ";
    }
    stm32uart::SendMessage(assigned_uart_, channels);
  }
     
  if(errors_list_.empty()){
    stm32uart::SendMessage(assigned_uart_, "No errors");
  } 
   
  
  while(!errors_list_.empty()){
    stm32uart::SendMessage(assigned_uart_, errors_list_.front());
    errors_list_.pop_front();
  }
  
  stm32uart::SendMessage(assigned_uart_, "* * *");
}


ReturnState Voltmeter::GetChannelValue(const stm32adc::AdcChannel channel, std::string *result_string){
  result_string->clear();
  
  auto ch_it = active_channels_.find(channel);
  if(ch_it == active_channels_.end())
     return kError;
   
  
  VoltmeterChannelPtr* this_channel = &(ch_it->second);
  
  std::string raw_str = "";
  ReturnState result_state = (*this_channel)->GetValue(&raw_str);
  
  int pt_index = raw_str.find(".");
  
  *result_string = raw_str.substr(0, pt_index + 5);
  
  return result_state; 
}


void Voltmeter::DumpChannelValues(const stm32adc::AdcChannel channel){
  auto ch_it = active_channels_.find(channel);
  if(ch_it == active_channels_.end())
     return;
  
  VoltmeterChannelPtr* this_channel = &(ch_it->second);
  
  (*this_channel)->DumpValues();
}

void Voltmeter::AssignUart(const stm32uart::UartHardwareNumber uart_number){
  assigned_uart_ = uart_number;
}


void Voltmeter::AssignAdc(const stm32adc::AdcHardwareNumber adc_number){
  assigned_adc_ = adc_number;
}

  
void Voltmeter::IncomingMessage(std::string new_message){
  ParamsList parsed_message = {};
  if( !parser::ParseMessage(new_message, " ", &parsed_message) )
    return;
  
  CommandDescriptor descriptor = CommandDescriptorFromString(parsed_message.front());
  
  parsed_message.pop_front();
  
  switch(descriptor){
  case kStartCommand:
    ProcessStartCommand(parsed_message);
    break;
  case kStopCommand:
    ProcessStopCommand(parsed_message);
    break;
  case kResultCommand:
    ProcessResultCommand(parsed_message);
    break;
  case kStatusCommand:
    ProcessStatusCommand(parsed_message);
    break;
  default:
    return;
  }
  return;
}


VoltmeterState Voltmeter::GetState(){
  UpdateState();
  return state_;
}


void Voltmeter::UpdateState(){
  if( !errors_list_.empty() ){
    state_ = kVoltmeterError;
    return;
  }
  
  if( !active_channels_.empty() ){
    state_ = kVoltmeterMeasuring;
    return;
  }
  
  state_ = kVoltmeterIdle;
}

}               //namespace voltmeter