/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */
#include "GHState.h"
#include "GHSensor.h"

// #include <YunServer.h>
// #include <YunClient.h> 
#include <FileIO.h>

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

const int READS_PER_SAMPLE = 1;

const int32_t EEPROM_MARKER = 0xA00AAFFA;
// 4 Bytes (0,1,2,3)
const int EEPROM_MARKER_ADDRESS = 0;
// 8 Bytes (6,7,8,9,10,11,12,13)
const int EEPROM_ITERATOR_ADDRESS = EEPROM_MARKER_ADDRESS + 4;

const int ITERATIONS_PER_FILE = 288;

const char* LOGFILE_PATH = "/mnt/sd/greenhouse/logs/";
const char* LOGFILE_PREFIX = "greenyun_";

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
  
    // Listen for incoming connection only from localhost
    // (no one from the external network could connect)
//    self->server = new YunServer();
//    self->server->listenOnLocalhost();
//    self->server->begin();
    DEBUG_LOG("GHState_Create")  
}

void GHState_Step(GHState* self) 
{
    DEBUG_LOG("GHState_Step")
    if (self->readSensorDataNextLoop) 
    {
        DEBUG_LOG("readSensorDataNextLoop")
        _sampleSensors(self);
        self->readSensorDataNextLoop = false;
        self->freshSensorDataAvailable = true;
        delay(50);
        if(self->readSensorDataNextLoop) {DEBUG_LOG("A OUCH!!!")} else {DEBUG_LOG("A GOOD!!!")}
        
        if(self->readSensorDataNextLoop) {DEBUG_LOG("A OUCH!!!")} else {DEBUG_LOG("A GOOD!!!")}
        
    } 
    else if (self->freshSensorDataAvailable) 
    {
            DEBUG_LOG("freshSensorDataAvailable")
        // Get clients coming from server
        if (self->sendToSensorDataToClient) 
        {
            DEBUG_LOG("sendToSensorDataToClient")
            // YunClient client = self->server->accept();
//             if (client) 
//             {
//                 // get the time from the server:
//                 String timeString = _getTimeAsString();
//             
//                 client.print("Current time on the Yun: ");
//                 client.println(timeString);      
//                 client.print("<hr>");      
//                 _sendSensorDataToClient(self->innerSensor, client);
//                 client.print("<br>"); 
//                 _sendSensorDataToClient(self->outerSensor, client);
//                 client.print("<br>");       
//                 client.print("<hr>");
//                 client.print("<br>Hits so far: ");
//                 client.print(self->hits);
//             }
//       
//             // Close connection and free resources.
//             client.stop();
//             self->hits++;      
            self->sendToSensorDataToClient = false;
        }
      
        if (self->writeSensorDataToFile) 
        {
            DEBUG_LOG("writeSensorDataToFile")
            _getCurrentLogFilename(self->logFilename);
            File dataFile =
                FileSystem.open(self->logFilename/*"/mnt/sd/greenhouse/logs/test.log"*/, FILE_APPEND);
            if (dataFile) 
            {
                String outputLine = "";
                outputLine += _getTimeAsLong();
                outputLine += ", ";
                outputLine += _getTimeAsString();
                outputLine += ", ";                                
                _appendSensorDataToString(&self->innerSensor, &outputLine);
                outputLine += ", ";  
                _appendSensorDataToString(&self->outerSensor, &outputLine);  
                dataFile.println(outputLine);
                self->lastUpdate = millis();          
            }
            else
            {
                DEBUG_LOG("Failed to open file")
                DEBUG_LOG(self->logFilename)
            }
            dataFile.close();
            self->writeSensorDataToFile = false;
        }
        self->freshSensorDataAvailable = false;
    } 
    else 
    {
        DEBUG_LOG("*****")
        // YunClient client = self->server->accept();
//         if (client) 
//         {
//             // read the command
//             String command = client.readString();
//             command.trim();        //kill whitespace
//             // is "temperature" command?
//             if (command == "temperature") 
//             {
//                 DEBUG_LOG("web client wants update")
//                 static int i = 0;
//                 client.print("Sensing...");
//                 client.print(i++);
//                 self->readSensorDataNextLoop = true;
//                 self->sendToSensorDataToClient = true;
//             }
//             client.stop();
//         }    
        if (self->lastUpdate == 0 || (millis() - self->lastUpdate) > 2000/*300000*/) 
        {
            DEBUG_LOG("*** time to update ***")
            self->readSensorDataNextLoop = true;
            self->writeSensorDataToFile = true;      
        }
    }
    if(self->readSensorDataNextLoop) {DEBUG_LOG("B OUCH!!!")} else {DEBUG_LOG("B GOOD!!!")}    
    delay(50); // Poll every 50ms
    if(self->readSensorDataNextLoop) {DEBUG_LOG("C OUCH!!!")} else {DEBUG_LOG("C GOOD!!!")}
}

void _getCurrentLogFilename(char* logFilename) {
    String tmp = String(LOGFILE_PATH);
    tmp += LOGFILE_PREFIX;
    tmp += _getFilePostFix();
    tmp += ".csv";
    tmp.toCharArray(logFilename, MAX_FILENAME_LEN);
    DEBUG_LOG(logFilename)
}

void _sampleSensors(GHState* self)
{
    DEBUG_LOG("sampleSensors")            
    GHSensor_BeginSampling(&self->innerSensor);
    GHSensor_BeginSampling(&self->outerSensor);
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        GHSensor_SampleSensor(&self->innerSensor);
        GHSensor_SampleSensor(&self->outerSensor);
    }
    GHSensor_EndSampling(&self->innerSensor);
    GHSensor_EndSampling(&self->outerSensor);
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

// void _sendSensorDataToClient(GHSensor* sensor, YunClient client)
// {
//     client.print("<br>");
//     client.print(GHSensor_GetName(sensor)); 
//     client.print(" Humidity (%): ");
//     client.print(GHSensor_GetHumidity(sensor));
//     client.print("<br>");
//     client.print(GHSensor_GetName(sensor)); 
//     client.print("Temperature: ");
//     client.print(GHSensor_GetTemperature(sensor));
//     client.print(" °F");
// }

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
