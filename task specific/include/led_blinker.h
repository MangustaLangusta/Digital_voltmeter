#ifndef LED_BLINKER_H
#define LED_BLINKER_H

namespace LedBlinker{
 
typedef enum {
  RETURN_FAIL,
  RETURN_OK
} ReturnStatus;

typedef enum {
  LED_IS_OFF,
  LED_IS_ON
} LedState;

//Singleton
class Led{
private:
  static LedState current_state;
  static int blink_period;
  
  Led()=delete;
  Led(const Led&)=delete;
  
public:
  static ReturnStatus Init();
  static void LedOn();
  static void LedOff();
  static LedState Toggle();
  static void SetPeriod(int new_period);
  static int GetPeriod();
};


  
}       //namespace LedBlinker

#endif  //LED_BLINKER_H
