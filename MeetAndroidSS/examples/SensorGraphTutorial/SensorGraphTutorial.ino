/*
  Sends sensor data to Arduino
  (needs SensorGraph and Amarino app installed and running on Android)
*/
 
#include <SoftwareSerial.h>
#include <MeetAndroidSS.h>

MeetAndroidSS meetAndroid(9600, 1, 0);
int sensor = 5;

void setup()  
{
  // we initialize analog pin 5 as an input pin
  pinMode(sensor, INPUT);
}

void loop()
{
  meetAndroid.receive(); // you need to keep this in your loop() to receive events
  
  // read input pin and send result to Android
  meetAndroid.send(analogRead(sensor));
  
  // add a little delay otherwise the phone is pretty busy
  delay(100);
}


