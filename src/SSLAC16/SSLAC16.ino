#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FS.h>

extern "C" {
#include <sntp.h>
}

//TODO: rename variables

#define MAX_RETRAY_COUNT 8

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2

#define MASTER_LOG_LEVEL LOG_LEVEL_INFO

#define TEXT_HTML  "text/html"
#define TEXT_PLAIN "text/plain"
#define TEXT_JSON  "text/json"

typedef struct _ver
{
  byte major = 0;
  byte minor = 37;
  byte rel = 0;
}; 
const _ver currentVersion;

typedef struct _shed 
{
  byte Hour = 256;
  byte Minute = 256;
};

typedef struct _newAlarm 
{
  byte index = 255;
  byte temp;
  byte step = 10;
};
_newAlarm newAlarm[8];

typedef struct _newCh 
{
  int value[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  _shed time[16];
  byte color[3];
  byte group;
  byte type;
  byte Inv;
  char name[32];
};
_newCh newCh[16];

typedef struct _group 
{
  char name[32];
  byte alarmIndex = 255;
  byte temp = 50;
  byte step = 10;
  byte isAlarm = 0;
};
_group group[8];

//TODO: remove in future versions
typedef struct _ch 
{
  int Max;
  int Min;
  int Cur;
  byte Inv;
  char Desc[16];
  _shed Sunrise;
  _shed Day;
  _shed Sunset;
  _shed Night;
  byte ds18x20_addr[8];
};
_ch ch[16];

typedef struct _ds18x20 
{
  DeviceAddress addr;
  float Temp;
  char Desc[16];
  byte index;
};
_ds18x20 Sensor[8];

unsigned long _millis = 0;

const int EEPROMMaxAddr = 4096;

byte playTime = 255;

ESP8266WebServer server(80);
char stream[1024];
bool isFirmware = false;
bool isSPIFFS = false;
File fsUploadFile;

byte CurrentCh = 0;
int newCurrent[16];
byte isEmLightOn = 0;
int emLight[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
byte isPressed = 0;

Adafruit_PWMServoDriver pwm;

DeviceAddress addr;
byte pSDA = 4;
byte pSCL = 5;
int pwmFreq = 1000;
byte isPCA = 0;

byte pOneWire = 13;
OneWire oneWire(pOneWire);
DallasTemperature sensors(&oneWire);
byte cSensor;
byte buff[sizeof Sensor];

//TODO: Add hardware disabling of softAP
byte foundedNet;
String ssid ;
String password ;
String espHostname = "";
byte isHidePassword = 0;
bool isConn = false;

byte RTCType = 0; // 0 - no RTC, 1 - DS1307, 2 - PCF8563
byte timeZone = 3;
byte isTimeSet = 1;
unsigned long msCurrent = 0;
unsigned long msStart = 0;
tmElements_t tm;

void setup(void) 
{
  delay(5000);
  Serial.begin(115200); 
  Serial.println();

  printToSerial(LOG_LEVEL_INFO, "SSLAC16 ver %d.%d.%d build %s %s", currentVersion.major, currentVersion.minor, currentVersion.rel, __DATE__, __TIME__);  
  printToSerial(LOG_LEVEL_INFO, "Start setup");

  setupTimeZone();
  initSPIFFS();
  readAllEEPROM();
  setupWiFi();
  setupWebServer();
  setupWire();
  setupDateTime();
  setupDS18x20();
  setupPWM();

  printToSerial(LOG_LEVEL_INFO, "Setup complete");
}

void initSPIFFS()
{
  SPIFFS.begin();
  printToSerial(LOG_LEVEL_INFO, "SPIFFS initialization done");
}

void setupWire()
{
  Wire.begin(pSDA, pSCL);
  printToSerial(LOG_LEVEL_INFO, "Wire initialization done");
}

void loop(void) 
{
  if (CurrentCh == 16)
  { 
    CurrentCh = 0;
  }

  delay(1);
  ticker();
  
  server.handleClient();
  
  if ((digitalRead(0) == 0) and (isPressed == 0)) 
  {
    isPressed = 1;
  }
  
  if ((digitalRead(0) == 1) and (isPressed == 1)) 
  {
    isPressed = 0;
    isEmLightOn = isEmLightOn == 0 ? 1 : 0;
  }

  setChannelPWM(CurrentCh);
  CurrentCh++;
}




