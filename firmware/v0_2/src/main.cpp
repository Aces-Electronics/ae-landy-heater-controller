#include <Arduino.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <CircularBuffer.hpp>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>

#define debug // turns ap mode on in debug

#if defined debug
  bool enable_debug = true;
  float onVoltage = 13.00; // input voltage to allow the heaters turn on
#else
  float onVoltage = 13.80; // input voltage to allow the heaters turn on
#endif

const char* ssid = "ae-update";

WiFiManager wm;
WiFiManagerParameter custom_heater_timeout("heaterTimeout", "Heater Timeout (0-30 mins)", "0", 2); // set the onTime in the portal according to savedparams

unsigned int  timeout   = 180; // seconds to run for
unsigned int  startTime = millis();
bool portalRunning      = false;
bool startAP            = false; // start AP and webserver if true, else start only webserver
bool deviceConfigured   = false;

const int windscreen = 10; // WS MOSFET
const int lMirror = 6; // LMR MOSFET
const int rMirror = 7; // RMR MOSFET
const int vIn = 1; // 1
const int sysLEDPWR = 3; // 3 NeoPixel power
const int sysLED = 4; // 4 NeoPixel
const int onSwitch = 5; // 5
const int numReadings = 100; // ADC samples (of inputVoltage) per poll
long previousMillis = 0;
long interval = 5000;
int neoPixelPin = sysLED;
int numPixels = 1;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, neoPixelPin, NEO_GRB + NEO_KHZ800);

// variables to control brightness levels
int brightness = 180; 
int brightDirection = -10;

// a pre-processor macro
#define DELAY_TIME (10)

int onTime; // -1 manual/unconfigured
int stateChangeCounter = 0; // counts the on/off toggles for mode setting purposes
int readings[numReadings]; 
int readIndex = 0;
float inputVoltage; // device input voltage, used to know when not to enable the heater

bool updateEnable = false; // stores whether or not to enable WiFI AP for the purposes of updating the firmware
bool updateRunning = false; // stores whether or not the AP has been started
bool onSwitchState = 0; // stores the on switch state, set to 1 (off) 
bool eventTimerExpired = 0; // stores the state of the event timer
bool needToSavePreferences = 0; // stores whether or not to update preferences, for use in the main loop
bool needToProcessSwitchEvents = 0; // stores whether or not to proces an interrupt driven switch event
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
unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers

hw_timer_t *clear_state_timer = NULL;
hw_timer_t *output_enable_timer = NULL;

Preferences preferences;

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
  digitalWrite(lMirror, HIGH); // high is off
  digitalWrite(rMirror, HIGH); // high is off
  timerRestart(output_enable_timer); // reset the counter for next time
  timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
  autoTimeout = true;
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

void adjustBrightness() {
  brightness = brightness + brightDirection;
  if( brightness < 5 ) {
     brightness = 0;
     brightDirection = -brightDirection;
  }
  else if( brightness > 255 ) {
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
  // the following ensures using the right type for the index variable
  using index_t = decltype(buffer)::index_t;
  for (index_t i = 0; i < buffer.size(); i++)
  {
    avg += buffer[i] / buffer.size();
  }
  inputVoltage = avg;
}

void processSwitchEvents()
{
  if ((millis() - lastDebounceTime) > debounceDelay) 
  {
    if (onSwitchState)
    {
      stateChangeCounter++;
      Serial.println("Looks like switch is on...");
      if (inputVoltage >= 13.80) // I've plucked 13.8v from fat air
      {
        if (buffer.size() > 199)
        {
          //logic is reversed, 0/false is on
          if (!autoTimeout) //timer not expired
          {
            Serial.printf("Auto mode timer running for %i minutes...\n", onTime);
            digitalWrite(lMirror, LOW); // high is off
            digitalWrite(rMirror, LOW); // high is off
            digitalWrite(windscreen, LOW); // high is off
            timerAlarmEnable(output_enable_timer); // enable the output timeout timer
          }
        }
        else
        {
          Serial.println("Waiting for the input voltage to stabilise...");
        }
      }
    }
    else
    {
      Serial.println("Looks like switch is off...");
      // turn off the outputs
      digitalWrite(windscreen, HIGH); // high is off
      digitalWrite(lMirror, HIGH); // high is off
      digitalWrite(rMirror, HIGH); // high is off
      timerRestart(output_enable_timer); // reset the counter for next time
      timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
      Serial.println("Outputs and timer disabled!");
    }
  }
  lastDebounceTime = millis();
  needToProcessSwitchEvents = false;
}

 void saveParamsCallback () {
  onTime = atoi (custom_heater_timeout.getValue());
  if (onTime > 30) {
    onTime = 30;
  } else if (onTime < 0)
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
  Serial.printf("ModeResetCounter = %i \n\n", t2-t1);
  
  if ((stateChangeCounter == 3) && (inputVoltage < onVoltage))
  {
    stateChangeCounter = 0; // reset counter to 0 for the second loop
    updateEnable = true; // enable WiFi AP
    Serial.println("AP enabled for firmware update!\n");
    timerAlarmDisable(clear_state_timer);
  }
  eventTimerExpired = false;
}

// Turns all the NeoPixels off
void allOff() {
  strip.clear();    // this is a simpler way to turn all the pixels off
  strip.show();
}

// the activate function will set the pixel color, change the brightness level
// and have a small delay
void activateLEDs() {   
  strip.setPixelColor(0, 0,0,255);   
  strip.setBrightness(brightness);  
  strip.show();
}

void doWiFiManager(){
  // is auto timeout portal running
  if(portalRunning){

    // check for timeout
    if((millis()-startTime) > (timeout*1000)){
      Serial.println("portaltimeout");
      portalRunning = false;
      if(startAP){
        wm.stopConfigPortal();
      }
      else{
        wm.stopWebPortal();
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
    }else
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

void setup() {
  // initialise Serial for debugging, uart over USB
  Serial.begin(115200);

  // initialise digital pins as an output.
  pinMode(windscreen, OUTPUT);
  pinMode(lMirror, OUTPUT);
  pinMode(rMirror, OUTPUT);
  pinMode(vIn, INPUT);
  pinMode(sysLEDPWR, OUTPUT);
  digitalWrite(sysLEDPWR, HIGH);
  pinMode(onSwitch, INPUT_PULLUP);

  strip.begin();  // initialize the strip
  strip.show();   // make sure it is visible
  strip.clear();  // Initialize all pixels to 'off'

  activateLEDs(); // show stet via NEOPixel

  // set outputs to off (high = off)
  digitalWrite(windscreen, HIGH);
  digitalWrite(lMirror, HIGH);
  digitalWrite(rMirror, HIGH);

  loadPreferences();
  newtime = millis();

  // detect switch events
  attachInterrupt(digitalPinToInterrupt(onSwitch), switchEvent, FALLING);

  // setup state change counter wipe, to timout enter settings changes/modes
  clear_state_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(clear_state_timer, &onTimer1, true);
  timerAlarmWrite(clear_state_timer, 5000000, true); // 5 seconds

  output_enable_timer = timerBegin(1, 80, true);
  timerAttachInterrupt(output_enable_timer, &onTimer2, true);
  timerAlarmWrite(output_enable_timer, 600000000, true); // 10 minutes

  //ToDo: read and set the timer
  newtime = millis();
  Serial.println("\n\nSetup done");
}

// the loop function runs over and over again forever
void loop() 
{
  t2 = millis();

  wm.process(); // do processing

  if (needToProcessSwitchEvents)
  {
    onSwitchState = !digitalRead(onSwitch); // logic is inverted
    processSwitchEvents();
  }  

  // check the input voltage, control the heaters
  checkVoltage();

  // save preferences, if required
  if (needToSavePreferences)
  {
    savePreferences();
  }

  if (eventTimerExpired)
  {
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
      //wm.resetSettings();
      Serial.println("Updating firmware, switch off heaters...");
      digitalWrite(lMirror, HIGH); // high is off
      digitalWrite(rMirror, HIGH); // high is off
      digitalWrite(windscreen, HIGH); // high is off

      WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
      wm.addParameter(&custom_heater_timeout);
      wm.setSaveParamsCallback(saveParamsCallback);

      //wm.resetSettings();
      wm.setConfigPortalBlocking(false);
      //automatically connect using saved credentials if they exist
      //If connection fails it starts an access point with the specified name
      if(wm.autoConnect(ssid)){
          Serial.println("connected...yeey :)");
      }
      else {
          Serial.println("Configportal running");
      }
        }
        updateRunning = true; // ToDO: time this out
      }

  static char inputBuffer[MAX_MESSAGE];
  static unsigned char index = 0;

  while (Serial.available() > 0) {
    serial_input_char = Serial.read();
    if (serial_input_char == '\r') {
      Serial.print("You entered: ");
      Serial.println(inputBuffer);
      inputBuffer[0] = 0;
      index = 0;
    } else {        
      if (index < MAX_MESSAGE-1) {
        inputBuffer[index++] = serial_input_char;
        inputBuffer[index] = 0;
      }
    }
  } 

  if (autoTimeout)
  {
    Serial.println("Auto timeout has switched off outputs...");
  }

  if ((t2-t1 > 10000) && (eventTimerExpired)) // timeout mode setting after 10 seconds
  {
    stateChangeCounter = 0;
    Serial.println("Hard resetting loop counters and states: Settings timeout!\n");
    t1 = millis(); // reset the timer for the third loop, save settings
    timerAlarmDisable(clear_state_timer);
  }

  if (inputVoltage < (onVoltage * .99)) // I've plucked 13v from fat air
  {
    digitalWrite(windscreen, HIGH); // high is off
    digitalWrite(lMirror, HIGH);
    digitalWrite(rMirror, HIGH);
    timerRestart(output_enable_timer); // reset the counter for next time
    timerAlarmDisable(output_enable_timer); // disable the  output timeout timer
    strip.setPixelColor(0, 255,0,0);
    brightness = 5;  // fully red and dull
    
  }
  else if (inputVoltage >= onVoltage)
  {
    strip.setPixelColor(0,0,255,0); // green 
    brightness = 5;
  }

  // simple timer to send serial prints at less than the speed of light
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval) { // ToDo: merge this code with the above for a simpler if
    if (inputVoltage < onVoltage * .99) // battery is flat- problem...
    {
      Serial.printf("Input voltage too low to turn on heater: %0.2fV!\n", inputVoltage);
      Serial.println("Outputs and timer disabled!\n");
    } 
    else if (inputVoltage >= onVoltage)
    {
      Serial.printf("System normal, running from the alternator: %0.2fV!\n", inputVoltage);
    }
    previousMillis = currentMillis;
  }

  strip.setBrightness(brightness);  
  strip.show();
  
  // delay for the purposes of debouncing the switch and allowing the NeoPixel to cycle colour/brightness
  delay(DELAY_TIME);
}