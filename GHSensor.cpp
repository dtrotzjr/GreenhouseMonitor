/*
 * Greenhouse Sensor Class
 * Retrieves sensor data and writes it to a file
 * David Trotz
 * 04/27/2014
 */

#include "GHSensor.h"
#include "GHState.h"

void GHSensor_Init(GHSensor* self, int powerPin, int dataPin, const char* name) {
    self->powerPin = powerPin;
    pinMode(self->powerPin,OUTPUT);
    digitalWrite(self->powerPin, LOW);    
    self->sensor.attach(dataPin);
    String tmp = String(name);
    int nameLen = tmp.length() + 1;
    self->name = (char*)calloc(nameLen, sizeof(char));
    tmp.toCharArray(self->name, nameLen);
    self->humidity = 0.0f;
    self->temperature = 0.0f;
    DEBUG_LOG("GHSensor_Create")
}

float GHSensor_GetTemperature(GHSensor* self){
    return self->temperature;
}

float GHSensor_GetHumidity(GHSensor* self) {
    return self->humidity;
}

const char* GHSensor_GetName(GHSensor* self) {
    return self->name;
}	

void GHSensor_BeginSampling(GHSensor* self) {
    self->humidityTotal = 0.0f;
    self->temperatureTotal = 0.0f;
    self->totalSamples = 0;
}

void GHSensor_SampleSensor(GHSensor* self) {
    // Power on the sensor and take a reading.
    digitalWrite(self->powerPin, HIGH);
    // The sensor needs 2s to initialize
    delay(2000);
    if (self->sensor.read() == 0) {
        self->humidityTotal += (float)self->sensor.humidity;
        self->temperatureTotal += self->sensor.fahrenheit();
    }
    digitalWrite(self->powerPin, LOW);  
    self->totalSamples++;
}

void GHSensor_EndSampling(GHSensor* self) {
    self->humidity = (self->humidityTotal / (float)self->totalSamples);
    self->temperature = (self->temperatureTotal / (float)self->totalSamples);
}
