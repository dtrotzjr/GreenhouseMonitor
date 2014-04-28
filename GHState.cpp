/*
 * Greenhouse Runloop State
 * Tracks the state of the app between each iteration of the run loop
 * David Trotz
 * 04/27/2014
 */

#include "EEPROM_Utils.h"

GHState::GHState() {
    lastUpdate = 0;
    fname[MAX_FILENAME_LEN]     = "";

    greenhouseHumidity          = 0.0;
    greenhouseTemperature       = 0.0;
    outsideHumidity             = 0.0;
    outsideTemperature          = 0.0;

    readSensorDataNextLoop      = false;
    sendToSensorDataToClient    = false;
    writeSensorDataToFile       = false;
    freshSensorDataAvailable    = false;
    
    // Bridge startup  
    Bridge.begin();
    FileSystem.begin(); 

    // Temperature and Humidity Sensor Setup
    _innerSensor = new GHSensor(DHT22_GH_POWER_PIN, DHT22_GH_SENSE_PIN);
    _outerSensor = new GHSensor(DHT22_OT_POWER_PIN, DHT22_OT_SENSE_PIN);
  
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
    if (readSensorDataNextLoop) {
        sampleSensors();
        readSensorDataNextLoop = false;
        freshSensorDataAvailable = true;
    } else if (freshSensorDataAvailable) {
        // Get clients coming from server
        if (sendToSensorDataToClient) {
            YunClient client = server.accept();
            if (client) {
                // get the time from the server:
                String timeString = getTimeString();
            
                client.print("Current time on the Yun: ");
                client.println(timeString);      
                client.print("<hr>");      
                sendSensorDataToClient(greenhouseHumidity, greenhouseTemperature, "Greenhouse", client);
                client.print("<br>"); 
                sendSensorDataToClient(outsideHumidity, outsideTemperature, "Outside", client);
                client.print("<br>");       
                client.print("<hr>");
                client.print("<br>This sketch has been running since ");
                client.print(startString);
                client.print("<br>Hits so far: ");
                client.print(hits);      
            }
      
            // Close connection and free resources.
            client.stop();
            hits++;      
            sendToSensorDataToClient = false;
        }
      
        if (writeSensorDataToFile) {
            File dataFile = FileSystem.open(fname, FILE_APPEND);
            if (dataFile) {
                String outputLine = "";
                outputLine += getUnixTime();
                outputLine += ", ";
                outputLine += getTimeString();
                outputLine += ", ";                                
                appendSensorDataToString(greenhouseHumidity, greenhouseTemperature, &outputLine);
                outputLine += ", ";  
                appendSensorDataToString(outsideHumidity, outsideTemperature, &outputLine);  
                dataFile.println(outputLine);
                lastUpdate = millis();          
            }
            dataFile.close();
            writeSensorDataToFile = false;
        }

        freshSensorDataAvailable = false;
    } else {
        YunClient client = server.accept();
        if (client) {
            // read the command
            String command = client.readString();
            command.trim();        //kill whitespace
            // is "temperature" command?
            if (command == "temperature") {
                static int i = 0;
                client.print("Sensing...");
                client.print(i++);
                readSensorDataNextLoop = true;
                sendToSensorDataToClient = true;
            }
            client.stop();
        }    
        if (lastUpdate == 0 || (millis() - lastUpdate) > 300000) {
            readSensorDataNextLoop = true;
            writeSensorDataToFile = true;      
        }
  
        delay(50); // Poll every 50ms
    }
}

const char* GHState::_getCurrentLogFilename() {
    String tmp = LOGFILE_PATH;
    tmp += LOGFILE_PREFIX;
    tmp += LOGFILE_INFIX;
    tmp += _getFileIndex();
    tmp  += ".csv";
    tmp.toCharArray(fname, _logFilename);
    return _logFilename;
}

int64_t GHState::_getCurrentIteration() {
    int64_t iteration = 0;
    int32_t marker = EEPROM_Utils::readInt32AtAddress(EEPROM_MARKER_ADDRESS);
    if (marker == EEPROM_MARKER)
    {
        iteration = EEPROM_Utils::readInt64AtAddress(EEPROM_ITERATOR_ADDRESS);
    }
    else
    {
        EEPROM_Utils::writeInt32AtAddress(EEPROM_MARKER_ADDRESS, EEPROM_MARKER);
        EEPROM_Utils::writeInt32AtAddress(EEPROM_MARKER_ADDRESS, iteration);        
    }
    return iteration;
}

String GHState::_getFileIndex() {
    String* fileIndexStr = "";
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
    _innerSensor.BeginSampling();
    _outerSensor.BeginSampling();
    for (int i = 0; i < READS_PER_SAMPLE; i++)
    {
        _innerSensor.SampleSensor();
        _outerSensor.SampleSensor();
    }
    _innerSensor.EndSampling();
    _outerSensor.EndSampling();
}