/*
TM16XX.cpp - Library implementation for TM16XX.

Copyright (C) 2011 Ricardo Batista (rjbatista <at> gmail <dot> com)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "TM16XX.h"
#include "string.h"

#if defined(ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define DELAY_BETWEEN_SEND
#endif

#define MAXIMUM_INTENSITY ((byte)7)

TM16XX::TM16XX(byte dataPin, byte clockPin, byte strobePin, byte displays, boolean activateDisplay,
	byte intensity)
{
  this->dataPin = dataPin;
  this->clockPin = clockPin;
  this->strobePin = strobePin;
  this->displays = displays;

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(strobePin, OUTPUT);

  digitalWrite(strobePin, HIGH);
  digitalWrite(clockPin, HIGH);

  sendCommand(0x40);
  sendCommand(0x80 | (activateDisplay ? 8 : 0) | min(MAXIMUM_INTENSITY, intensity));

  writeDigitalDataToPin(strobePin, LOW);
  send(0xC0);
  for (int i = 0; i < 16; i++) {
    send(0x00);
  }
  digitalWrite(strobePin, HIGH);
}

void TM16XX::setupDisplay(boolean active, byte intensity)
{
  sendCommand(0x80 | (active ? 8 : 0) | min(MAXIMUM_INTENSITY, intensity));

  // necessary for the TM1640
  digitalWrite(strobePin, LOW);
  writeDigitalDataToPin(clockPin, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::setDisplayDigit(byte digit, byte pos, boolean dot, const byte numberFont[])
{
  sendChar(pos, numberFont[digit & 0xF], dot);
}

void TM16XX::setDisplayToError()
{
    setDisplay(ERROR_DATA, 8);

	for (int i = 8; i < displays; i++) {
	    clearDisplayDigit(i, 0);
	}
}

void TM16XX::clearDisplayDigit(byte pos, boolean dot)
{
  sendChar(pos, 0, dot);
}

void TM16XX::setDisplay(const byte values[], unsigned int size)
{
  for (int i = 0; i < size; i++) {
    sendChar(i, values[i], 0);
  }
}

void TM16XX::clearDisplay()
{
  for (int i = 0; i < displays; i++) {
    sendData(i << 1, 0);
  }
}

void TM16XX::setDisplayToString(const char* string, const word dots, const byte pos, const byte font[])
{
  for (int i = 0; i < displays - pos; i++) {
  	if (string[i] != '\0') {
	  sendChar(i + pos, font[string[i] - 32], (dots & (1 << (displays - i - 1))) != 0);
	} else {
	  break;
	}
  }
}

void TM16XX::setDisplayToString(const String string, const word dots, const byte pos, const byte font[])
{
  int stringLength = string.length();

  for (int i = 0; i < displays - pos; i++) {
    if (i < stringLength) {
      sendChar(i + pos, font[string.charAt(i) - 32], (dots & (1 << (displays - i - 1))) != 0);
    } else {
      break;
    }
  }
}

void TM16XX::sendCommand(byte cmd)
{
  writeDigitalDataToPin(strobePin, LOW);
  send(cmd);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::sendData(byte address, byte data)
{
  sendCommand(0x44);
  digitalWrite(strobePin, LOW);
  send(0xC0 | address);
  send(data);
  digitalWrite(strobePin, HIGH);
}

void TM16XX::send(byte data)
{
  for (int i = 0; i < 8; i++) {
    writeDigitalDataToPin(clockPin, LOW);
    writeDigitalDataToPin(dataPin, data & 1 ? HIGH : LOW);
    data >>= 1;
    writeDigitalDataToPin(clockPin, HIGH);
  }
}

void TM16XX::writeDigitalDataToPin(byte pin, byte data)
{
	digitalWrite(pin, data);
	#ifdef DELAY_BETWEEN_SEND
		delayMicroseconds(2);
	#endif
}

byte TM16XX::receive()
{
  byte temp = 0;

  // Pull-up on
  pinMode(dataPin, INPUT);
  digitalWrite(dataPin, HIGH);

  for (int i = 0; i < 8; i++) {
    temp >>= 1;

    writeDigitalDataToPin(clockPin, LOW);

    if (digitalRead(dataPin)) {
      temp |= 0x80;
    }

    digitalWrite(clockPin, HIGH);
  }

  // Pull-up off
  pinMode(dataPin, OUTPUT);
  writeDigitalDataToPin(dataPin, LOW);

  return temp;
}

#if !defined(ARDUINO) || ARDUINO < 100
// empty implementation instead of pure virtual for older Arduino IDE
void TM16XX::sendChar(byte pos, byte data, boolean dot) {}
#endif