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

static String _getFilePostFix();
static long _getTimeAsLong();
static String _getTimeAsString();
static void _getCurrentLogFilename(char* logFilename);
static void _appendSensorDataToString(GHSensor* sensor, String* outputLine);
static void _sendSensorDataToClient(GHSensor* sensor, YunClient client);
static void _sampleSensors(GHState* self);

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


GHState* GHState_Create() {
    GHState* self = (GHState*)calloc(1, sizeof(GHState));

    // Temperature and Humidity Sensor Setup
    self->_innerSensor = GHSensor_Create(DHT22_GH_POWER_PIN, DHT22_GH_SENSE_PIN, "Greenhouse");
    self->_outerSensor = GHSensor_Create(DHT22_OT_POWER_PIN, DHT22_OT_SENSE_PIN, "Outside");
  
    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
    self->_server.listenOnLocalhost();
    self->_server.begin();
}

void GHState_Step(GHState* self) {
    if (self->_readSensorDataNextLoop) {
        _sampleSensors(self);
        self->_readSensorDataNextLoop = false;
        self->_freshSensorDataAvailable = true;
    } else if (self->_freshSensorDataAvailable) {
        // Get clients coming from server
        if (self->_sendToSensorDataToClient) {
            YunClient client = self->_server.accept();
            if (client) {
                // get the time from the server:
                String timeString = _getTimeAsString();
            
                client.print("Current time on the Yun: ");
                client.println(timeString);      
                client.print("<hr>");      
                _sendSensorDataToClient(self->_innerSensor, client);
                client.print("<br>"); 
                _sendSensorDataToClient(self->_outerSensor, client);
                client.print("<br>");       
                client.print("<hr>");
                client.print("<br>Hits so far: ");
                client.print(self->_hits);
            }
      
            // Close connection and free resources.
            client.stop();
            self->_hits++;      
            self->_sendToSensorDataToClient = false;
        }
      
        if (self->_writeSensorDataToFile) {
            _getCurrentLogFilename(self->_logFilename);
            File dataFile = FileSystem.open(self->_logFilename, FILE_APPEND);
            if (dataFile) {
                String outputLine = "";
                outputLine += _getTimeAsLong();
                outputLine += ", ";
                outputLine += _getTimeAsString();
                outputLine += ", ";                                
                _appendSensorDataToString(self->_innerSensor, &outputLine);
                outputLine += ", ";  
                _appendSensorDataToString(self->_outerSensor, &outputLine);  
                dataFile.println(outputLine);
                self->_lastUpdate = millis();          
            }
            dataFile.close();
            self->_writeSensorDataToFile = false;
        }
        self->_freshSensorDataAvailable = false;
    } else {
        YunClient client = self->_server.accept();
        if (client) {
            // read the command
            String command = client.readString();
            command.trim();        //kill whitespace
            // is "temperature" command?
            if (command == "temperature") {
                static int i = 0;
                client.print("Sensing...");
                client.print(i++);
                self->_readSensorDataNextLoop = true;
                self->_sendToSensorDataToClient = true;
            }
            client.stop();
        }    
        if (self->_lastUpdate == 0 || (millis() - self->_lastUpdate) > 300000) {
            self->_readSensorDataNextLoop = true;
            self->_writeSensorDataToFile = true;      
        }
  
        delay(50); // Poll every 50ms
    }
}

void _getCurrentLogFilename(char* logFilename) {
    String tmp = String(LOGFILE_PATH);
    tmp += LOGFILE_PREFIX;
    tmp += _getFilePostFix();
    tmp  += ".csv";
    tmp.toCharArray(logFilename, MAX_FILENAME_LEN);
}

void _sampleSensors(GHState* self)
{
    GHSensor_BeginSampling(self->_innerSensor);
    GHSensor_BeginSampling(self->_outerSensor);
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        GHSensor_SampleSensor(self->_innerSensor);
        GHSensor_SampleSensor(self->_outerSensor);
    }
    GHSensor_EndSampling(self->_innerSensor);
    GHSensor_EndSampling(self->_outerSensor);
}

String _getFilePostFix() {
    Process time;
    time.runShellCommand("date -u +%Y_%U");
    String timeString = "";
    while(time.available()) {
        char c = time.read();
        timeString += c;
    }
    return timeString;
}

void _sendSensorDataToClient(GHSensor* sensor, YunClient client)
{
    client.print("<br>");
    client.print(GHSensor_GetName(sensor)); 
    client.print(" Humidity (%): ");
    client.print(GHSensor_GetHumidity(sensor));
    client.print("<br>");
    client.print(GHSensor_GetName(sensor)); 
    client.print("Temperature: ");
    client.print(GHSensor_GetTemperature(sensor));
    client.print(" °F");
}

void _appendSensorDataToString(GHSensor* sensor, String* outputLine)
{
    *outputLine += GHSensor_GetHumidity(sensor);
    *outputLine += ",";
    *outputLine += GHSensor_GetTemperature(sensor);
}

String _getTimeAsString()
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

long _getTimeAsLong()
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