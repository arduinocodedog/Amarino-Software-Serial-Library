/*
  MeetAndroidSS.cpp - Arduino Library for Amarino
  Copyright (c) 2009 Bonifaz Kaufmann.  All right reserved.
  
  Changes to support SoftwareSerial Library by Jeff French.

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

// Includes
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include <SoftwareSerial.h>
#include "MeetAndroidSS.h"

extern "C" {
#include <stdlib.h>
}


// Private methods
void MeetAndroidSS::processCommand(){
	if(buffer[0]-FunctionBufferOffset < FunctionBufferLenght){
		void (*H_FuncPtr)(uint8_t, uint8_t) = intFunc[buffer[0]-FunctionBufferOffset];
		if (H_FuncPtr != 0) {
			H_FuncPtr(buffer[0], getArrayLength());
		}
		else {
			send("Flag not registered: ");
			send(buffer[0]);
		}
	}
	else {
		if (customErrorFunc)
			errorFunc(buffer[0], getArrayLength());
		else {
			send("Flag out of bounds: ");
			send(buffer[0]);
		}
	}
}


void MeetAndroidSS::init()
{
	waitTime = 30;
	startFlag = 18;
	ack = 19;
	abord = 27;
	delimiter = 59; //';'

	numberOfValues = 0;
	
	for(int a = 0;a < FunctionBufferLenght;a++){
		intFunc[a] = errorFunc;
	}
    
    flush();
}


// public methods
MeetAndroidSS::MeetAndroidSS(uint16_t baudrate, uint8_t rxpin, uint8_t txpin)
{
    SSerial = new SoftwareSerial(rxpin, txpin);
    SSerial->begin(baudrate);
    
    // it is hard to use member function pointer together with normal function pointers.
    customErrorFunc = false;
	errorFunc = 0;
	init();
}

// Constructur for use with HardwareSerial library
MeetAndroidSS::MeetAndroidSS(H_voidFuncPtr err, uint16_t baudrate, uint8_t rxpin, uint8_t txpin)
{
    SSerial = new SoftwareSerial(rxpin, txpin);
    SSerial->begin(baudrate);
    
    customErrorFunc = true;
	errorFunc = err;
	init();
}

void MeetAndroidSS::registerFunction(void(*userfunction)(uint8_t, uint8_t),uint8_t command){
	intFunc[command-FunctionBufferOffset] = userfunction;
}
void MeetAndroidSS::unregisterFunction(uint8_t command){
	intFunc[command-FunctionBufferOffset] = errorFunc;
}

bool MeetAndroidSS::receive(){
	uint8_t lastByte;
	boolean timeout = false;
	while(!timeout)
	{
		while(SSerial->available() > 0)
		{
			lastByte = SSerial->read();
			
			if(lastByte == abord){
				flush();
			}
			else if(lastByte == ack){
				processCommand();
				flush();
			}
			else if(bufferCount < ByteBufferLenght){
				buffer[bufferCount] = lastByte;
				bufferCount++;
			}
            else return false;
		}
		
		if(SSerial->available() <= 0 && !timeout){
			if(waitTime > 0) delayMicroseconds(waitTime);
			if(SSerial->available() <= 0) timeout = true;
		}
	}
	return timeout;
}

void MeetAndroidSS::getBuffer(uint8_t buf[]){

	for(int a = 0;a < bufferCount;a++){
		buf[a] = buffer[a];
	}
}

void MeetAndroidSS::getString(char string[]){

	for(int a = 1;a < bufferCount;a++){
		string[a-1] = buffer[a];
	}
	string[bufferCount-1] = '\0';
}

int MeetAndroidSS::getInt()
{
	uint8_t b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atoi((char*)b);
}

long MeetAndroidSS::getLong()
{
	uint8_t b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atol((char*)b);
}

float MeetAndroidSS::getFloat()
{
	return (float)getDouble();
}

int MeetAndroidSS::getArrayLength()
{
	if (bufferCount == 1) return 0; // only a flag and ack was sent, not data attached
	numberOfValues = 1;
	// find the amount of values we got
	for (int a=1; a<bufferCount;a++){
		if (buffer[a]==delimiter) numberOfValues++;
	}
	return numberOfValues;
}

void MeetAndroidSS::getFloatValues(float values[])
{
	int t = 0; // counter for each char based array
	int pos = 0;

	int start = 1; // start of first value
	for (int end=1; end<bufferCount;end++){
		// find end of value
		if (buffer[end]==delimiter) {
			// now we know start and end of a value
			char b[(end-start)+1]; // create container for one value plus '\0'
			t = 0;
			for(int i = start;i < end;i++){
				b[t++] = (char)buffer[i];
			}
			b[t] = '\0';
			values[pos++] = atof(b);
			start = end+1;
		}
	}
	// get the last value
	char b[(bufferCount-start)+1]; // create container for one value plus '\0'
	t = 0;
	for(int i = start;i < bufferCount;i++){
		b[t++] = (char)buffer[i];
	}
	b[t] = '\0';
	values[pos] = atof(b);
}

// not tested yet
void MeetAndroidSS::getDoubleValues(float values[])
{
	getFloatValues(values);
}

// not tested yet
void MeetAndroidSS::getIntValues(int values[])
{
	int t = 0; // counter for each char based array
	int pos = 0;

	int start = 1; // start of first value
	for (int end=1; end<bufferCount;end++){
		// find end of value
		if (buffer[end]==delimiter) {
			// now we know start and end of a value
			char b[(end-start)+1]; // create container for one value plus '\0'
			t = 0;
			for(int i = start;i < end;i++){
				b[t++] = (char)buffer[i];
			}
			b[t] = '\0';
			values[pos++] = atoi(b);
			start = end+1;
		}
	}
	// get the last value
	char b[(bufferCount-start)+1]; // create container for one value plus '\0'
	t = 0;
	for(int i = start;i < bufferCount;i++){
		b[t++] = (char)buffer[i];
	}
	b[t] = '\0';
	values[pos] = atoi(b);
}


double MeetAndroidSS::getDouble()
{
	char b[bufferCount];
	for(int a = 1;a < bufferCount;a++){
		b[a-1] = (char)buffer[a];
	}

	b[bufferCount-1] = '\0';
	return atof(b);
	
}


#if defined(ARDUINO) && ARDUINO >= 100
size_t MeetAndroidSS::write(uint8_t b){
	return SSerial->print(b);
}
#else
void MeetAndroidSS::write(uint8_t b){
	SSerial->print(b);
}
#endif
	


void MeetAndroidSS::send(char c ){
	SSerial->print(startFlag);
	SSerial->print(c);
	SSerial->print(ack);
}

void MeetAndroidSS::send(const char str[]){
	SSerial->print(startFlag);
	SSerial->print(str);
	SSerial->print(ack);
}
void MeetAndroidSS::send(uint8_t n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::send(int n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::send(unsigned int n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::send(long n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::send(unsigned long n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::send(long n, int base){
	SSerial->print(startFlag);
	SSerial->print(n, base);
	SSerial->print(ack);
}
void MeetAndroidSS::send(double n){
	SSerial->print(startFlag);
	SSerial->print(n);
	SSerial->print(ack);
}
void MeetAndroidSS::sendln(void){
	SSerial->print(startFlag);
	SSerial->println();
	SSerial->print(ack);
}

void MeetAndroidSS::flush(){
	for(uint8_t a=0; a < ByteBufferLenght; a++){
		buffer[a] = 0;
	}
	bufferCount = 0;
	numberOfValues = 0;
}
