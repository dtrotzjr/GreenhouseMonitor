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

typedef struct {
  float fahrenheit;
  float humidity;
  
} DHT22_Data;

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;
String startString;
long hits = 0;

dht22 DHT22a;
dht22 DHT22b;
unsigned long lastUpdate;
const int fnameLen = 32;
char fname[fnameLen];
  
  
void setup() {
  Serial.begin(9600);

  // Bridge startup
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  FileSystem.begin();  
  digitalWrite(13, HIGH);

  // Temperature and Humidity Sensor Setup
  DHT22a.attach(2);
  DHT22b.attach(4);  
  
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

void saveSensorDataTofile(float humidity, float temperature, String* whatToPrint)
{
    *whatToPrint += humidity;
    *whatToPrint += ",";
    *whatToPrint += temperature;
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

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();
  File dataFile = FileSystem.open(fname, FILE_APPEND);
  float humidityIn = 0;
  float temperatureIn = 0;
  float humidityOut = 0;
  float temperatureOut = 0;
  
  if (DHT22a.read() == 0)
  {
    humidityIn = (float)DHT22a.humidity;
    temperatureIn = DHT22a.fahrenheit();
  }
  
  if (DHT22b.read() == 0)
  {
    humidityOut = (float)DHT22b.humidity;
    temperatureOut = DHT22b.fahrenheit();
  }
  
  // There is a new client?
  if (client) {
    // read the command
    String command = client.readString();
    command.trim();        //kill whitespace
    Serial.println(command);
    // is "temperature" command?
    if (command == "temperature") {

      // get the time from the server:
      String timeString = getTimeString();
      
      client.print("Current time on the Yun: ");
      client.println(timeString);      
      client.print("<hr>");      
      sendSensorDataToClient(humidityIn, temperatureIn, "Greenhouse", client);
      client.print("<br>"); 
      sendSensorDataToClient(humidityOut, temperatureOut, "Outside", client);
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
  }
  
  unsigned long now = millis();
  if (dataFile)
  {
    if (lastUpdate == 0 || (now - lastUpdate) > 300000)
    {
      String whatToPrint = "";
      whatToPrint += getUnixTime();
      whatToPrint += ",";
      saveSensorDataTofile(humidityIn, temperatureIn, &whatToPrint);
      whatToPrint += ",";  
      saveSensorDataTofile(humidityOut, temperatureOut, &whatToPrint);  
      dataFile.println(whatToPrint);
      lastUpdate = now;
    }
    
    dataFile.flush();
    dataFile.close();
  }
  delay(2000); // Poll every 50ms
}


