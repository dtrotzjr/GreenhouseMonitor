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
    char logFilename[MAX_FILENAME_LEN];    
    unsigned long lastUpdate;
    unsigned long lastImageTaken;
    long hits;
    String lastImageName;
    
    YunServer server;
    GHSensor innerSensor;
    GHSensor outerSensor;
} GHState;

void GHState_Init(GHState* self);

void GHState_Step(GHState* self);

#endif
