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
    _innerSensor = CreateGHSensor(DHT22_GH_POWER_PIN, DHT22_GH_SENSE_PIN, "Greenhouse");
    _outerSensor = CreateGHSensor(DHT22_OT_POWER_PIN, DHT22_OT_SENSE_PIN, "Outside");
  
    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
    _server.listenOnLocalhost();
    _server.begin();
}

GHState::~GHState() {
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
    tmp += _getFilePostFix();
    tmp  += ".csv";
    tmp.toCharArray(_logFilename, MAX_FILENAME_LEN);
    return _logFilename;
}

String GHState::_getFilePostFix() {
    Process time;
    time.runShellCommand("date -u +%Y_%U");
    String timeString = "";
    while(time.available()) {
        char c = time.read();
        timeString += c;
    }
    return timeString;
}

void GHState::_sendSensorDataToClient(GHSensor* sensor, YunClient client)
{
    client.print("<br>");
    client.print(GetName(sensor)); 
    client.print(" Humidity (%): ");
    client.print(GetHumidity(sensor));
    client.print("<br>");
    client.print(GetName(sensor)); 
    client.print("Temperature: ");
    client.print(GetTemperature(sensor));
    client.print(" °F");
}

void GHState::_appendSensorDataToString(GHSensor* sensor, String* outputLine)
{
    *outputLine += GetHumidity(sensor);
    *outputLine += ",";
    *outputLine += GetTemperature(sensor);
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
    BeginSampling(_innerSensor);
    BeginSampling(_outerSensor);
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        SampleSensor(_innerSensor);
        SampleSensor(_outerSensor);
    }
    EndSampling(_innerSensor);
    EndSampling(_outerSensor);
}