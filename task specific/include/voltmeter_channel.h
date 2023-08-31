#ifndef VOLTMETER_CHANNEL_H
#define VOLTMETER_CHANNEL_H

#include <map>
#include <list>

#include "freeRTOS.h"
#include "timers.h" 

#include "voltmeter.h"

namespace voltmeter{


class VoltageAdcRangeMap{
private:
  AdcBounds adc_bounds_;
  VoltageBounds voltage_bounds_;
public:
  VoltageAdcRangeMap();
  VoltageAdcRangeMap(const AdcBounds &new_adc_bounds, const VoltageBounds &new_voltage_bounds);
  VoltageAdcRangeMap(const VoltageAdcRangeMap &map_to_copy);
  ReturnState GetVoltageByAdc(const AdcValue input_adc, Voltage* result_voltage);
};


class IVoltmeterChannel{
protected:
  VoltageAdcRangeMap voltage_adc_range_map_;
  stm32adc::AdcHardwareNumber adc_number_;
  stm32adc::AdcChannel channel_;
public:
  IVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                    const stm32adc::AdcHardwareNumber adc_number, 
                    const stm32adc::AdcChannel channel);
  virtual ~IVoltmeterChannel();
  virtual ReturnState GetValue(std::string *value) = 0;
  virtual ReturnState DropMeasurement(const AdcValue new_measurement) = 0;
  virtual void DumpValues();
  ReturnState TakeMeasurement(AdcValue *measurement);
};


class IFreeRtosTimerUsage{
protected:
  TimerHandle_t timer_;
  
  static std::map<TimerHandle_t, IVoltmeterChannel*> timers_map_;
  void ClearFreeRtosTimer();  
public:
  IFreeRtosTimerUsage();
  virtual ~IFreeRtosTimerUsage();
  static void FreeRtosTimerCallback( TimerHandle_t timer );
  void SetTimerHandle( const TimerHandle_t new_timer );
  
};


class InstantVoltmeterChannel : public IVoltmeterChannel{
public:
  InstantVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                          const stm32adc::AdcHardwareNumber adc_number, 
                          const stm32adc::AdcChannel channel);
  ~InstantVoltmeterChannel() override;
  ReturnState GetValue(std::string *value) override;
  ReturnState DropMeasurement(const AdcValue new_measurement) override;
};


class RMSVoltmeterChannel : public IVoltmeterChannel, public IFreeRtosTimerUsage{
private:
  std::list<AdcValue> measurements_;
  int required_measurements_amount_;
  TimeMs measurements_period_;
public:
  RMSVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                      const stm32adc::AdcHardwareNumber adc_number, 
                      const stm32adc::AdcChannel channel, 
                      const int measurements_amount,
                      const TimeMs measurements_period);
  ~RMSVoltmeterChannel() override;
  ReturnState GetValue(std::string *value) override;
  ReturnState DropMeasurement(const AdcValue new_measurement) override;
  
  //debug
  void DumpValues() override;
};


class AverageVoltmeterChannel : public IVoltmeterChannel, public IFreeRtosTimerUsage{
private:
  std::list<AdcValue> measurements_;
  int required_measurements_amount_;
  TimeMs measurements_period_;
public:
  AverageVoltmeterChannel(const VoltageAdcRangeMap &new_voltage_adc_map, 
                          const stm32adc::AdcHardwareNumber adc_number, 
                          const stm32adc::AdcChannel channel, 
                          const int measurements_amount,
                          const TimeMs measurements_period);
  ~AverageVoltmeterChannel() override;
  ReturnState GetValue(std::string *value) override;  
  ReturnState DropMeasurement(const AdcValue new_measurement) override;
  
  //debug
  void DumpValues() override;
};


}               //namespace voltmeter


#endif                  //VOLTMETER_CHANNEL_H