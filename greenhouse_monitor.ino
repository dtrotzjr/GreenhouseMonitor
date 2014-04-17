/*
Temperature web interface

This example shows how to serve data from an analog input  
via the Arduino YÃºn's built-in webserver using the Bridge library.

The circuit:
* TMP36 temperature sensor on analog pin A1
* SD card attached to SD card slot of the Arduino YÃºn

Prepare your SD card with an empty folder in the SD root 
named "arduino" and a subfolder of that named "www". 
This will ensure that the YÃºn will create a link 
to the SD to the "/mnt/sd" path.

In this sketch folder is a basic webpage and a copy of zepto.js, a 
minimized version of jQuery.  When you upload your sketch, these files
will be placed in the /arduino/www/TemperatureWebPanel folder on your SD card.

You can then go to http://arduino.local/sd/TemperatureWebPanel
to see the output of this sketch.

You can remove the SD card while the Linux and the 
sketch are running but be careful not to remove it while
the system is writing to it.

created  6 July 2013
by Tom Igoe


This example code is in the public domain.

*/
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h> 
#include <dht22.h>
#include <FileIO.h>

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;
String startString;
long hits = 0;

dht22 DHT22_GH;
dht22 DHT22_OT;

const int DHT22_GH_POWER_PIN = 2;
const int DHT22_GH_SENSE_PIN = 3;
const int DHT22_OT_POWER_PIN = 4;
const int DHT22_OT_SENSE_PIN = 5;

const int INITIALIZED_LED_PIN = 13;

const int READS_PER_SAMPLE = 3;

unsigned long lastUpdate;
const int fnameLen = 32;
char fname[fnameLen];

float greenhouseHumidity = 0.0;
float greenhouseTemperature = 0.0;
float outsideHumidity = 0.0;
float outsideTemperature = 0.0;
  
bool readSensorDataNextLoop = false;
bool sendToSensorDataToClient = false;
bool writeSensorDataToFile = false;
bool freshSensorDataAvailable = false;
  
void setup() {
	Serial.begin(9600);
	Serial.println("Setup...");  
	pinMode(INITIALIZED_LED_PIN,OUTPUT);
	digitalWrite(INITIALIZED_LED_PIN, LOW); 
  
	// Bridge startup  
	Bridge.begin();
	FileSystem.begin();  

	// Temperature and Humidity Sensor Setup
	pinMode(DHT22_GH_POWER_PIN,OUTPUT);
	digitalWrite(DHT22_GH_POWER_PIN, LOW);
	DHT22_GH.attach(DHT22_GH_SENSE_PIN);
  
	pinMode(DHT22_OT_POWER_PIN,OUTPUT);
	digitalWrite(DHT22_OT_POWER_PIN, LOW);   
	DHT22_OT.attach(DHT22_OT_SENSE_PIN);  
  
	// Listen for incoming connection only from localhost
	// (no one from the external network could connect)
	server.listenOnLocalhost();
	server.begin();

	// get the time that this sketch started:
	Process startTime;
	startTime.runShellCommand("date");
	while(startTime.available()) {
		char c = startTime.read();
		startString += c;
	}
  
	lastUpdate = 0;
  
	String fnameStr = "/mnt/sd/greenyun_";
	fnameStr += getUnixTime();
	fnameStr  += ".txt";
	fnameStr.toCharArray(fname, fnameLen);
  
	digitalWrite(13, HIGH);
}

void sendSensorDataToClient(float humidity, float temperature, const char* name, YunClient client)
{
	client.print("<br>");
	client.print(name); 
	client.print(" Humidity (%): ");
	client.print(humidity);
	client.print("<br>");
	client.print(name); 
	client.print("Greenhouse Temperature: ");
	client.print(temperature);
	client.print(" °F");
}

void appendSensorDataToString(float humidity, float temperature, String* outputLine)
{
	*outputLine += humidity;
	*outputLine += ",";
	*outputLine += temperature;
}

String getTimeString()
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

long getUnixTime()
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

void sampleSensors()
{
	greenhouseHumidity = 0.0;
	greenhouseTemperature = 0.0;
	outsideHumidity = 0.0;
	outsideTemperature = 0.0;
	// We will take several samples from the sensors
	// and then average the results to get something 
	// close to the actual values
	for (int i = 0; i < READS_PER_SAMPLE; i++)
	{
		// Power on the Greenhouse sensor and take a reading.
		digitalWrite(DHT22_GH_POWER_PIN, HIGH);
		delay(2000);
		if (DHT22_GH.read() == 0)
		{
			greenhouseHumidity += (float)DHT22_GH.humidity;
			greenhouseTemperature += DHT22_GH.fahrenheit();
		}
		digitalWrite(DHT22_GH_POWER_PIN, LOW);  
  
		// Power on the Outside sensor and take a reading.  
		digitalWrite(DHT22_OT_POWER_PIN, HIGH);
		delay(2000);    
		if (DHT22_OT.read() == 0)
		{
			outsideHumidity += (float)DHT22_OT.humidity;
			outsideTemperature += DHT22_OT.fahrenheit();
		}
		digitalWrite(DHT22_OT_POWER_PIN, LOW);
	}
	greenhouseHumidity /= READS_PER_SAMPLE;
	greenhouseTemperature /= READS_PER_SAMPLE;
	outsideHumidity /= READS_PER_SAMPLE;
	outsideTemperature /= READS_PER_SAMPLE; 
}

void loop() {
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
                                client.print("Sensing...");
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


