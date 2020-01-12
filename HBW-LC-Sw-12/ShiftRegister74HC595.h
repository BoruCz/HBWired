/*
  ShiftRegister74HC595.h - Library for simplified control of 74HC595 shift registers.
  Created by Timo Denk (www.timodenk.com), Nov 2014.
  Additional information is available at http://shiftregister.simsso.de/
  Released into the public domain.
*/

#ifndef ShiftRegister74HC595_h
#define ShiftRegister74HC595_h

#include "Arduino.h"

class ShiftRegister74HC595 
{
public:
    ShiftRegister74HC595(const uint8_t numberOfShiftRegisters, const uint8_t serialDataPin, const uint8_t clockPin, const uint8_t latchPin);
    void setAll(uint8_t * digitalValues);
    uint8_t * getAll();
    void set(const uint8_t pin, const uint8_t value);
    void setNoUpdate(const uint8_t pin, uint8_t value);
    void updateRegisters();
    void setAllLow();
    void setAllHigh(); 
    uint8_t get(const uint8_t pin);

private:
    uint8_t _numberOfShiftRegisters;
    uint8_t _clockPin;
    uint8_t _serialDataPin;
    uint8_t _latchPin;
    uint8_t * _digitalValues;
};

#endif
