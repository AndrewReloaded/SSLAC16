#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>
#include <Time.h>
//FIXME
//#include <DS1307RTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FS.h>

extern "C" {
#include <sntp.h>
}

//TODO:
//Add hardware disabling of softAP

#define TEMPERATURE_PRECISION 9
#define USE_SERIAL Serial

byte version[3] = {0, 36, 1};

typedef struct _adoptation {
  byte groupK[8];
  byte days[8];
  tmElements_t startDate[8];
};
_adoptation adoptation;

typedef struct _shed {
  byte Hour = 256;
  byte Minute = 256;
};

typedef struct _newAlarm {
  byte index = 255;
  byte temp;
  byte step = 10;
};
_newAlarm newAlarm[8];

typedef struct _newCh {
  int value[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  _shed time[16];
  byte color[3];
  byte group;
  byte type;
  byte Inv;
  char name[32];
};
_newCh newCh[16];

typedef struct _pump {
  int value;
  byte index;
  byte gpio;
};
_pump Pumps[16];

typedef struct _group {
  char name[32];
  byte alarmIndex = 255;
  byte temp = 50;
  byte step = 10;
  byte isAlarm = 0;
};
_group group[8];

typedef struct _alarm_temperature {
  byte index = 255;
  byte temp = 50;
  byte step = 10;
};

typedef struct _ch {
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

typedef struct _ds18x20 {
  DeviceAddress addr;
  float Temp;
  char Desc[16];
  byte index;
};
_ds18x20 Sensor[8];

byte Current_ch = 0;
unsigned long _millis = 0;

File fsUploadFile;
const String text_html = "text/html";
const String text_plain = "text/plain";
const String text_json = "text/json";
const String htm = ".htm";
const String html = ".html";
byte playTime = 255;
int newCurrent[16];
int emLight[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
byte foundedNet;

DeviceAddress addr;
_alarm_temperature tAlarm;
byte isAlarm[8] = {0, 0, 0, 0, 0, 0, 0, 0};

bool isFirmware = false;
bool isSPIFFS = false;
bool isConn = false;
char stream[1024];
byte isRTC = 0; // 0 - no RTC, 1 - DS1307, 2 - PCF8563, 3 - both
byte isPCA = 0;
int tmp_a = 0;
const int max_addr = 4096;
byte pSDA = 4;
byte pSCL = 5;
byte pOneWire = 13;
byte channelGroup = 0;
byte isPressed = 0;

String ssid ;
String password ;
const char *ssid_ap = "";
const char *passwd_ap = "";
String esp_hostname = "";
int pwmFreq = 1000;
byte Time_Zone;
byte is_time_set = 1;
unsigned long msCurrent = 0;
unsigned long msStart = 0;
byte isFan = 0;
byte isMode = 1;
byte isAlone;
byte isSetupCh = 0;
int plusMs = 0;

byte EmLight = 0;
byte isHidePassword = 0;
tmElements_t tm;

ESP8266WebServer server(80);

Adafruit_PWMServoDriver pwm;
OneWire oneWire(13);
DallasTemperature sensors(&oneWire);
byte buff[sizeof Sensor ];
byte cSensor;

void setup(void) 
{
  delay(5000);
  Serial.begin(115200);
  
  Serial.println("");
  Serial.println("SSLAC16 ver" + String(version[0]) + "." + String(version[1]) + "rev" + String(version[2]));
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  
  Serial.println("Start setup");

  SPIFFS.begin();
  delay(200);//Why?
  Serial.println(">> SPIFFS.begin done");

  readAllEEPROM();
  Serial.println(">> readAllEEPROM done");

  setupWiFi();
  Serial.println(">> setupWiFi done");

  setupWebServer();
  Serial.println(">> setupWebServer done");

  Wire.begin(pSDA, pSCL);
  delay(1000);//Why?
  Serial.println(">> Wire.begin done");

  setupDateTime();
  Serial.println(">> setupDateTime done");
  
  setupDS18x20();
  Serial.println(">> setupDS18x20 done");

  setupPWM();
  Serial.println(">> setupPWM done");

  Serial.println("Setup complete");
}

void loop(void) 
{
  if (Current_ch == 16)
  { 
    Current_ch = 0;
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
    EmLight = EmLight == 0 ? 1 : 0;
  }

  setChannelPWM(Current_ch);
  Current_ch++;
}




