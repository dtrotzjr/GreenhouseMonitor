/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */
#include <YunServer.h>

String startString;
long hits = 0;

GHSensor* innerSensor = NULL;
GHSensor* outerSensor = NULL;
dht22 DHT22_GH;
dht22 DHT22_OT;

const int DHT22_GH_POWER_PIN = 2;
const int DHT22_GH_SENSE_PIN = 3;
const int DHT22_OT_POWER_PIN = 4;
const int DHT22_OT_SENSE_PIN = 5;

const int INITIALIZED_LED_PIN = 13;

const int READS_PER_SAMPLE = 3;

const int MAX_FILENAME_LEN = 64;

const int32_t EEPROM_MARKER = 0xA00AAFFA;
// 4 Bytes (0,1,2,3)
const int EEPROM_MARKER_ADDRESS = 0;
// 8 Bytes (6,7,8,9,10,11,12,13)
const int EEPROM_ITERATOR_ADDRESS = EEPROM_MARKER_ADDRESS + 4;

const int ITERATIONS_PER_FILE = 288;

const char* LOGFILE_PATH = "/mnt/sd/greenhouse/logs";
const char* LOGFILE_PREFIX = "greenyun"
const char* LOGFILE_SERIES_INFIX = "_A_";

class GHState {
public:
    GHState();
    ~GHState();    
    
    void Step();
private:
    String _getFileIndex();
    int64_t _getCurrentIteration();
    const char* _getCurrentLogFilename();
    void _sampleSensors();
    long _getTimeAsLong();
    String _getTimeAsString();
    void _appendSensorDataToString(float humidity, float temperature, String* outputLine);
    void _sendSensorDataToClient(GHSensor* sensor, YunClient client);
    
    char _logFilename[MAX_FILENAME_LEN];    
    unsigned long _lastUpdate;

    // Loop state
    bool _readSensorDataNextLoop;
    bool _sendToSensorDataToClient;
    bool _writeSensorDataToFile;
    bool _freshSensorDataAvailable;
    
    YunServer _server;
    GHSensor _innerSensor;
    GHSensor _outerSensor;
};