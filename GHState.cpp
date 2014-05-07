/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */
#include "GHState.h"
#include "GHSensor.h"

#include <YunServer.h>
#include <YunClient.h> 
#include <FileIO.h>

static String _getFilePostFix();
static long _getTimeAsLong();
static String _getTimeAsString();
static void _getCurrentLogFilename(char* logFilename);
static void _appendSensorDataToString(GHSensor* sensor, String* outputLine);
static void _sendSensorDataToClient(GHSensor* sensor, YunClient client);
static void _sampleSensors(GHState* self);
void _takePicture(GHState* self);

const int DHT22_GH_POWER_PIN = 2;
const int DHT22_GH_SENSE_PIN = 3;
const int DHT22_OT_POWER_PIN = 4;
const int DHT22_OT_SENSE_PIN = 5;

const int READS_PER_SAMPLE = 1;

const int32_t EEPROM_MARKER = 0xA00AAFFA;
// 4 Bytes (0,1,2,3)
const int EEPROM_MARKER_ADDRESS = 0;
// 8 Bytes (6,7,8,9,10,11,12,13)
const int EEPROM_ITERATOR_ADDRESS = EEPROM_MARKER_ADDRESS + 4;

const int ITERATIONS_PER_FILE = 288;

const char* LOGFILE_PATH = "/mnt/sd/greenhouse/logs/";
const char* FILE_PREFIX = "greenyun_";
const char* IMGFILE_PATH = "/mnt/sd/greenhouse/imgs/";

#ifdef ENABLE_DEBUG_LOG
void _debugLog(const char* logMsg) {
    File logFile = FileSystem.open("/mnt/sd/log.txt", FILE_APPEND);
    logFile.println(logMsg);
    logFile.close();
}
#endif

void GHState_Init(GHState* self) {
    // Temperature and Humidity Sensor Setup
    GHSensor_Init(&self->innerSensor, DHT22_GH_POWER_PIN, DHT22_GH_SENSE_PIN, "Greenhouse");
    GHSensor_Init(&self->outerSensor, DHT22_OT_POWER_PIN, DHT22_OT_SENSE_PIN, "Outside");
    self->lastUpdate = 0;
    self->lastImageTaken = 0;
    self->hits = 0;
    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
    self->server.listenOnLocalhost();
    self->server.begin();
}

void GHState_Step(GHState* self) 
{
    DEBUG_LOG("Stepping...")
    if (self->lastUpdate == 0 || (millis() - self->lastUpdate) > 300000) 
    {
        if(self->lastImageTaken == 0 || (millis() - self->lastImageTaken) >
                1800000)
            _takePicture(self);
        _sampleSensors(self);
        _getCurrentLogFilename(self->logFilename);
        File dataFile = FileSystem.open(self->logFilename, FILE_APPEND);
        if (dataFile) 
        {
            String outputLine = "";
            outputLine += _getTimeAsLong();
            outputLine += ",";
            outputLine += _getTimeAsString();
            outputLine += ",";                                
            _appendSensorDataToString(&self->innerSensor, &outputLine);
            outputLine += ",";  
            _appendSensorDataToString(&self->outerSensor, &outputLine);  
            dataFile.println(outputLine);
            self->lastUpdate = millis();          
        }
        dataFile.close();
    }
     // Get clients coming from server
    YunClient client = self->server.accept();
    if (client) 
    {
        String command = client.readString();
        command.trim();
        if (command == "temperature") 
        {
            // get the time from the server:
            String timeString = _getTimeAsString();
        
            client.print("Current time on the Yun: ");
            client.println(timeString);      
            client.print("<hr>");      
            _sendSensorDataToClient(&self->innerSensor, client);
            client.print("<br>"); 
            _sendSensorDataToClient(&self->outerSensor, client);
            client.print("<br>");       
            client.print("<hr>");
            String imgTag = "<img src=\"";
            String imgName = self->lastImageName;
            imgName.replace("/mnt/","/");
            imgTag += imgName;
            imgTag += "\" alt=\"Inside Greenhouse\" height=50% width=50%>";
            client.print(imgTag);
            client.print("<br>Hits so far: ");
            client.print(self->hits++);
        }
        // Close connection and free resources.
        client.stop();
    }
    DEBUG_LOG("Done Stepping")
    delay(50); // Poll every 50ms
}

void _getCurrentLogFilename(char* logFilename) {
    String tmp = String(LOGFILE_PATH);
    tmp += FILE_PREFIX;
    tmp += _getFilePostFix();
    tmp += ".csv";
    tmp.toCharArray(logFilename, MAX_FILENAME_LEN);
}

void _sampleSensors(GHState* self)
{
    DEBUG_LOG("Sampling the sensors...")
    GHSensor_BeginSampling(&self->innerSensor);
    GHSensor_BeginSampling(&self->outerSensor);
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        GHSensor_SampleSensor(&self->innerSensor);
        GHSensor_SampleSensor(&self->outerSensor);
    }
    GHSensor_EndSampling(&self->innerSensor);
    GHSensor_EndSampling(&self->outerSensor);
    DEBUG_LOG("Done Sampling the sensors")
}

String _getFilePostFix() {
    Process time;
    time.runShellCommand("date -u +%Y_%U");
    String timeString = "";
    while(time.available()) {
        char c = time.read();
        timeString += c;
    }
    timeString.trim();
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
    timeString.trim();
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

void _takePicture(GHState* self)
{
    String command = "fswebcam -r 1280x960 ";
    self->lastImageName  = IMGFILE_PATH;
    self->lastImageName += FILE_PREFIX;
    self->lastImageName += _getFilePostFix();
    self->lastImageName += "_";
    self->lastImageName += _getTimeAsLong();
    self->lastImageName += ".png";
    command += self->lastImageName;
    Process proc;
    proc.runShellCommand(command);
    self->lastImageTaken = millis();
}
