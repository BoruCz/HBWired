//*******************************************************************
//
// HBW-LC-Sw-12
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 6 + 6 bistabile Relais über Shiftregister und [TODO: 6 Kanal Strommessung]
// [TODO: Peering mit Zeitschaltfuntion]
// - Active HIGH oder LOW kann konfiguriert werden

// TODO: Test der hardware...
//
// http://loetmeister.de/Elektronik/homematic/index.htm#modules
//
//*******************************************************************
// Changes
// v0.1
// - initial version


#define HMW_DEVICETYPE 0x93
#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0001
//#define USE_HARDWARE_SERIAL

#define NUM_CHANNELS 12
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x20

#include "FreeRam.h"


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWLinkSwitch.h"
//#include "HBWSwitch.h"

// shift register library
#include "ShiftRegister74HC595.h"

#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // Transmit-Enable
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 3       //SH_CP shift register clock input
  #define shiftRegOne_Latch 4       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  12
  #define shiftRegTwo_Clock 8
  #define shiftRegTwo_Latch 9
#else
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3  // Transmit-Enable
  // 6 realys and LED attached to 3 shiftregisters
  #define shiftRegOne_Data  10       //DS serial data input
  #define shiftRegOne_Clock 9       //SH_CP shift register clock input
  #define shiftRegOne_Latch 11       //ST_CP storage register clock input
  // extension shifregister for another 6 relays and LEDs
  #define shiftRegTwo_Data  5
  #define shiftRegTwo_Clock 7
  #define shiftRegTwo_Latch 6
  
  #include "HBWSoftwareSerial.h"
  // HBWSoftwareSerial can only do 19200 baud
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif



// Pins
#ifdef USE_HARDWARE_SERIAL
  #define BUTTON A6  // Button fuer Factory-Reset etc.
#else
  #define BUTTON 8  // Button fuer Factory-Reset etc.
#endif

#define LED LED_BUILTIN        // Signal-LED

#define RELAY_PULSE_DUARTION 80  // HIG duration in ms, to set or reset double coil latching relay

#define JT_ONDELAY 0
#define JT_ON 1
#define JT_OFFDELAY 2
#define JT_OFF 3
#define JT_NO_JUMP_IGNORE_COMMAND 4


struct hbw_config_switch {
  uint8_t logging:1;              // 0x0000
  uint8_t output_unlocked:1;      // 0x07:1    0=LOCKED, 1=UNLOCKED
  uint8_t n_inverted:1;           // 0x07:2    0=inverted, 1=not inverted (device reset will set to 1!)
  uint8_t        :5;              // 0x0000
  uint8_t dummy;
};

struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_switch switchcfg[NUM_CHANNELS]; // 0x07-0x... ?
} hbwconfig;



class HBWChanSw : public HBWChannel {
  public:
    //HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, hbw_config_switch* _config);
    HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, ShiftRegister74HC595* _shiftRegister, hbw_config_switch* _config);
    virtual uint8_t get(uint8_t* data);
    virtual void loop(HBWDevice*, uint8_t channel);
    virtual void set(HBWDevice*, uint8_t length, uint8_t const * const data);
    void setOutput(uint8_t const * const data);
    void initRelays();
    uint8_t getNextState(uint8_t bitshift);
  private:
    uint8_t relayPos; // bit position for actual IO port
    uint8_t ledPos;
    ShiftRegister74HC595* shiftRegister;  // allow function calls to the correct shift register
    hbw_config_switch* config; // logging
    uint32_t lastFeedbackTime;  // when did we send the last feedback?
    uint16_t nextFeedbackDelay; // 0 -> no feedback pending
    
//    bool portStatus;	// Port Status, d.h. Port ist auf 0 oder 1
    bool operateRelay;
    unsigned long relayOperationTimeStart;
    
    // set from links/peering
    uint8_t actiontype;
    uint8_t onDelayTime;
    uint8_t offDelayTime;
    uint8_t onTime;
    uint8_t offTime;
    uint8_t currentState;
    uint8_t nextState; // needed?
    uint8_t newLevel;
    uint16_t jumpTable;
    unsigned long stateCangeWaitTime;
    unsigned long lastStateChangeTime;
    //bool changeState;
};


HBWChanSw* switches[NUM_CHANNELS];


class HBSwDevice : public HBWDevice {
    public: 
    HBSwDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
            Stream* _rs485, uint8_t _txen, 
            uint8_t _configSize, void* _config, 
        uint8_t _numChannels, HBWChannel** _channels,
        Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
          HBWDevice(_devicetype, _hardware_version, _firmware_version,
            _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
            _debugstream, linksender, linkreceiver) {
      // looks like virtual methods are not properly called here
      afterReadConfig();
    };

    void afterReadConfig() {
        // defaults setzen
        if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
        for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++){
            switches[channel]->initRelays();
        };
    };
};


HBSwDevice* device = NULL;

//ShiftRegister74HC595 myShReg_LED(2, shiftRegLED_Data, shiftRegLED_Clock, shiftRegLED_Latch);
//ShiftRegister74HC595 myShReg_RELAY(4, shiftRegRELAY_Data, shiftRegRELAY_Clock, shiftRegRELAY_Latch);

ShiftRegister74HC595 myShReg_one(3, shiftRegOne_Data, shiftRegOne_Clock, shiftRegOne_Latch);
ShiftRegister74HC595 myShReg_two(3, shiftRegTwo_Data, shiftRegTwo_Clock, shiftRegTwo_Latch);


HBWChanSw::HBWChanSw(uint8_t _relayPos, uint8_t _ledPos, ShiftRegister74HC595* _shiftRegister, hbw_config_switch* _config) {
  relayPos = _relayPos;
  ledPos =_ledPos;
  config = _config;
  shiftRegister = _shiftRegister;
  nextFeedbackDelay = 0;
  lastFeedbackTime = 0;
  
  relayOperationTimeStart = 0;
  operateRelay = false;
  //changeState = false;
};


void HBWChanSw::initRelays() {    //need intial reset (or set if inverterted) for all relays - bistable relays may have incorrect state!!!

  if (config->n_inverted) { // off - perform reset
    shiftRegister->set(relayPos, LOW);      // set coil
    shiftRegister->set(relayPos +1, HIGH);  // reset coil
    shiftRegister->set(ledPos, LOW); // LED
  }
  else {  // on - perform set
    shiftRegister->set(relayPos +1, LOW);    // reset coil
    shiftRegister->set(relayPos, HIGH);  // set coil
    shiftRegister->set(ledPos, HIGH); // LED
  }
  //TODO: add sleep? setting 12 relays at once would consume high current...

  currentState = JT_OFF;
  nextState = currentState;
  newLevel = 0;

  relayOperationTimeStart = millis();  // Relay coils must be set two low after some ms (bistable Relays!!)
  operateRelay = true;
}


void HBWChanSw::set(HBWDevice* device, uint8_t length, uint8_t const * const data) {
  
//  hbwdebug(F("cs: "));
//  hbwdebughex(currentState);
//  hbwdebug(F("\n"));
  
  if (length > 1) {  // got called with peering parameters
    actiontype = *(data);
    
    if ((actiontype & B00110000) == 32) { // TOGGLE_USE, DIRECT
      byte level = 255;
      setOutput(&level);
    }
    else {  // actual state change happens in main loop
      jumpTable = ((uint16_t)(*(data+6)) << 8) | *(data+5);
      onDelayTime = *(data+1);
      onTime = *(data+2);
      offDelayTime = *(data+3);
      offTime = *(data+4);
      nextState = 255; // force update
    }
  }
  else {
    setOutput(data);
    nextState = currentState; // avoid state machine to run - TODO: handle on_time / off_time??
  }
  
};


void HBWChanSw::setOutput(uint8_t const * const data) {
  
  unsigned long now = millis();
  
  if (config->output_unlocked) {  //0=LOCKED, 1=UNLOCKED
    byte level = *(data);

    if (level > 200) // toggle
        level = !shiftRegister->get(ledPos); // get current state and negate
        
    else if (level)   // set to 0 or 1
      level = (LOW ^ config->n_inverted);
      
    else
      level = (HIGH ^ config->n_inverted);
// TODO:  zero crossing function?. Just set portStatus[]? + add portStatusDesired[]?

    if (level) { // on - perform set
      shiftRegister->set(relayPos +1, LOW);    // reset coil
      shiftRegister->set(relayPos, HIGH);  // set coil
      currentState = JT_ON;
    }
    else {  // off - perform reset
      shiftRegister->set(relayPos, LOW);      // set coil
      shiftRegister->set(relayPos +1, HIGH);  // reset coil
      currentState = JT_OFF;
    }
    shiftRegister->set(ledPos, level); // set LEDs (register used for actual state!)
    
    relayOperationTimeStart = now;  // Relay coils must be set two low after some ms (bistable Relays!!)
    operateRelay = true;
  }
  // Logging
  // (logging is considered for locked channels)
  if(!nextFeedbackDelay && config->logging) {
//    lastFeedbackTime = millis();
    lastFeedbackTime = now;
    nextFeedbackDelay = device->getLoggingTime() * 100;
  }
};


uint8_t HBWChanSw::get(uint8_t* data) {
  
  //if (portStatus ^ config->inverted)
  if (shiftRegister->get(ledPos) ^ config->n_inverted)
    (*data) = 0;
  else
    (*data) = 200;
  return 1;
};


uint8_t HBWChanSw::getNextState(uint8_t bitshift) {
  return ((jumpTable >>bitshift) & B00000111);
}

void HBWChanSw::loop(HBWDevice* device, uint8_t channel) {
  
  unsigned long now = millis();

  /* important to remove power from latching relay after some milliseconds!! */
  if (((now - relayOperationTimeStart) >= RELAY_PULSE_DUARTION) && operateRelay == true) {  // time to remove power from both coils?

    shiftRegister->setNoUpdate(relayPos +1, LOW);    // reset coil
    shiftRegister->setNoUpdate(relayPos, LOW);  // set coil
    shiftRegister->updateRegisters();
    
    operateRelay = false;
  }

  
 bool setNewLevel = false;

  // prep timer?
  // TODO: add counter to avoid infinitive loops??
  
// state machine
  if ((now - lastStateChangeTime > stateCangeWaitTime && (currentState == JT_ONDELAY || currentState == JT_OFFDELAY)) || currentState != nextState) {
  
  //nextState = JT_NO_JUMP_IGNORE_COMMAND; // default?

  hbwdebug(F("cs: "));
  hbwdebughex(currentState);
  hbwdebug(F("\n"));
  
  switch (currentState) { // check next jump from current state
    case JT_ONDELAY:
    // jump from on delay state
      //nextState = ((jumpTable) & B00000111);
      nextState = getNextState(0);
      break;
    case JT_ON:
    // jump from on state
      //nextState = ((jumpTable >>3) & B00000111);
      nextState = getNextState(3);
      break;
    case JT_OFFDELAY:
    // jump from off delay state
      //nextState = ((jumpTable >>6) & B00000111);
      nextState = getNextState(6);
      break;
    case JT_OFF:
    // jump from off state
      //nextState = ((jumpTable >>9) & B00000111);
      nextState = getNextState(9);
      break;
    }

  hbwdebug(F("ns: "));
  hbwdebughex(nextState);
  hbwdebug(F("\n"));
 
  if (nextState != JT_NO_JUMP_IGNORE_COMMAND) {
    switch (nextState) {
    case JT_ONDELAY:
      //newLevel = 0;  // ?????
      stateCangeWaitTime = (onDelayTime *1000); // TODO: change to dynamic factor? add relative & absolute handling
      lastStateChangeTime = now;
      currentState = JT_ONDELAY;
      break;
    case JT_ON:
      newLevel = 200;
      stateCangeWaitTime = 0; // TODO add onTime
      lastStateChangeTime = now;
      setNewLevel = true;
      break;
    case JT_OFFDELAY:
      //newLevel = 200;  // ?????
      stateCangeWaitTime = (offDelayTime *1000); // TODO: change to dynamic factor? add relative & absolute handling
      lastStateChangeTime = now;
      currentState = JT_OFFDELAY;
      break;
    case JT_OFF:
      newLevel = 0;
      stateCangeWaitTime = 0; // TODO add offTime
      lastStateChangeTime = now;
      setNewLevel = true;
      break;
    }
  }
  else {  // NO_JUMP_IGNORE_COMMAND

    //currentState = (level ? JT_ON : JT_OFF );
  }

  if (setNewLevel) {
    setOutput(&newLevel);
    setNewLevel = false;
  }
  }

	
  if(!nextFeedbackDelay)  // feedback trigger set?
    return;
  if (now - lastFeedbackTime < nextFeedbackDelay)
    return;
  lastFeedbackTime = now;  // at least last time of trying
  // sendInfoMessage returns 0 on success, 1 if bus busy, 2 if failed
  // we know that the level has only 1 byte here
  uint8_t level;
  get(&level);
  uint8_t errcode = device->sendInfoMessage(channel, 1, &level);   
  if (errcode == 1)  // bus busy
  // try again later, but insert a small delay
    nextFeedbackDelay = 250;
  else
    nextFeedbackDelay = 0;
}

  
void setup()
{
   // assing switches (relay) pins
//   uint8_t RelayBitPos[NUM_CHANNELS] = {0, 2, 4, 8, 10, 12, 16, 18, 20, 24, 26, 28};
//   uint8_t LEDBitPos[NUM_CHANNELS] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13};
   uint8_t LEDBitPos[6] = {0, 1, 2, 3, 4, 5};    // shift register 1: 6 LEDs // not only used for the LEDs, but also to keep track of the output state!
   uint8_t RelayBitPos[6] = {8, 10, 12,          // shift register 2: 3 relays (with 2 coils each)
                             16, 18, 20};        // shift register 3: 3 relays (with 2 coils each)
  // create channels
  for(uint8_t i = 0; i < NUM_CHANNELS; i++){
    if (i < 6)
      switches[i] = new HBWChanSw(RelayBitPos[i], LEDBitPos[i], &myShReg_one, &(hbwconfig.switchcfg[i]));
    else
      switches[i] = new HBWChanSw(RelayBitPos[i %6], LEDBitPos[i %6], &myShReg_two, &(hbwconfig.switchcfg[i]));
    //switches[i] = new HBWChanSw(BitPos[i], BitPos[i/8], &(hbwconfig.switchcfg[i]));
  };

  #ifdef USE_HARDWARE_SERIAL  // RS485 via UART Serial, no debug (_debugstream is NULL)
    Serial.begin(19200, SERIAL_8E1);
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &Serial, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS,(HBWChannel**)switches,
                           NULL,
                           NULL, new HBWLinkSwitch(NUM_LINKS,LINKADDRESSSTART));
    
    device->setConfigPins(BUTTON, LED, true);  // use analogRead for 'BUTTON'
    
  #else
    Serial.begin(19200);
    rs485.begin();    // RS485 via SoftwareSerial
    
    device = new HBSwDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                           &rs485, RS485_TXEN, sizeof(hbwconfig), &hbwconfig,
                           NUM_CHANNELS, (HBWChannel**)switches,
                           &Serial,
                           NULL, new HBWLinkSwitch(NUM_LINKS,LINKADDRESSSTART));
     
    device->setConfigPins(BUTTON, LED);  // 8 (button) and 13 (led) is the default
   
    hbwdebug(F("B: 2A "));
    hbwdebug(freeRam());
    hbwdebug(F("\n"));
  #endif
}


void loop()
{
  device->loop();
};

