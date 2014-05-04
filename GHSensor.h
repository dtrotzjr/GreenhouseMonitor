/*
 * Greenhouse Sensor Class
 * Retrieves sensor data and writes it to a file
 * David Trotz
 * 04/27/2014
 */
#ifndef __GHSENSOR_H__
#define __GHSENSOR_H__

#include <dht22.h>

typedef struct {	
	int powerPin;
	char* name;
	float humidity;
	float temperature;
	
	float humidityTotal;
	float temperatureTotal;
	int totalSamples;
	
	dht22 sensor;
} GHSensor;

void GHSensor_Init(GHSensor* self, int powerPin, int dataPin, const char* name);
float GHSensor_GetTemperature(GHSensor* self);
float GHSensor_GetHumidity(GHSensor* self);
const char* GHSensor_GetName(GHSensor* self);
// Because we need to wait 2 seconds between samples and have more than one
// sensor to sample we shouldn't do all the samples at once so that while
// we are waiting we can sample other sensors. So we will GHSensor_BeginSampling()
// on each sensor and GHSensor_SampleSensor() on each sensor in order, by the time
// we come back to the initial sensor at least 2 seconds has passed and we
// can sample it again. When we are done we GHSensor_EndSampling() and we can
// average the samples for a more accurate reading.
void GHSensor_BeginSampling(GHSensor* self);
void GHSensor_SampleSensor(GHSensor* self);
void GHSensor_EndSampling(GHSensor* self);

#endif
