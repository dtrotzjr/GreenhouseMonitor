/*
Temperature web interface

This example shows how to serve data from an analog input  
via the Arduino YÃºn's built-in webserver using the Bridge library.

The circuit:
* TMP36 temperature sensor on analog pin A1
* SD card attached to SD card slot of the Arduino YÃºn

Prepare your SD card with an empty folder in the SD root 
named "arduino" and a subfolder of that named "www". 
This will ensure that the YÃºn will create a link 
to the SD to the "/mnt/sd" path.

In this sketch folder is a basic webpage and a copy of zepto.js, a 
minimized version of jQuery.  When you upload your sketch, these files
will be placed in the /arduino/www/TemperatureWebPanel folder on your SD card.

You can then go to http://arduino.local/sd/TemperatureWebPanel
to see the output of this sketch.

You can remove the SD card while the Linux and the 
sketch are running but be careful not to remove it while
the system is writing to it.

created  6 July 2013
by Tom Igoe


This example code is in the public domain.
*/
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h> 
#include <dht22.h>
#include <FileIO.h>

#include "GHSensor.h"
#include "GHState.h"
  
const int INITIALIZED_LED_PIN = 13;
GHState* greenhouseStateMachine;  

void setup() { 
    pinMode(INITIALIZED_LED_PIN,OUTPUT);
    digitalWrite(INITIALIZED_LED_PIN, LOW); 
    // Bridge startup  
    Bridge.begin();
    FileSystem.begin();
    DEBUG_LOG("Starting...")
    greenhouseStateMachine = GHState_Create();
    digitalWrite(13, HIGH);
}

void loop() {
    GHState_Step(greenhouseStateMachine);
}


