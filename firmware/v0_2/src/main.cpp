//////
// How to read the LED on the JonoTron Hardware:
//  Flashing red:
//    Battery voltage too low, switch off and auto-mode off, heaters off.
//  Flashing green:
//    Battery voltage okay, switch off and auto-mode off, heaters off.
//  Solid red:
//    Battery voltage too low, switch on or auto-mode on, heaters off.
//  Solid green:
//    Battery voltage okay, switch on or auto-mode on, heaters on.
//  Flash blue once:
//    System booting/rebooting.
//  Flashing blue:
//    If WiFi unconfigured, hardware in setup mode, access the web interface at http://192.168.4.1 after connecting to the ae-update WiFi network.
//    If Wifi configured, accessing Internet and checking for new firmware
//  Solid red/green/blue combined (probably looks mostly white):
//    Software update available, after the system updates the device will restart and the LED will return to the system's state.
/////

//////
// How to use the physical switch:
//  Switch in the on position:
//    Manually turn on the heaters (maybe this should be the enable auto mode function, rather than manual mode on?).
//  Switch in the off position:
//    System in auto mode, will run the heaters every time the car starts for the duration of the timeout set in the web portal (defaults to 10 mins).
//  To turn on the config portal:
//    Pause for a second on each state change (off to on or on to off), toggle the switch on-off-on-off-on-off (turn it on three times, turn it off)
//    You can then connect a phone, laptop or tablet to the ae-update wifi network; no password required. A captive portal with some options should pop up.
//    You can also access the portal, once started, by browsing to http://192.168.4.1
//    The portal will timeout after 3 minutes.
//  To factory default the device:
//    Use the same process as above but toggle the switch on 5 times, then off, so
//    on-off-on-off-on-off-on-off-on-off
//    This will set the auto mode timout to 10 mins and erase all portal/WiFi settings.
//    If you connect the device to your home network, use a factory default to regain access to the portal if you are unable to find the device's IP address, which will  no longer be 192.168.4.1, check the WiFi router's web interface for the
////

#include <Arduino.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <CircularBuffer.hpp>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>

#define debug_mode false

const char *ssid = "ae-update";
float onVoltage = 13.80; // input voltage to allow the heaters turn on

#define OTAGH_OWNER_NAME "Aces-Electronics"
#define OTAGH_REPO_NAME "ae-landy-heater-controller"
#include <OTA-Hub-diy.hpp>

long onTimeTimer = 1; // default output on time duration (10)
unsigned int timeout = 180; // seconds to run the AP/WiFi client for

unsigned int startTime = millis();

const int windscreen = 10;   // WS MOSFET
const int lMirror = 6;       // LMR MOSFET
const int rMirror = 7;       // RMR MOSFET
const int vIn = 1;           // 1
const int sysLEDPWR = 3;     // 3 NeoPixel power
const int sysLED = 4;        // 4 NeoPixel
const int onSwitch = 5;      // 5
const int numReadings = 100; // ADC samples (of inputVoltage) per poll
long previousMillis = 0;
long interval = 1000; // seconds between IO and CLI updates
int dodgySecondsCounter = 0; // keeps track of how many seconds between switch events
int checkDodgySecondsCounter = 6; // a vlaue it can't ever be, to trigger a timer reset
int neoPixelPin = sysLED;
int numPixels = 1;

// variables to control brightness levels
int brightness = 100;
int brightDirection = -10;
long onTime;                 // -1 manual/unconfigured
int stateChangeCounter = 0; // counts the on/off toggles for mode setting purposes
int readings[numReadings];
int readIndex = 0;

char voltageSource[11] = {"unknown"}; 

float inputVoltage; // device input voltage, used to know when not to enable the heater
float lastReading = 13.2; // made up this initial value

bool updateUserCLITimeout = true; // allows the CLI to be updated with the heater timout output
bool enableWiFi = false;        // stores whether or not to enable WiFI AP for the purposes of updating the firmware
bool updateRunning = false;       // stores whether or not the AP has been started
bool check_for_updates = false;    // sets a flag from the timer once per day
bool onSwitchState = 0;           // stores the on switch state, set to 1 (off)
bool eventTimerExpired = 0;       // stores the state of the event timer
bool needToSavePreferences = 0;   // stores whether or not to update preferences, for use in the main loop
bool autoTimeout = 0;             // stores the state of the autoTimeout feature
bool greenBlink = 0;              // green led blink off
bool redBlink = 0;                // red led blink off
bool blueBlink = 0;               // blue led blink off
bool enableOutputs = 0;

const float r1 = 13000.0f;                             // R1 in ohm, 13k
const float r2 = 2200.0f;                              // R2 in ohm, 2.2k
float vRefScale = (3.0f / 4096.0f) * ((r1 + r2) / r2); // gives us the voltage per LSB (0.005060369318)

long total = 0;
long loopCounter = 0;
long int t1 = 0;
long int t2 = 0;

unsigned long newtime = 0;
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 200;  // the debounce time; increase if the output flickers

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, neoPixelPin, NEO_GRB + NEO_KHZ800);
WiFiManager wm;
WiFiManagerParameter custom_heater_timeout("heaterTimeout", "Heater Timeout (0-30 mins, 0=manual enable)", "10", 2); // set the onTime in the portal according to savedparams
WiFiClientSecure wifi_client;

hw_timer_t *check_for_update = NULL;
hw_timer_t *output_enable_timer = NULL;
hw_timer_t *ap_enable_timer = NULL;

Preferences preferences;

CircularBuffer<float, 200> buffer;

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Variable to store if sending data was successful
String success;

void IRAM_ATTR onTimer0()
{ 
  check_for_updates = true; // ToDo: make this simpler
  enableWiFi = true;
}

void IRAM_ATTR onTimer1()
{                                         // output timeout
  autoTimeout = true;                     // heater timeout has timed out
  updateUserCLITimeout = true;            // allow the CLI to be updated with the timeout info
}

void disableAp()
{ // ap timeout
  enableWiFi = false;
  updateRunning = false;
  if (wm.getConfigPortalActive())
  {
    wm.stopConfigPortal();
  }
  if (wm.getWebPortalActive())
  {
    wm.stopWebPortal();
  }
}

void switchEvent()
{
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (onSwitchState)
    {
      stateChangeCounter++;
    }
  }
  lastDebounceTime = millis();
}

void pulseLED()
{
  strip.setPixelColor(0, 0, 0, 255); // set pixel to blue
  for (int i = 0; i < 10; i = i + 1)
  {   
    brightness = brightness + brightDirection;
    if (brightness < 1)
    {
      brightness = 0;
      brightDirection = -brightDirection;
    }
    else if (brightness > 99)
    {
      brightness = 100;
      brightDirection = -brightDirection;
    }
    delay(5);
    strip.setBrightness(brightness);
    strip.show();
  }
}

long smooth()
{
  // perform average on sensor readings
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

  float avg = 0.0;

  buffer.push(smooth() * (vRefScale)); // fill the circular buffer for super smooth values
  using index_t = decltype(buffer)::index_t;
  for (index_t i = 0; i < buffer.size(); i++)
  {
    avg += buffer[i] / buffer.size();
  }
  inputVoltage = avg;

  if (inputVoltage < 13.5)
  {
    strcpy(voltageSource, "battery");
  }
  else
  {
    strcpy(voltageSource, "alternator");
  }
}

void saveParamsCallback()
{
  onTime = atoi(custom_heater_timeout.getValue());
  if (onTime > 30)
  {
    onTime = 30;
  }
  else if (onTime < 0)
  {
    onTime = 0;
  }

  onTime = onTimeTimer;
  needToSavePreferences = true;
  timerAlarmDisable(output_enable_timer);
  timerAlarmWrite(output_enable_timer, onTimeTimer * 60000000, false); // set in portal, 10 by defult
  timerAlarmEnable(output_enable_timer);
}

void factoryReset()
{
  nvs_flash_erase(); // erase the NVS partition.
  nvs_flash_init();  // initialize the NVS partition.
  delay(500);
  ESP.restart(); // reset to clear memory
}

void savePreferences()
{
  Serial.println("saving settings to flash");
  preferences.begin("ae-landy-heater", false);
  preferences.putInt("onTime", onTime);
  preferences.end();
  needToSavePreferences = false;
}

void processEventData()
{
  Serial.printf("Switch has been set high %d times\n", stateChangeCounter);
  Serial.printf("Seconds since switch event (ModeResetCounter) = %i \n\n", t2 - t1);

  if (stateChangeCounter == 1)
  {
    autoTimeout = false; // allows the switch to re-arm
    Serial.println("Switch re-armed!\n");
  }
  else if (stateChangeCounter == 3)
  {
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    enableWiFi = true;    // enable WiFi AP
    Serial.println("AP enabled for configuration!\n");
  }
  else if (stateChangeCounter > 4)
  {
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    Serial.println("Factory reset in 3  2  1!\n");
    wm.resetSettings();
    factoryReset();
  }
  eventTimerExpired = false;
}

void OTACheck()
{
  // turn everthing off
  digitalWrite(lMirror, HIGH);    // high is off
  digitalWrite(rMirror, HIGH);    // high is off
  digitalWrite(windscreen, HIGH); // high is off

  // Initialise OTA
  wifi_client.setCACert(OTAGH_CA_CERT); // Set the api.github.cm SSL cert on the WiFiSecure modem
  OTA::init(wifi_client);

  // Check OTA for updates
  OTA::UpdateObject details = OTA::isUpdateAvailable();
  details.print();
  if (OTA::NO_UPDATE != details.condition)
  {
      Serial.println("An update is available!");
      // Perform OTA update
      OTA::InstallCondition result = OTA::performUpdate(&details);
      // GitHub hosts files on different server, so we have to follow the redirect, unfortunately.
      if (result == OTA::REDIRECT_REQUIRED)
      {
        wifi_client.setCACert(OTAGH_REDIRECT_CA_CERT); // Now set the objects.githubusercontent.com SSL cert
        OTA::continueRedirect(&details);               // Follow the redirect and performUpdate.
      }
  }
  else
  {
    Serial.println("No new update available. Continuing...");
  }
  check_for_updates = false;
}

void doWiFiManager()
{
  if ((millis() - startTime) > (timeout * 1000))
  {
    Serial.println("portaltimeout");
    disableAp();
    check_for_updates = false;
    enableWiFi = false;
  } 
  else
  {
    if (check_for_updates)
    {
      if (wm.autoConnect(ssid))
      {
        Serial.println("Updating firmware, switch off heaters...");
        OTACheck(); // only runs if WiFi client os configured and connected
      }
    }
  }
}

void loadPreferences()
{
  if (preferences.begin("ae-landy-heater", false))
  {
    if (preferences.getBool("configured"))
    {
      onTime = preferences.getInt("onTime");
      Serial.print("Device has been configured!");
      Serial.printf("Device is running in auto mode and on for %i minutes", onTime);
    }
    else
    {
      preferences.putInt("onTime", 10);
      onTime = preferences.getInt("onTime");
      Serial.printf("Setting onTime to %i minutes", onTime);
      preferences.putBool("configured", true);
    }
    // set WiFiManager value
    char _cstrOnTime[8];
    itoa(onTime, _cstrOnTime, 10);
    custom_heater_timeout.setValue(_cstrOnTime, 2);
  }
  preferences.end();
}

void setup()
{
  // initialise Serial for debugging, uart over USB
  Serial.begin(115200);

  // initialise IO
  pinMode(windscreen, OUTPUT);
  pinMode(lMirror, OUTPUT);
  pinMode(rMirror, OUTPUT);
  pinMode(vIn, INPUT);
  pinMode(sysLEDPWR, OUTPUT);
  digitalWrite(sysLEDPWR, HIGH);
  pinMode(onSwitch, INPUT_PULLUP);

  strip.begin();                     // initialize the strip
  strip.clear();                     // Initialize all pixels to 'off'
  strip.setPixelColor(0, 0, 0, 255); // set pixel to blue
  strip.setBrightness(brightness);
  strip.show();

  // set outputs to off (high = off)
  digitalWrite(windscreen, HIGH);
  digitalWrite(lMirror, HIGH);
  digitalWrite(rMirror, HIGH);

  // load configured settings from NVRAM
  loadPreferences();
  newtime = millis();
  
  // detect switch events
  attachInterrupt(digitalPinToInterrupt(onSwitch), switchEvent, CHANGE);

  // setup state change counter wipe, to timout enter settings changes/modes
  check_for_update = timerBegin(0, 80, true);
  timerAttachInterrupt(check_for_update, &onTimer0, true);
  timerAlarmWrite(check_for_update, 86400000000, true); // 24 hours, auto reloads  
  timerAlarmEnable(check_for_update); // Enable the timer


  if (onTime > 0) // checks to see if the hardware is configured
  { 
    onTimeTimer = onTime;
  }

  output_enable_timer = timerBegin(1, 80, true); 
  timerAttachInterrupt(output_enable_timer, &onTimer1, true);
  timerAlarmWrite(output_enable_timer, onTimeTimer * 60000000, true); // set in portal, 30 by defult

  Serial.println("\n\nSetup done");
}

// the loop function runs over and over again forever
void loop()
{
  int _avgVal = 0;
  for (int i = 0; i < 10; i = i + 1)
  {                                    // reads ten values
    _avgVal += !digitalRead(onSwitch); // read value from switch input
    delay(5);
  }
  if (_avgVal > 5)
  {
    onSwitchState = 1; // logic is inverted in hardware
  }
  else
  {
    onSwitchState = 0; // logic is inverted in hardware
  }

  t2 = millis() / 1000; // keep track of time since x

  wm.process(); // do processing

  // check the input voltage, control the heaters
  checkVoltage();

  // save preferences, if required
  if (needToSavePreferences)
  {
    savePreferences();
  }

  if (eventTimerExpired)
  {
    Serial.printf("Input wait timer expired!\n");
    processEventData();
    Serial.println("Hard resetting loop counters and states: Settings timeout!\n");
    stateChangeCounter = 0;
  }

  if (debug_mode)
  {
    enableWiFi = true;
  }

  if (enableWiFi)
  {
    pulseLED();
    if (updateRunning)
    {
      doWiFiManager();
    }
    else
    {
      WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
      wm.addParameter(&custom_heater_timeout);
      wm.setSaveParamsCallback(saveParamsCallback);
      wm.setConfigPortalBlocking(false);

      startTime = millis(); // keeps track of whn the AP was enabled, to timout after 3 minutes

      // automatically connect using saved credentials, if they exist
      // If connection fails, starts an access point with the specified name
      if (wm.autoConnect(ssid))
      {
        Serial.println("Connected to WiFi, as per the SSID that was selected in the config portal!");
        if (check_for_updates)
        {
          Serial.println("Updating firmware, switch off heaters...");
          OTACheck(); // only runs if WiFi client os configured and connected
        }
      }
      else
      {
        Serial.println("Configportal running...");
      }
      updateRunning = true;
    }
  }

  if ((autoTimeout) && (updateUserCLITimeout))
  {
    updateUserCLITimeout = false;
    digitalWrite(windscreen, HIGH);         // high is off
    digitalWrite(lMirror, HIGH);            // high is off
    digitalWrite(rMirror, HIGH);            // high is off
    timerWrite(output_enable_timer, 0);
    timerAlarmDisable(output_enable_timer); // disable the output timeout timer
    enableOutputs = 0;
    onSwitchState = 0; // 1 is off
    Serial.println("Auto timeout has switched off outputs...");
  }

  // simple timer to send serial prints at less than the speed of light
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) // roughly one second between loops
  {
    dodgySecondsCounter++;
    // deal with the startup delay for voltage normalilsation (20 seconds), to avoid turning the outputs on unnecessarily
    if ((buffer.size() > 199) && ((millis() - newtime) > 20000 ))
    {
      if ((inputVoltage - lastReading) > 0.2) // if current reading is xV greater than the last, assume the car has started
      {
        if (inputVoltage > 10) // plucked 10V, stops it from trying to drive the outputs of USB-C etc.
        {
          enableOutputs = true;
          autoTimeout = false;
          timerWrite(output_enable_timer, 0);
          timerAlarmEnable(output_enable_timer); // enable the output timeout timer
          Serial.println("Engine start detected!");
        }
      } 
    }
    lastReading = inputVoltage;

    if (inputVoltage < onVoltage * .99)
    { // battery is flat- problem...
      Serial.printf("Input voltage too low to turn on heater: %0.2fV!\n", inputVoltage);
      digitalWrite(windscreen, HIGH); // high is off
      digitalWrite(lMirror, HIGH);
      digitalWrite(rMirror, HIGH);
      timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
      enableOutputs = 0;
      onSwitchState = 0; // 1 is off

      if ((onSwitchState) || (enableOutputs)) 
      {
        if (!enableWiFi)
        {
          strip.setPixelColor(0, 255, 0, 0); // red
        }
      }
      else
      {
        if (!redBlink)
        {
          if (!enableWiFi)
          {
            strip.setPixelColor(0, 255, 0, 0); // red
            redBlink = true;
          }
        }
        else
        {
          if (!enableWiFi)
          {
            redBlink = false;
            strip.clear();
          }
        }
        brightness = 5; // fully red and dull
        if ((enableWiFi) && (wm.getConfigPortalActive()))
        {
          if (!enableWiFi)
          {
            strip.setPixelColor(0, 255, 255, 255); // set pixel to blue
            strip.setBrightness(15);
          }
        }
      }
    }
    else if (inputVoltage >= onVoltage)
    {
      if ((onSwitchState) || (enableOutputs))
      {
        if (onSwitchState)
        {
          Serial.println("Switch is on");
        }
        if (!enableWiFi)
        {
          strip.setPixelColor(0, 0, 255, 0); // green
          strip.setBrightness(5);
        }
        t1 = millis() / 1000;                // used for: seconds since switch was flicked
        
        dodgySecondsCounter = 0; // reset the switch input timout

        if (buffer.size() > 199)
        {
          Serial.println("Voltage reading is valid!");
          if (!autoTimeout)
          { 
            Serial.println("Heater is on");
            if ((!enableOutputs) && ((millis() - lastDebounceTime) > 5))
            {
              digitalWrite(lMirror, LOW);            // high is off
              digitalWrite(rMirror, LOW);            // high is off
              digitalWrite(windscreen, LOW);         // high is off
              
              if (checkDodgySecondsCounter != dodgySecondsCounter) // second counter stops if switch is on, only resets timer once
              {
                timerWrite(output_enable_timer, 0);
                timerAlarmEnable(output_enable_timer); // enable the output timeout timer
              }  
              checkDodgySecondsCounter = dodgySecondsCounter;
              Serial.println("Heaters switched on manually, auto-timout is set!");
            }
            else
            {
              digitalWrite(lMirror, LOW);            // high is off
              digitalWrite(rMirror, LOW);            // high is off
              digitalWrite(windscreen, LOW);         // high is off
              timerWrite(output_enable_timer, 0);
              timerAlarmEnable(output_enable_timer); // enable the output timeout timer
              Serial.println("Heaters switched on automatically, auto-timout is set!");
            }
            Serial.printf("Outputs enabled for a further %0.2fs of %i seconds!\n", (onTimeTimer*60 - (timerReadSeconds(output_enable_timer))), onTimeTimer*60);
          }
          else
          {
            digitalWrite(windscreen, HIGH);         // high is off
            digitalWrite(lMirror, HIGH);            // high is off
            digitalWrite(rMirror, HIGH);            // high is off
            Serial.printf("Timer has expired, disabliing outputs...\n");
          }
        }
        else
        {
          Serial.println("Waiting for the input voltage to stabilise...");
        }
      }
      else
      {
        if (eventTimerExpired)
        {
          digitalWrite(windscreen, HIGH);         // high is off
          digitalWrite(lMirror, HIGH);            // high is off
          digitalWrite(rMirror, HIGH);            // high is off
        }
        Serial.printf("System waiting, switch is off, running from %s: %0.2fV!\n", voltageSource, inputVoltage); 
        
        if (!greenBlink)
        {
          if (!enableWiFi)
          {
          strip.setPixelColor(0, 0, 255, 0); // green
          greenBlink = true;
          }
        }
        else
        {
          greenBlink = false;
          strip.clear();
        }
      }
    }
    
    if (!enableWiFi)
    {
      strip.setBrightness(5);
    }
    if ((enableWiFi) && (wm.getConfigPortalActive()))
    {
      if (!enableWiFi)
      {
        strip.setPixelColor(0, 255, 255, 255); // set pixel to blue
        strip.setBrightness(15);
      }
    }
    previousMillis = currentMillis;
  }
  strip.show();

  // keeps the maximum loops/seconds tracker limited to 4 or less
  if (dodgySecondsCounter > 4)
  {
    dodgySecondsCounter = 0;
    if (stateChangeCounter > 0)
    {
      // switch input timeout, process the switch on count
      Serial.println("Processing switch state...");
      eventTimerExpired = true;
    }
  }
}