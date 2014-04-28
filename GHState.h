/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */
#ifndef __GHSTATE_H__
#define __GHSTATE_H__

#include <YunServer.h>
#include <FileIO.h>
#include "GHSensor.h"

#define MAX_FILENAME_LEN 64

#define ENABLE_DEBUG_LOG
#ifdef ENABLE_DEBUG_LOG
void _debugLog(const char* logMsg);
#define DEBUG_LOG(log) { _debugLog(log); }
#else
#define DEBUG_LOG(log)
#endif

typedef struct {
    char _logFilename[MAX_FILENAME_LEN];    
    unsigned long _lastUpdate;
    long _hits;

    // Loop state
    bool _readSensorDataNextLoop;
    bool _sendToSensorDataToClient;
    bool _writeSensorDataToFile;
    bool _freshSensorDataAvailable;
    
    YunServer* _server;
    GHSensor* _innerSensor;
    GHSensor* _outerSensor;
} GHState;

GHState* GHState_Create();

void GHState_Step(GHState* self);

#endif