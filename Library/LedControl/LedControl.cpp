/*
  LedControl.cpp - A library for controling Led Digits or 
  Led arrays with a MAX7219/MAX7221
  Copyright (c) 2007 Eberhard Fahle

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include "LedControl.h"
#include <avr/pgmspace.h>

//the opcodes the MAX7221 and MAX7219 understand
#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

LedControl::LedControl(int dataPin, int clkPin, int csPin, int numDevices) : _mirror(true), _turn_back(true), _reverse(true)
{
    SPI_MOSI=dataPin;
    SPI_CLK=clkPin;
    SPI_CS=csPin;
    if(numDevices<=0 || numDevices>8 )
	numDevices=8;
    maxDevices=numDevices;
    pinMode(SPI_MOSI,OUTPUT);
    pinMode(SPI_CLK,OUTPUT);
    pinMode(SPI_CS,OUTPUT);
    digitalWrite(SPI_CS,HIGH);
    SPI_MOSI=dataPin;
    for(int i=0;i<64;i++) 
	status[i]=0x00;
    for(int i=0;i<maxDevices;i++) {
	spiTransfer(i,OP_DISPLAYTEST,0);
	//scanlimit is set to max on startup
	setScanLimit(i,7);
	//decode is done in source
	spiTransfer(i,OP_DECODEMODE,0);
	clearDisplay(i);
	//we go into shutdown-mode on startup
	shutdown(i,true);
    }
}

int LedControl::getDeviceCount() {
    return maxDevices;
}

void LedControl::shutdown(int addr, bool b) {
    if(addr<0 || addr>=maxDevices)
	return;
    if(b)
	spiTransfer(addr, OP_SHUTDOWN,0);
    else
	spiTransfer(addr, OP_SHUTDOWN,1);
}
	
void LedControl::setScanLimit(int addr, int limit) {
    if(addr<0 || addr>=maxDevices)
	return;
    if(limit>=0 || limit<8)
    	spiTransfer(addr, OP_SCANLIMIT,limit);
}

void LedControl::setIntensity(int addr, int intensity) {
    if(addr<0 || addr>=maxDevices)
	return;
    if(intensity>=0 || intensity<16)	
	spiTransfer(addr, OP_INTENSITY,intensity);
    
}

void LedControl::clearDisplay(int addr) {
    int offset;

    if(addr<0 || addr>=maxDevices)
	return;
    offset=addr*8;
    for(int i=0;i<8;i++) {
	status[offset+i]=0;
	spiTransfer(addr, i+1,status[offset+i]);
    }
}

void LedControl::setLed(int addr, int row, int column, boolean state) {
    int offset;
    byte val=0x00;

    if(addr<0 || addr>=maxDevices)
	return;
    if(row<0 || row>7 || column<0 || column>7)
	return;
    offset=addr*8;
    val=B10000000 >> column;
    if(state)
	status[offset+row]=status[offset+row]|val;
    else {
	val=~val;
	status[offset+row]=status[offset+row]&val;
    }
    spiTransfer(addr, row+1,status[offset+row]);
}
	
void LedControl::setRow(int addr, int row, byte value) {
    int offset;
    if(addr<0 || addr>=maxDevices)
	return;
    if(row<0 || row>7)
	return;
    offset=addr*8;
    status[offset+row]=value;
    spiTransfer(addr, row+1,status[offset+row]);
}
    
void LedControl::setColumn(int addr, int col, byte value) {
    byte val;

    if(addr<0 || addr>=maxDevices)
	return;
    if(col<0 || col>7) 
	return;
    for(int row=0;row<8;row++) {
	val=value >> (7-row);
	val=val & 0x01;
	setLed(addr,row,col,val);
    }
}

/*
 * Here are the segments to be switched on for characters and digits on
 * 7-Segment Displays
 */
static const byte PROGMEM charTable[128] = {
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B10000000,B00000001,B10000000,B00000000,
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000
};

static byte mirror(byte srcByte)
{
    byte temp = 0;
                //  abcdefg
    //swap a, d
    if (srcByte & B01000000)
        temp |= B00001000;
    else
        temp &= B11110111;
    if (srcByte & B00001000)
        temp |= B01000000;
    else
        temp &= B10111111;
    //swap b,c 
    if (srcByte & B00100000)
        temp |= B00010000;
    else
        temp &= B11101111;
    if (srcByte & B00010000)
        temp |= B00100000;
    else
        temp &= B11011111;
    //swap f, e
    if (srcByte & B00000010)
        temp |= B00000100;
    else
        temp &= B11111011;
    if (srcByte & B00000100)
        temp |= B00000010;
    else
        temp &= B11111101;
    //assign g
    if (srcByte & B00000001)
        temp |= B00000001;
    else
        temp &= B11111110;

    return temp;
}

void printBits(byte myByte){
    for(byte mask = 0x80; mask; mask >>= 1){
        if(mask  & myByte)
            Serial.print('1');
        else
            Serial.print('0');
    }
}

static byte turn_back(byte srcByte)
{
    byte temp;
    int shift = 3;
    int bitSize = 6;
    byte cleanByte = B00000001;
    byte result = B00000000;
    result |= (srcByte & cleanByte);
    for (int i = 1; i < 7; ++i)
    {
        bool setBit = false;
        if (((srcByte >> i) & cleanByte) == 0)
            continue;
        int start = i;
        if (start-1 < shift)
            start += bitSize;

        int _shift = start - shift;
        result |= (B00000001 << _shift);
    }
    return result;
}

void LedControl::setDigit(int addr, int digit, byte value, boolean dp) {
    int offset;
    byte v;

    if(addr<0 || addr>=maxDevices)
	return;
    if(digit<0 || digit>7 || value>15)
	return;
    offset=addr*8;
    v=pgm_read_byte(charTable + value);

    if (_mirror)
        v = mirror(v);
    if (_turn_back)
        v = turn_back(v);

    if(dp)
	v|=B10000000;

    if (_reverse)
        digit = 7 - digit;

    status[offset+digit]=v;
    spiTransfer(addr, digit+1,v);
    
}

void LedControl::setChar(int addr, int digit, char value, boolean dp) {
    int offset;
    byte index,v;

    if(addr<0 || addr>=maxDevices)
	return;
    if(digit<0 || digit>7)
 	return;
    offset=addr*8;
    index=(byte)value;
    if(index >127) {
	//nothing define we use the space char
	value=32;
    }
    v=pgm_read_byte(charTable + index);
    if(dp)
	v|=B10000000;
    status[offset+digit]=v;
    spiTransfer(addr, digit+1,v);
}

void LedControl::spiTransfer(int addr, volatile byte opcode, volatile byte data) {
    //Create an array with the data to shift out
    int offset=addr*2;
    int maxbytes=maxDevices*2;

    for(int i=0;i<maxbytes;i++)
	spidata[i]=(byte)0;
    //put our device data into the array
    spidata[offset+1]=opcode;
    spidata[offset]=data;
    //enable the line 
    digitalWrite(SPI_CS,LOW);
    //Now shift out the data 
    for(int i=maxbytes;i>0;i--)
 	shiftOut(SPI_MOSI,SPI_CLK,MSBFIRST,spidata[i-1]);
    //latch the data onto the display
    digitalWrite(SPI_CS,HIGH);
}    

