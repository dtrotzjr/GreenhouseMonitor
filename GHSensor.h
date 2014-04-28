/*
 * Greenhouse Sensor Class
 * Retrieves sensor data and writes it to a file
 * David Trotz
 * 04/27/2014
 */
#ifndef __GHSENSOR_H__
#define __GHSENSOR_H__

#include <dht22.h>

class GHSensor {
public:
	GHSensor(int powerPin, int dataPin, const char* name);
	~GHSensor();
	float GetTemperature();
	float GetHumidity();
	const char* GetName();
	// Because we need to wait 2 seconds between samples and have more than one
	// sensor to sample we shouldn't do all the samples at once so that while
	// we are waiting we can sample other sensors. So we will BeginSampling()
	// on each sensor and SampleSensor() on each sensor in order, by the time
	// we come back to the initial sensor at least 2 seconds has passed and we
	// can sample it again. When we are done we EndSampling() and we can
	// average the samples for a more accurate reading.
	void BeginSampling();
	void SampleSensor();
	void EndSampling();
	
private:
	const int _powerPin;
	char* _name;
	float _humidity;
	float _temperature;
	
	float _humidityTotal;
	float _temperatureTotal;
	int _totalSamples;
	
	dht22 _sensor;
};

#endif