/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */
#include "GHState.h"
#include "GHSensor.h"
#include "EEPROM_Utils.h"

#include <YunServer.h>
#include <YunClient.h> 
#include <FileIO.h>
#include <EEPROM.h>

const int DHT22_GH_POWER_PIN = 2;
const int DHT22_GH_SENSE_PIN = 3;
const int DHT22_OT_POWER_PIN = 4;
const int DHT22_OT_SENSE_PIN = 5;

const int READS_PER_SAMPLE = 3;

const int32_t EEPROM_MARKER = 0xA00AAFFA;
// 4 Bytes (0,1,2,3)
const int EEPROM_MARKER_ADDRESS = 0;
// 8 Bytes (6,7,8,9,10,11,12,13)
const int EEPROM_ITERATOR_ADDRESS = EEPROM_MARKER_ADDRESS + 4;

const int ITERATIONS_PER_FILE = 288;

const char* LOGFILE_PATH = "/mnt/sd/greenhouse/logs";
const char* LOGFILE_PREFIX = "greenyun";
const char* LOGFILE_SERIES_INFIX = "_A_";


GHState::GHState() {
    _lastUpdate = 0;
    _hits = 0;
    _logFilename[0] = 0;

    _readSensorDataNextLoop      = false;
    _sendToSensorDataToClient    = false;
    _writeSensorDataToFile       = false;
    _freshSensorDataAvailable    = false;
    
    // Bridge startup  
    Bridge.begin();
    FileSystem.begin(); 

    // Temperature and Humidity Sensor Setup
    _innerSensor = new GHSensor(DHT22_GH_POWER_PIN, DHT22_GH_SENSE_PIN, "Greenhouse");
    _outerSensor = new GHSensor(DHT22_OT_POWER_PIN, DHT22_OT_SENSE_PIN, "Outside");
  
    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
    _server.listenOnLocalhost();
    _server.begin();
}

GHState::~GHState() {
    delete _innerSensor;
    delete _outerSensor;
}

void GHState::Step() {
    if (_readSensorDataNextLoop) {
        _sampleSensors();
        _readSensorDataNextLoop = false;
        _freshSensorDataAvailable = true;
    } else if (_freshSensorDataAvailable) {
        // Get clients coming from server
        if (_sendToSensorDataToClient) {
            YunClient client = _server.accept();
            if (client) {
                // get the time from the server:
                String timeString = _getTimeAsString();
            
                client.print("Current time on the Yun: ");
                client.println(timeString);      
                client.print("<hr>");      
                _sendSensorDataToClient(_innerSensor, client);
                client.print("<br>"); 
                _sendSensorDataToClient(_outerSensor, client);
                client.print("<br>");       
                client.print("<hr>");
                client.print("<br>Hits so far: ");
                client.print(_hits);
            }
      
            // Close connection and free resources.
            client.stop();
            _hits++;      
            _sendToSensorDataToClient = false;
        }
      
        if (_writeSensorDataToFile) {
            File dataFile = FileSystem.open(_getCurrentLogFilename(), FILE_APPEND);
            if (dataFile) {
                String outputLine = "";
                outputLine += _getTimeAsLong();
                outputLine += ", ";
                outputLine += _getTimeAsString();
                outputLine += ", ";                                
                _appendSensorDataToString(_innerSensor, &outputLine);
                outputLine += ", ";  
                _appendSensorDataToString(_outerSensor, &outputLine);  
                dataFile.println(outputLine);
                _lastUpdate = millis();          
            }
            dataFile.close();
            _writeSensorDataToFile = false;
        }

        _freshSensorDataAvailable = false;
    } else {
        YunClient client = _server.accept();
        if (client) {
            // read the command
            String command = client.readString();
            command.trim();        //kill whitespace
            // is "temperature" command?
            if (command == "temperature") {
                static int i = 0;
                client.print("Sensing...");
                client.print(i++);
                _readSensorDataNextLoop = true;
                _sendToSensorDataToClient = true;
            }
            client.stop();
        }    
        if (_lastUpdate == 0 || (millis() - _lastUpdate) > 300000) {
            _readSensorDataNextLoop = true;
            _writeSensorDataToFile = true;      
        }
  
        delay(50); // Poll every 50ms
    }
}

const char* GHState::_getCurrentLogFilename() {
    String tmp = String(LOGFILE_PATH);
    tmp += LOGFILE_PREFIX;
    tmp += LOGFILE_SERIES_INFIX;
    tmp += _getFileIndex();
    tmp  += ".csv";
    tmp.toCharArray(_logFilename, MAX_FILENAME_LEN);
    return _logFilename;
}

int64_t GHState::_getCurrentIteration() {
    int64_t iteration = 0;
    int32_t marker = readInt32AtAddressInEEPROM(EEPROM_MARKER_ADDRESS);
    if (marker == EEPROM_MARKER)
    {
        iteration = readInt64AtAddressInEEPROM(EEPROM_ITERATOR_ADDRESS);
    }
    else
    {
        writeInt32ToAddressInEEPROM(EEPROM_MARKER_ADDRESS, EEPROM_MARKER);
        writeInt32ToAddressInEEPROM(EEPROM_MARKER_ADDRESS, iteration);        
    }
    return iteration;
}

String GHState::_getFileIndex() {
    String fileIndexStr;
    int index = 0;    
    int64_t iteration = _getCurrentIteration();
    index = iteration / ITERATIONS_PER_FILE;
    
    // Pad the string with leading 0's since Arduino does not provide printf
    if (iteration < 10) {
        fileIndexStr = "0000";
    } else if (iteration < 100) {
        fileIndexStr = "000";
    } else if (iteration < 1000) {
        fileIndexStr = "00";
    } else if (iteration < 10000) {
        fileIndexStr = "0";
    }
    
    fileIndexStr += index;
}

void GHState::_sendSensorDataToClient(GHSensor* sensor, YunClient client)
{
    client.print("<br>");
    client.print(sensor->GetName()); 
    client.print(" Humidity (%): ");
    client.print(sensor->GetHumidity());
    client.print("<br>");
    client.print(sensor->GetName()); 
    client.print("Temperature: ");
    client.print(sensor->GetTemperature());
    client.print(" °F");
}

void GHState::_appendSensorDataToString(GHSensor* sensor, String* outputLine)
{
    *outputLine += sensor->GetHumidity();
    *outputLine += ",";
    *outputLine += sensor->GetTemperature();
}

String GHState::_getTimeAsString()
{
    Process time;
    time.runShellCommand("date");
    String timeString = "";
    while(time.available()) {
        char c = time.read();
        timeString += c;
    }
    return timeString;
}

long GHState::_getTimeAsLong()
{
    Process time;
    time.runShellCommand("date -u +%s");
    String timeString = "";
    while(time.available()) {
        char c = time.read();
        timeString += c;
    }
    return timeString.toInt();
}

void GHState::_sampleSensors()
{
    _innerSensor->BeginSampling();
    _outerSensor->BeginSampling();
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        _innerSensor->SampleSensor();
        _outerSensor->SampleSensor();
    }
    _innerSensor->EndSampling();
    _outerSensor->EndSampling();
}