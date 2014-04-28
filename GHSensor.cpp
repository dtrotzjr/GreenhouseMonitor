/*
 * Greenhouse Sensor Class
 * Retrieves sensor data and writes it to a file
 * David Trotz
 * 04/27/2014
 */

#include "GHSensor.h"

GHSensor::GHSensor(int powerPin, int dataPin, const char* name) {
	_powerPin = powerPin;
	pinMode(_powerPin,OUTPUT);
	digitalWrite(_powerPin, LOW);
	DHT22_GH.attach(dataPin);
	String tmp = String(name);
	int nameLen = tmp->length() + 1;
	_name = calloc(nameLen, sizeof(char));
	tmp->toCharArray(_name, nameLen);
	_humidity = 0.0f;
	_temperature = 0.0f;
}

GHSensor::~GHSensor() {
	free(_name);
}

float GHSensor::GetTemperature(){
	return _temperature;
}

float GHSensor::GetHumidity() {
	return _humidity;
}

void GHSensor::BeginSampling() {
	_humidityTotal = 0.0;
	_temperatureTotal = 0.0;
	_totalSamples 0;
}

void GHSensor::SampleSensor() {
    // Power on the sensor and take a reading.
    digitalWrite(_powerPin, HIGH);
	// The sensor needs 2s to initialize
    delay(2000);
    if (DHT22_GH.read() == 0)
    {
        _humidityTotal += (float)DHT22_GH.humidity;
        _temperatureTotal += DHT22_GH.fahrenheit();
    }
    digitalWrite(_powerPin, LOW);  
	_totalSamples++;
}

void GHSensor::EndSampling() {
	_humidity = (_humidityTotal / (float)_totalSamples);
	_temperature = (_temperatureTotal / (float)_totalSamples);
}