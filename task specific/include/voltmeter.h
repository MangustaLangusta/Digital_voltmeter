#ifndef VOLTMETER_H
#define VOLTMETER_H

#include <list>
#include <map>

#include "freeRTOS.h"

#include "stm32uart.h"
#include "stm32adc.h"


namespace voltmeter {
  
const int kMaxSimultaneouslyWorkingChannels = 3;
  
typedef enum {
  kOk,
  kError,
  kNotEnoughMeasurements,
  kOutOfRange   }       ReturnState;

typedef enum {
  kNoCommand,
  kStartCommand,
  kStopCommand,
  kResultCommand, 
  kStatusCommand  }     CommandDescriptor;

typedef enum {
  kNoMode,
  kModeInstant,
  kModeAverage,
  kModeRMS      }       ChannelMode;


typedef enum {
  kVoltmeterIdle,
  kVoltmeterMeasuring,
  kVoltmeterError }     VoltmeterState;


typedef std::list<std::string> ParamsList;

typedef float Voltage;
typedef stm32adc::AdcValue AdcValue;
typedef std::pair<AdcValue, AdcValue> AdcBounds;
typedef std::pair<Voltage, Voltage> VoltageBounds;
typedef TickType_t TimeMs;

class VoltageAdcRangeMap;
class IVoltmeterChannel;

typedef std::unique_ptr<IVoltmeterChannel> VoltmeterChannelPtr;
typedef std::weak_ptr<IVoltmeterChannel> VoltmeterChannelWeakPtr;

class Voltmeter{
private:
  static stm32uart::UartHardwareNumber assigned_uart_;
  static stm32adc::AdcHardwareNumber assigned_adc_;
  
  static std::map<stm32adc::AdcChannel, VoltmeterChannelPtr> active_channels_;
  
  static std::list<std::string> errors_list_;
  
  static VoltmeterState state_;
  
  static void ProcessStartCommand(const ParamsList &parsed_message);
  static void ProcessStopCommand(const ParamsList &parsed_message);
  static void ProcessResultCommand(const ParamsList &parsed_message);
  static void ProcessStatusCommand(const ParamsList &parsed_message);
  
  static ReturnState GetChannelValue(const stm32adc::AdcChannel channel, std::string *result_string);
  static void DumpChannelValues(const stm32adc::AdcChannel channel);
  
  Voltmeter() = delete;
  Voltmeter(const Voltmeter&) = delete;
public:
  static void AssignUart(const stm32uart::UartHardwareNumber uart_number);
  static void AssignAdc(const stm32adc::AdcHardwareNumber adc_number);
  static void IncomingMessage(std::string new_message);  
  static VoltmeterState GetState();
  static void UpdateState();
};



}               //namespace voltmeter

#endif          //VOLTMETER_H