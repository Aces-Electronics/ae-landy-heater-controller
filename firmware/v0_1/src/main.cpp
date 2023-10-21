#include <Arduino.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <CircularBuffer.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSerial.h>


bool debug = true; // turns ap mode on

const char* apssid = "ae-update";

const char* ssid1 = "ShelveNET";
const char* password1 = "buttpiratry";

const char* ssid2 = "vodafoneC230";
const char* password2 = "HAHMLY8BTD";

const int windscreen = 3; // 0.2: 10, 0.1: 3
//const int lMirror = -1; // 0.2: 6, 0.1: 3
//const int rMirror = -1; // 0.2: 7, 0.1: 3
const int vIn = 1; // 1
const int sysLED = 4; // 4
const int onSwitch = 5; // 5
const int numReadings = 100; // ADC samples (of inputVoltage) per poll

int brightness = 20;  // how bright the sysLED is
int fadeAmount = 5;  // how many points to fade the sysLED by
int onTime; // -1 manual/unconfigured
int stateChangeCounter = 0; // counts the on/off toggles for mode setting purposes
int readings[numReadings]; 
int readIndex = 0;
int lastOnSwitchState = 0;
int settingsLoopCounter = 0;

float inputVoltage; // device input voltage, used to know when not to enable the heater

bool updateEnable = false; // stores whether or not to enable WiFI AP for the purposes of updating the firmware
bool updateRunning = false; // stores whether or not the AP has been started
bool onSwitchState = 0; // stores the on switch state, set to 1 (off) 
bool eventTimerExpired = 0; // stores the state of the event timer
bool readyToSetMode = 0; // stores whether or not a change of modes is required
bool needToSavePreferences = 0; // stores whether or not to update preferences, for use in the main loop
bool needToProcessSwitchEvents = 0; // stores whether or not to proces an interrupt driven switch event
bool disableTasks = 0; // stores the state for when the device is in update mode
bool autoMode = 1; // stores the manual or auto run state
bool autoTimeout = 0; // stores the state of the autoTimeout feature

const float r1 = 13000.0f; // R1 in ohm, 13k
const float r2 = 2200.0f; // R2 in ohm, 2.2k
float vRefScale = (3.0f / 4096.0f) * ((r1 + r2) / r2); // gives us the voltage per LSB (0.005060369318)

long total = 0;
long loopCounter = 0;
long int t1 = 0;
long int t2 = 0;

char serial_input_char; // stores the char from inbound uart
#define MAX_MESSAGE 30

unsigned long newtime = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 150;    // the debounce time; increase if the output flickers

hw_timer_t *clear_state_timer = NULL;
hw_timer_t *output_enable_timer = NULL;

Preferences preferences;

AsyncWebServer server(80);
unsigned long ota_progress_millis = 0;

CircularBuffer<float, 200> buffer;

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Variable to store if sending data was successful
String success;

void IRAM_ATTR onTimer1() { // switch input timeout
  eventTimerExpired = true;
}

void IRAM_ATTR onTimer2() { // ouput timeout
  digitalWrite(windscreen, HIGH); // high is off
  timerRestart(output_enable_timer); // reset the counter for next time
  timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
  autoTimeout = true;
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  
  if (d.indexOf("manual") > -1)
  {
    autoMode = 0; // setmode to manual
    WebSerial.print("Mode set to Manual (auto-timeout off)!");
  }
  else if (d.indexOf("auto") > -1)
  {
    autoMode = 1; // setmode to manual
    onTime = getValue(d, ',', 1).toInt();
    WebSerial.printf("Auto mode timer set for %i minutes...\n", onTime);
  }


  if (d == "1")
  {
    digitalWrite(windscreen, LOW); // low is on
    WebSerial.println("Turning outputs ON!");
  }
  else if (d == "0")
  {
    digitalWrite(windscreen, HIGH); // low is on
    WebSerial.println("Turning outputs OFF!");
  }
}

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  WebSerial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    WebSerial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
    WebSerial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
    WebSerial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void wifiAPUpdate() {
  Serial.println("Enabling WiFi for configuration updates, switching off heaters...");
  //digitalWrite(lMirror, HIGH); // low is on
  //digitalWrite(rMirror, HIGH); // low is on
  digitalWrite(windscreen, HIGH); // low is on

  // try and connect to network 1
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid1, password1);
  // wait 5 seconds for connection:
  delay(5000);

  Serial.print("First connected state: ");
  Serial.println(WL_CONNECTED);
  // try and connect to network 2, if required
  if (WL_CONNECTED != 3)
  {
    Serial.print("Attempting to connect to known WiFi networks...");
    WiFi.begin(ssid2, password2);
    // wait 5 seconds for connection:
    delay(5000);
  }

  Serial.print("Second connected state: ");
  Serial.println(WL_CONNECTED);

  if (WL_CONNECTED == 3)
  {
    IPAddress IP = WiFi.localIP();
    Serial.print("Connected to WiFi network with the following IP: ");
    Serial.println(IP);
  }
  else
  {
    WiFi.softAP(apssid);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Couldn't connect to home WiFi, enabling AP with the following IP: ");
    Serial.println(IP);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/update");
  });

  ElegantOTA.begin(&server);    // start ElegantOTA
  WebSerial.begin(&server);     // start webserial
  WebSerial.msgCallback(recvMsg);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
  Serial.println("HTTP server started");
  updateEnable = true;
}

void pulseSysLED() 
{
  // set the brightness of pin 9:
  analogWrite(sysLED, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }
}

void switchEvent() // toggle switch in cabin
{
  needToProcessSwitchEvents = true; // tell the main loop that a switch event has happened
  if (onSwitchState)
  {
    t1 = millis();
    timerRestart(clear_state_timer);
    timerAlarmEnable(clear_state_timer);
  }
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
  buffer.push(smooth() * (vRefScale * 1.01)); // fill the circular buffer for super smooth values

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
    Serial.printf("Input voltage is currently %0.2fV!\n", inputVoltage);
    WebSerial.printf("Input voltage is currently %0.2fV!\n", inputVoltage);

    
    if (inputVoltage < 13.00) // I've plucked 13v from fat air
    {
      Serial.println("Input voltage too low to turn on heater!");
      WebSerial.println("Input voltage too low to turn on heater!");
      digitalWrite(windscreen, HIGH); // high is off
      timerRestart(output_enable_timer); // reset the counter for next time
      timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
      Serial.println("Outputs and timer disabled!");
      WebSerial.println("Outputs and timer disabled!");
    }
  }
}

void processSwitchEvents()
{
  if ((millis() - lastDebounceTime) > debounceDelay) 
  {
    if (onSwitchState)
    {
      stateChangeCounter++;
      Serial.println("Looks like switch is on...");
      WebSerial.println("Looks like switch is on...");
      if (inputVoltage >= 13.00) // I've plucked 13v from fat air
      {
        if ((!readyToSetMode) && (settingsLoopCounter == 0))
        {
          if (buffer.size() > 199)
          {
            //logic is reversed, 0/false is on
            if ((autoMode) && (!autoTimeout)) // auto mode, timer not expired
            {
              Serial.printf("Auto mode timer running for %i minutes...\n", onTime);
              WebSerial.printf("Auto mode timer running for %i minutes...\n", onTime);
              //digitalWrite(lMirror, LOW); // low is on
              //digitalWrite(rMirror, LOW); // low is on
              digitalWrite(windscreen, LOW); // low is on
              timerAlarmEnable(output_enable_timer); // enable the output timeout timer
            }
            else if ((!autoMode) && (autoTimeout)) // manual mode
            {
              Serial.println("Manual mode, switch on...");
              WebSerial.println("Manual mode, switch on...");
              //digitalWrite(lMirror, LOW); // low is on
              //digitalWrite(rMirror, LOW); // low is on
              digitalWrite(windscreen, LOW); // low is on
            }
          }
          else
          {
            Serial.println("Waiting for the input voltage to stabilise...");
            WebSerial.println("Waiting for the input voltage to stabilise...");
          }
        }
      }
    }
    else
    {
      Serial.println("Looks like switch is off...");
      WebSerial.println("Looks like switch is off...");
      // turn off the outputs
      digitalWrite(windscreen, HIGH); // high is off
      timerRestart(output_enable_timer); // reset the counter for next time
      timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
      Serial.println("Outputs and timer disabled!");
      WebSerial.println("Outputs and timer disabled!");
    }
  }
  else 
  {
    Serial.println("Debounced onSwitchEvents!");
    WebSerial.println("Debounced onSwitchEvents!");
  }
  lastDebounceTime = millis();
  needToProcessSwitchEvents = false;
}

void loadPreferences()
{
  if (preferences.begin("ae-landy-heater", false))
  {
    if (preferences.getBool("configured"))
    {
      onTime = preferences.getInt("onTime");
      autoMode = preferences.getBool("autoMode");
      Serial.print("Device has been configured!");
      if (!autoMode)
      {
        Serial.print("Device is running in manual mode!");
      } 
      else
      {
        Serial.printf("Device is running in auto mode and on for %i minutes", onTime);
      }
    }
    else
    {
      preferences.putBool("autoMode", true);
      preferences.putInt("onTime", 10);
      preferences.putBool("configured", true);
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
  needToSavePreferences = false;
}

void factoryReset()
{
  nvs_flash_erase(); // erase the NVS partition.
  nvs_flash_init();  // initialize the NVS partition.
  delay(500);
  ESP.restart(); // reset to clear memory
}

void processEventData()
{
  Serial.printf("stateChangeCounter has been set high %d times\n", stateChangeCounter);
  Serial.println(stateChangeCounter);
  Serial.printf("SettingsLoopCounter = %i \n\n", settingsLoopCounter);
  Serial.printf("ModeResetCounter = %i \n\n", t2-t1);

  WebSerial.printf("stateChangeCounter has been set high %d times\n", stateChangeCounter);
  WebSerial.println(stateChangeCounter);
  WebSerial.printf("SettingsLoopCounter = %i \n\n", settingsLoopCounter);
  WebSerial.printf("ModeResetCounter = %i \n\n", t2-t1);


  settingsLoopCounter++; // set to 1 for first loop, timer from switch toggled on
  if ((stateChangeCounter == 3) && (settingsLoopCounter == 1))
  {
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    updateEnable = true; // enable WiFi AP
    Serial.println("AP enabled for firmware update!\n");
    WebSerial.println("AP enabled for firmware update!\n");
    eventTimerExpired = false;
    timerAlarmDisable(clear_state_timer);
  }
  if ((stateChangeCounter == 2) && (settingsLoopCounter == 1))
  {
    Serial.println("Switch has been toggled twice, entering into mode settings...");
    WebSerial.println("Switch has been toggled twice, entering into mode settings...");
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    Serial.println("Soft resetting loop counters...");
    WebSerial.println("Soft resetting loop counters...");
  }
  if (settingsLoopCounter == 2)
  {
    Serial.println("Mode is being set...");
    WebSerial.println("Mode is being set...");
    onTime = stateChangeCounter;
    if (onTime == 0) // switch state to manual
    {
      onTime = -1;
      autoMode = 0;
      Serial.println("Mode is set to manual!");
      WebSerial.println("Mode is set to manual!");
    }
    else
    {
      Serial.printf("Mode is set to Auto with a timeout of %i minutes!\n"), stateChangeCounter;
      WebSerial.printf("Mode is set to Auto with a timeout of %i minutes!\n"), stateChangeCounter;
    }
    needToSavePreferences = true;
    stateChangeCounter = 0;
    timerAlarmDisable(clear_state_timer);
    Serial.println("Soft resetting loop counters...");
    WebSerial.println("Soft resetting loop counters...");
  }
  eventTimerExpired = false;
}

void setup() {
  // initialise serial for debugging, uart over USB
  Serial.begin(9600);

  // initialise digital pins as an output.
  pinMode(windscreen, OUTPUT);
  //pinMode(lMirror, INPUT_PULLDOWN);
  //pinMode(rMirror, INPUT_PULLDOWN);
  pinMode(vIn, INPUT);
  pinMode(sysLED, OUTPUT);
  pinMode(onSwitch, INPUT_PULLUP);

  // set outputs to off (high = off)
  digitalWrite(windscreen, HIGH);
  //digitalWrite(lMirror, HIGH);
  //digitalWrite(rMirror, HIGH);

  loadPreferences();
  newtime = millis();

  // detect switch events
  attachInterrupt(digitalPinToInterrupt(onSwitch), switchEvent, CHANGE);

  // setup state change counter wipe, to timout enter settings changes/modes
  clear_state_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(clear_state_timer, &onTimer1, true);
  timerAlarmWrite(clear_state_timer, 5000000, true); // 5 seconds

  output_enable_timer = timerBegin(1, 80, true);
  timerAttachInterrupt(output_enable_timer, &onTimer2, true);
  timerAlarmWrite(output_enable_timer, 600000000, true); // 10 minutes

  //ToDo: set the preferences, read and set the timer

  Serial.println("\n\nSetup done");
}

// the loop function runs over and over again forever
void loop() 
{
  if ((updateEnable) || (debug))
  {
    if (updateRunning)
    {
      ElegantOTA.loop();
    }
    else
    {
      wifiAPUpdate();
    }
    updateRunning = true;
  }

  t2 = millis();

  if (needToProcessSwitchEvents)
  {
    onSwitchState = !digitalRead(onSwitch); // logic is inverted
    processSwitchEvents();
  }  

  // check the input voltage, control the heaters
  checkVoltage();

  // pulse the sysLED
  pulseSysLED();

  // save preferences, if required
  if (needToSavePreferences)
  {
    savePreferences();
  }

  if (eventTimerExpired)
  {
    processEventData();
  }

  static char buffer[MAX_MESSAGE];
  static unsigned char index = 0;

  while (Serial.available() > 0) {
    serial_input_char = Serial.read();
    if (serial_input_char == '\r') {
      Serial.print("You entered: ");
      Serial.println(buffer);
      buffer[0] = 0;
      index = 0;
    } else {        
      if (index < MAX_MESSAGE-1) {
        buffer[index++] = serial_input_char;
        buffer[index] = 0;
      }
    }
  } 

  if (autoTimeout)
  {
    Serial.println("Auto timeout has switched off outputs...");
    WebSerial.println("Auto timeout has switched off outputs...");
  }

  if ((t2-t1 > 10000) && (eventTimerExpired)) // timeout mode setting after 10 seconds
  {
    readyToSetMode = false;
    stateChangeCounter = 0;
    settingsLoopCounter = 0;
    Serial.println("Hard resetting loop counters and states: Settings timeout!\n");
    WebSerial.println("Hard resetting loop counters and states: Settings timeout!\n");
    t1 = millis(); // reset the timer for the third loop, save settings
    timerAlarmDisable(clear_state_timer);
  }
  delay(30); // needed to make the LED pulse
}