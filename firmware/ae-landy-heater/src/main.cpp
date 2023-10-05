#include <Arduino.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <CircularBuffer.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

const char* ssid = "ae-update";

const int windscreen = 10;
const int lMirror = 6;
const int rMirror = 7;
const int vIn = 1;
const int sysLED = 4;

int onTime = -1; // -1 manual/unconfigured
int rawValue = 0;

float inputVoltage;

bool updateEnable = false;

const float r1 = 13000.0f; // R1 in ohm, 13k
const float r2 = 2200.0f; // R2 in ohm, 2.2k
float vRefScale = (3.3f / 4096.0f) * ((r1 + r2) / r2);
const int numReadings = 100;
int readings[numReadings];
int readIndex = 0;
long total = 0;

unsigned long newtime = 0;

Preferences preferences;

WebServer server(80);

CircularBuffer<float, 500> buffer;

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Variable to store if sending data was successful
String success;

void wifiAPUpdate() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/plain", "Time to update the Landy Heater Controller!");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  updateEnable = true;
}

void loadPreferences()
{
  if (preferences.begin("ae-landy-heater", false))
  {
    if (preferences.getBool("configured"))
    {
      onTime = preferences.getInt("onTime");
      Serial.print("Device has been configured!");
      if (onTime == -1)
      {
        Serial.print("Device is running in manual mode!");
      } 
      else
      {
        Serial.printf("Device is running in auto mode and on for %i minutes", onTime);
      }
    }
  }
  preferences.end();
}

void savePreferences()
{  
  Serial.println("saving settings to flash");
  preferences.begin("ae-landy-heater", false);
  preferences.putInt("onTime", onTime);
  preferences.end();
}

void factoryReset()
{
  nvs_flash_erase(); // erase the NVS partition.
  nvs_flash_init();  // initialize the NVS partition.
  delay(500);
  ESP.restart(); // reset to clear memory
}

long smooth()
{ /* function smooth */
  ////Perform average on sensor readings
  long average;
  // subtract the last reading:
  total = total - readings[readIndex];
  // read the sensor:
  readings[readIndex] = analogRead(vIn);
  // add value to total:
  total = total + readings[readIndex];
  // handle index
  readIndex = readIndex + 1;
  if (readIndex >= numReadings)
  {
    readIndex = 0;
  }
  // calculate the average:
  average = total / numReadings;

  return average;
}

void checkVoltage()
{
  buffer.push(smooth() * (vRefScale * 1.006)); // fill the circular buffer for super smooth values

  if (millis() - newtime >= 1000)
  {
    newtime = millis();
    float avg = 0.0;
    // the following ensures using the right type for the index variable
    using index_t = decltype(buffer)::index_t;
    for (index_t i = 0; i < buffer.size(); i++)
    {
      avg += buffer[i] / buffer.size();
    }

    inputVoltage = avg;
    if (inputVoltage >= 13.00) // I've plucked 13v from fat air
    {
      if (buffer.size() > 499)
      {
        if (onTime != -1)
        {
          //ToDo: set the timer to timout the heater, then turn the heater on
        }
        else
        {
          //ToDo: just turn the heater on
        }
      }
      else
      {
        Serial.println("Waiting for the input voltage to stabilise...");
      }
    }
    else
    {
      Serial.println("input voltage too low to turn on heater!!!");
    }
  }
}

void setup() {
  // initialise digital pins as an output.
  pinMode(windscreen, OUTPUT);
  pinMode(lMirror, OUTPUT);
  pinMode(rMirror, OUTPUT);
  pinMode(vIn, OUTPUT);
  pinMode(sysLED, OUTPUT);

  // set outputs to off (high = off)
  digitalWrite(windscreen, HIGH);
  digitalWrite(lMirror, HIGH);
  digitalWrite(rMirror, HIGH);

  loadPreferences();
  newtime = millis();

  Serial.println("Setup done");
}

// the loop function runs over and over again forever
void loop() {
  if (updateEnable)
  {
    server.handleClient();
    ElegantOTA.loop();
  }
  checkVoltage();
}