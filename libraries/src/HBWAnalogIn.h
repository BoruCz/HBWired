/*
 * HBWAnalogIn.cpp
 * 
 * analog input channel, max. 16 bit reading
 * 
 * www.loetmeister.de
 * 
 */

#ifndef HBWAnalogIn_h
#define HBWAnalogIn_h

#include <inttypes.h>
#include "HBWired.h"


//#define DEBUG_OUTPUT   // extra debug output on serial/USB

#define SAMPLE_INTERVAL 2  // seconds * 3 samples
#define DEFAULT_UPDATE_INTERVAL 300  // seconds


struct hbw_config_analog_in {
  uint8_t input_disabled:1;     // 0x07:0   1=DISABLED (default), 0=ENABLED
  uint8_t notify_disabled:1;   // 0x07:1   1=DISABLED (default), 0=ENABLED
  uint8_t        :6;      // 0x07:2-7
  uint8_t update_interval;  // factor 10, range 10 to 2540 seconds //TODO: 10 seconds stepping is ok?
};


// Class HBWAnalogIn
class HBWAnalogIn : public HBWChannel {
  public:
    HBWAnalogIn(uint8_t _pin, hbw_config_analog_in* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    // virtual void afterReadConfig();
    
  private:
    uint8_t pin;   // Pin
    hbw_config_analog_in* config;
    uint32_t lastActionTime;
    uint8_t nextActionDelay;
    uint16_t currentValue;  // store last result (average)
};

#endif
