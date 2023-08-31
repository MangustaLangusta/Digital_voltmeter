#ifndef STM32_ADC_H
#define STM32_ADC_H

#include "stm32adcConfig.h"


namespace stm32adc {
  
  //=========================  TYPES  ===============================//
  
typedef enum {  kOk, 
                kAdcNotInitialised,
                kChannelsLimitReached,
                kChannelNotActive,
                kChannelAlreadyActive,
                kError   }              ReturnState;

typedef enum {  kAdc1,  
                kAdc3    }              AdcHardwareNumber;

typedef enum {  kCh0, kCh1, 
                kCh2, kCh3, 
                kCh4, kCh5, 
                kCh6, kCh7, 
                kCh8, kCh9, 
                kCh10, kCh11, 
                kCh12, kCh13, 
                kCh14, kCh15,
                kNoChannel,}       AdcChannel;  

typedef unsigned short AdcValue;

struct AdcConfiguration {
  int max_simultaneously_scanned_channels = adc_configDEFAULT_MAX_SIMULTANEOUSLY_SCANNED_CHANNELS;
};

constexpr AdcConfiguration kDefaultAdcConfiguration;
constexpr AdcValue kInvalidValue = 0;
constexpr AdcValue kMaxAdcValue = 4095;



//========================= INTERFACE METHODS ==================//



  //Initialisation of Adc
  //-configuration- can be replaced by default constant kDefaultAdcConfiguration
  //            Possible returns:
  //    kOk                     : initialised successfuly
  //    kError                  : not initialised due to some error
ReturnState InitAdc(const AdcHardwareNumber adc_number, const AdcConfiguration &configuration);


  //Adds -new_channel- to scan list of Adc -adc_number-
  //It means that this channel will be measured regulary
  //and its value is available at any time
  //            Possible returns:
  //    kOk                     : channel successfuly added to scan list
  //    kkAdcNotInitialised     : ch not added due to respective Adc was not initialized
  //    kChannelAlreadyActive   : channel was already added, nothing to do
  //    kChannelsLimitReached   : not added due to channels limit reached
  //    kError                  : non added due to some other error
ReturnState AddChannelToScanList(const AdcHardwareNumber adc_number, const AdcChannel new_channel);


  //Gets value of adc channel -channel- by Adc -adc_number-
  //Value to be stored on -*value- address
  //value is valid only in case of kOk return state
 
  //            Possible returns:
  //    kOk                     : Value is be stored on -*value- address
  //    kkAdcNotInitialised     : value is invalid due to Adc was not initialized
  //    kChannelNotActive       : value is invalid due to channel was not added to scan list
  //    kError                  : other error, value is invalid
ReturnState GetCurrentValue(const AdcHardwareNumber adc_number, const AdcChannel channel, AdcValue *value);


  //Removes adc channel -channel_to_remove- from Adc -adc_number- scan list
 
  //            Possible returns:
  //    kOk                     : Successfuly removed
  //    kkAdcNotInitialised     : error: Adc was not initialized
  //    kError                  : other error
ReturnState RemoveChannelFromScanList(const AdcHardwareNumber adc_number, const AdcChannel channel_to_remove);

  
}               //namespace stm32adc

#endif          //STM32_ADC_H
