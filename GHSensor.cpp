/*
 * Greenhouse Sensor Class
 * Retrieves sensor data and writes it to a file
 * David Trotz
 * 04/27/2014
 */

#include "GHSensor.h"
#include "GHState.h"

GHSensor* GHSensor_Create(int powerPin, int dataPin, const char* name) {
	GHSensor* self = (GHSensor*)calloc(1, sizeof(GHSensor));
	self->_powerPin = powerPin;
	pinMode(self->_powerPin,OUTPUT);
	digitalWrite(self->_powerPin, LOW);
	self->_sensor.attach(dataPin);
	String tmp = String(name);
	int nameLen = tmp.length() + 1;
	self->_name = (char*)calloc(nameLen, sizeof(char));
	tmp.toCharArray(self->_name, nameLen);
	self->_humidity = 0.0f;
	self->_temperature = 0.0f;
    DEBUG_LOG("GHSensor_Create")
}

float GHSensor_GetTemperature(GHSensor* self){
	return self->_temperature;
}

float GHSensor_GetHumidity(GHSensor* self) {
	return self->_humidity;
}

const char* GHSensor_GetName(GHSensor* self) {
	return self->_name;
}	

void GHSensor_BeginSampling(GHSensor* self) {
	self->_humidityTotal = 0.0;
	self->_temperatureTotal = 0.0;
	self->_totalSamples = 0;
}

void GHSensor_SampleSensor(GHSensor* self) {
    // Power on the sensor and take a reading.
    digitalWrite(self->_powerPin, HIGH);
	// The sensor needs 2s to initialize
    delay(2000);
    if (self->_sensor.read() == 0)
    {
        self->_humidityTotal += (float)self->_sensor.humidity;
        self->_temperatureTotal += self->_sensor.fahrenheit();
    }
    digitalWrite(self->_powerPin, LOW);  
	self->_totalSamples++;
}

void GHSensor_EndSampling(GHSensor* self) {
	self->_humidity = (self->_humidityTotal / (float)self->_totalSamples);
	self->_temperature = (self->_temperatureTotal / (float)self->_totalSamples);
}