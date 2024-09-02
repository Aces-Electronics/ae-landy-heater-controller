//////
// How to read the LED on the JonoTron Hardware:
// Flashing red:
//   Battery voltage too low, switch off and auto-mode off, heaters off.
// Flashing green:
//   Battery voltage okay, switch off and auto-mode off, heaters off.
// Solid red:
//   Battery voltage too low, switch on or auto-mode on, heaters off.
// Solid green:
//   Battery voltage okay, switch on or auto-mode on, heaters on.
// Flash blue:
//   System booting/rebooting.
// Solid blue:
//   Hardware in setup mode, access the web interface at http://192.168.4.1 after connecting to the ae-update WiFi network.
// Solid red/green/blue combined (probably looks mostly white):
//   Software update available, if connected to a WiFi network the system will check once a day to see if there are any new
//   updates, it will signal that the update is ready, and then update. After the system updates the LED will return to the normal state.
/////

//////
// How to use the physical switch:
// Switch in the on position:
//   Manually turn on the heaters (maybe this should be the enable auto mode function, rather than manual mode on?).
// Switch in the off position:
//   System in auto mode, will run the heaters every time the car starts for the duration of the timeout set in the web portal (defaults to 10 mins).
// To turn on the config portal:
//   Pause for a second on each state change (off to on or on to off), toggle the switch on-off-on-off-on-off (turn it on three times, turn it off)
//   You can then connect a phone, laptop or tablet to the ae-update wifi network; no password required. A captive portal with some options should pop up.
//   You can also access the portal, once started, by browsing to http://192.168.4.1
//   The portal will timeout after 3 minutes.
// To factory default the device:
//   Use the same process as above but toggle the switch on 5 times, then off, so
//   on-off-on-off-on-off-on-off-on-off
//   This will set the auto mode timout to 10 mins and erase all portal/WiFi settings.
//   If you connect the device to your home network, use a factory default to regain access to the portal if you are unable to find the device's IP address, which will  no longer be 192.168.4.1, check the WiFi router's web interface for the
////

// ToDo: try to connect to wifi, if configured, once per day to check for updates: https://medium.com/@adityabangde/esp32-firmware-updates-from-github-a-simple-ota-solution-173a95f4a97b
// ToDo: detect when the car starts. take the lowest and highest voltages in the last minute and compare them? Look for a larg difference

#include <Arduino.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <CircularBuffer.hpp>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>

const char *ssid = "ae-update";
float onVoltage = 13.80; // input voltage to allow the heaters turn on

unsigned int timeout = 180; // seconds to run the AP for
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
int onTimeTimer = 30;
int neoPixelPin = sysLED;
int numPixels = 1;

// variables to control brightness levels
int brightness = 100;
int brightDirection = -10;
int onTime;                 // -1 manual/unconfigured
int stateChangeCounter = 0; // counts the on/off toggles for mode setting purposes
int readings[numReadings];
int readIndex = 0;
float inputVoltage; // device input voltage, used to know when not to enable the heater

bool updateUserCLITimeout = true; // allows the CLI to be updated with the heater timout output
bool updateEnable = false;        // stores whether or not to enable WiFI AP for the purposes of updating the firmware
bool updateRunning = false;       // stores whether or not the AP has been started
bool check_for_updates = false;    // sets a flag from the timer once per day
bool onSwitchState = 0;           // stores the on switch state, set to 1 (off)
bool eventTimerExpired = 0;       // stores the state of the event timer
bool needToSavePreferences = 0;   // stores whether or not to update preferences, for use in the main loop
bool autoTimeout = 0;             // stores the state of the autoTimeout feature
bool greenBlink = 0;              // green led blink off
bool redBlink = 0;                // red led blink off
bool blueBlink = 0;               // blue led blink off

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
WiFiManagerParameter custom_heater_timeout("heaterTimeout", "Heater Timeout (0-30 mins, 0 = manual enable)", "10", 2); // set the onTime in the portal according to savedparams

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
  check_for_updates = true;
}

void IRAM_ATTR onTimer1()
{                                         // ouput timeout
  digitalWrite(windscreen, HIGH);         // high is off
  digitalWrite(lMirror, HIGH);            // high is off
  digitalWrite(rMirror, HIGH);            // high is off
  timerRestart(output_enable_timer);      // reset the counter for next time
  timerAlarmDisable(output_enable_timer); // disable the output timeout timer
  autoTimeout = true;                     // heater timeout has timed out
  updateUserCLITimeout = true;            // allow the CLI to be updated with the timeout info
}

void disableAp()
{ // ap timeout
  updateEnable = false;
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

void adjustd()
{
  brightness = brightness + brightDirection;
  if (brightness < 5)
  {
    brightness = 0;
    brightDirection = -brightDirection;
  }
  else if (brightness > 255)
  {
    brightness = 255;
    brightDirection = -brightDirection;
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

  buffer.push(smooth() * (vRefScale * 1.01)); // fill the circular buffer for super smooth values
  using index_t = decltype(buffer)::index_t;
  for (index_t i = 0; i < buffer.size(); i++)
  {
    avg += buffer[i] / buffer.size();
  }
  inputVoltage = avg;
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

  needToSavePreferences = true;
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

  if (stateChangeCounter == 3)
  {
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    updateEnable = true;    // enable WiFi AP
    Serial.println("AP enabled for firmware update!\n");
    //timerAlarmDisable(check_for_update); 
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

void doWiFiManager()
{
  if ((millis() - startTime) > (timeout * 1000))
  {
    Serial.println("portaltimeout");
    disableAp();
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
      preferences.putInt("onTime", 0);
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

  loadPreferences();
  newtime = millis();

  // detect switch events
  attachInterrupt(digitalPinToInterrupt(onSwitch), switchEvent, CHANGE);

  // setup state change counter wipe, to timout enter settings changes/modes
  check_for_update = timerBegin(0, 80, true);
  timerAttachInterrupt(check_for_update, &onTimer0, true);
  timerAlarmWrite(check_for_update, 86400000000, true); // 24 hours, auto reloads  

  if (onTime < 1)
  { // sets the maximum on time, even in manual mode (0), to 30 mins (might be too short)
    onTimeTimer = 30;
  }
  else
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
  //
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
    Serial.printf("Input wait timer expired\n");
    processEventData();
  }

  if (updateEnable)
  {
    if (updateRunning)
    {
      doWiFiManager();
    }
    else
    {

      Serial.println("Updating firmware, switch off heaters...");
      digitalWrite(lMirror, HIGH);    // high is off
      digitalWrite(rMirror, HIGH);    // high is off
      digitalWrite(windscreen, HIGH); // high is off

      WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
      wm.addParameter(&custom_heater_timeout);
      wm.setSaveParamsCallback(saveParamsCallback);
      // wm.setBreakAfterConfig(true);
      wm.setConfigPortalBlocking(false);

      startTime = millis(); // keeps track of whn the AP was enabled, to timout after 3 minutes

      // automatically connect using saved credentials if they exist
      // If connection fails it starts an access point with the specified name
      if (wm.autoConnect(ssid))
      {
        Serial.println("Connected to WiFi, as per the SSID that was selected in the config portal!");
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
    Serial.println("Auto timeout has switched off outputs...");
  }

  if ((t2 - t1 > 10) && (stateChangeCounter > 0)) // this is used to reset counters
  { // timeout mode setting after 10 seconds
    stateChangeCounter = 0;
    Serial.println("Hard resetting loop counters and states: Settings timeout!\n");
    t1 = millis() / 1000; // reset the timer for the third loop, save settings
    //timerAlarmDisable(check_for_update); // This can be used to enable/disable the dodgy seconds counter
  }

  // simple timer to send serial prints at less than the speed of light
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) // roughly one second between loops
  {
    dodgySecondsCounter++;
    if (inputVoltage < onVoltage * .99)
    { // battery is flat- problem...
      Serial.printf("Input voltage too low to turn on heater: %0.2fV!\n", inputVoltage);
      Serial.println("Outputs and timer disabled!\n");
      digitalWrite(windscreen, HIGH); // high is off
      digitalWrite(lMirror, HIGH);
      digitalWrite(rMirror, HIGH);
      timerRestart(output_enable_timer);      // reset the counter for next time
      timerAlarmDisable(output_enable_timer); // disable the  output timeout timer

      if (onSwitchState)
      {
        strip.setPixelColor(0, 255, 0, 0); // red
      }
      else
      {
        if (!redBlink)
        {
          strip.setPixelColor(0, 255, 0, 0); // red
          redBlink = true;
        }
        else
        {
          redBlink = false;
          strip.clear();
        }
        brightness = 5; // fully red and dull
        if ((updateEnable) && (wm.getConfigPortalActive()))
        {
          strip.setPixelColor(0, 255, 255, 255); // set pixel to blue
          strip.setBrightness(15);
        }
      }
    }
    else if (inputVoltage >= onVoltage)
    { // ToDo: need a way to turn on the heater when the car is started
      if (onSwitchState)
      {
        strip.setPixelColor(0, 0, 255, 0); // green
        strip.setBrightness(5);
        t1 = millis() / 1000;                // used for: seconds since switch was flicked
        
        dodgySecondsCounter = 0; // reset the switch input timout

        Serial.println("Switch is on, heaters are on");
        if (buffer.size() > 199)
        {
          if (!autoTimeout)
          { // timer not expired
            if (onTime < 1)
            {
              Serial.printf("Heaters enabled in manual mode (auto timeout set to 0 in the Portal\n");
            }
            else
            {
              Serial.printf("Auto mode timer running for %i minutes...\n", onTime);
            }
            if (eventTimerExpired)
            {
              digitalWrite(lMirror, LOW);            // high is off
              digitalWrite(rMirror, LOW);            // high is off
              digitalWrite(windscreen, LOW);         // high is off
              timerAlarmEnable(output_enable_timer); // enable the output timeout timer
            }
          }
        }
        else
        {
          Serial.println("Waiting for the input voltage to stabilise...");
        }
      }
      else
      {
        Serial.printf("System normal, switch is off, running from alternator: %0.2fV!\n", inputVoltage);
        if (eventTimerExpired)
        {
          digitalWrite(windscreen, HIGH);         // high is off
          digitalWrite(lMirror, HIGH);            // high is off
          digitalWrite(rMirror, HIGH);            // high is off
          timerRestart(output_enable_timer);      // reset the counter for next time
          timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
        }

        if (!greenBlink)
        {
          strip.setPixelColor(0, 0, 255, 0); // green
          greenBlink = true;
        }
        else
        {
          greenBlink = false;
          strip.clear();
        }
      }
    }
    previousMillis = currentMillis;
    strip.setBrightness(5);
    if ((updateEnable) && (wm.getConfigPortalActive()))
    {
      strip.setPixelColor(0, 255, 255, 255); // set pixel to blue
      strip.setBrightness(15);
    }
  }
  strip.show();

  // keeps the maximum loops/seconds tracker limited to 4 ot less
  if (dodgySecondsCounter > 4)
  {
    dodgySecondsCounter = 0;
    if (stateChangeCounter > 0)
    {
      // switch input timeout, process the switch on count
      eventTimerExpired = true;
    }
  }
}