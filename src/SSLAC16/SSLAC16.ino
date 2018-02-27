#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FS.h>

extern "C" {
#include <sntp.h>
}

//TODO:
//1. Remove death code
//2. Fix text formatting
//3. Remove isMaster and isSlave
// ...
//N. Add hardware disabling of softAP

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
IPAddress broadcast;

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

WiFiClientSecure client;

void setup(void) {
  delay(5000);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Start setup");
  Serial.println(__DATE__);
  Serial.println(__TIME__);
  
  setupAll();

  Serial.println("Setup complete");
}



void setupAll {

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  SPIFFS.begin();
  delay(200);
  readall();
  //вместо скрытия пароля сделать отключение точки доступа
  if (digitalRead(0) == 0) isHidePassword = 0;
  start_softAP();
  if (esp_hostname == "") esp_hostname = "SSLAC";
  Wire.begin(pSDA, pSCL);
  delay(1000);
  check_RTC();
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) //Serial.println(F("PCA9685 not Found!!!"));
    isPCA = 1;
  else {
    isPCA = 0;
    Serial.println("PCA9685 not found !!!");
  }
  pwm = Adafruit_PWMServoDriver();
  pwm.begin();
  pwm.setPWMFreq(pwmFreq);

  WiFi.mode(WIFI_AP_STA);
  sensors.begin();
  scanDS18x20();
  byte n = WiFi.scanNetworks();
  if (found_WIFI(ssid) ) {

    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      tmp_a++;
      isConn = true;
      if (tmp_a > 128) {
        isConn = false;
        break;
      }
    }
  }
  server.onNotFound(handleNotFound);
  server.on("/reboot", reboot);
  server.on("/sv", saveall);
  server.on("/get", _get);
  server.on("/set", _set);
  server.on("/rescue", HTTP_GET, []() {
    server.send(200, text_html, F("<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\"> <input\
        name=\"myfile1\" size=\"20\" lang=\"en\" type=\"file\"> <input value=\"Upload\" type=\"submit\"></form>"));
  });
  //***********************************************************************************
  server.onFileUpload([]() {
    if (server.uri() != "/upload") return;
    for (byte i = 0; i < server.args(); i++) {
    }
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {

      Serial.setDebugOutput(true);

      if (upload.filename.endsWith(".spiffs.bin")) {
        isSPIFFS = true;
      }
      if ((upload.filename.endsWith(".bin")) and (upload.filename.startsWith("0x0"))) {
        isFirmware = true;
      }
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (isSPIFFS) {
        SPIFFS.format();
        if (!Update.begin(maxSketchSpace, U_SPIFFS)) { //start with max available size
          Update.printError(Serial);
        }
      }
      if (isFirmware) {
        if (ESP.getFlashChipRealSize() != 4194304) return;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      }
      if (!isFirmware and !isSPIFFS) {
        String filename = upload.filename;
        if (filename[0] != '/') filename = "/" + filename;

        if (filename == "/") return;
        fsUploadFile = SPIFFS.open(filename, "w");
        if (!fsUploadFile) //Serial.println("error "+filename);
          filename = String();
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (isFirmware || isSPIFFS) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else {
        if (fsUploadFile)
          fsUploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (isFirmware || isSPIFFS) {
        if (Update.end(true)) { //true to set the size to the current progress
        } else {
          Update.printError(Serial);
        }
      } else {
        if (fsUploadFile) fsUploadFile.close(); //else //Serial.println("error "+upload.filename);
        server.serveStatic(upload.filename.c_str(), SPIFFS, upload.filename.c_str(), "max-age=86400");
      }

      Serial.setDebugOutput(false);
    }
    yield();
  });
  server.on("/upload", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    if (isFirmware || isSPIFFS) {
      server.send(200,  text_html, F("<meta http-equiv=\"refresh\" content=\"0; URL='/reboot'\" />\n\r<script>alert('Need to reboot');</script>"));
      ESP.reset();
    } else server.send(200,  text_html, F("<meta http-equiv=\"refresh\" content=\"0; URL='/'\" />\n\r<script>alert('File uploaded');</script>"));
  });

  //***********************************************************************************
  server.begin();
  for (byte i = 0; i < 32; i++) {
    if (MDNS.begin(esp_hostname.c_str())) {
      MDNS.addService("http", "tcp", 80);
      break;
    }
  }

  if (isRTC != 0) {
    if (isRTC == 1) ds1307RTC.read(tm);
    if (isRTC == 2) readPCF8563(tm);

  }// else Serial.println(F("Time may be wrong !!!"));
  
  delay(1000);
  Dir dir = SPIFFS.openDir("/");
  server.serveStatic("/", SPIFFS, "/root_page.html");
  File file_gz;
  while (dir.next()) {
    if (dir.fileName() != "/root_page.html") {


      if (dir.fileName().endsWith(".gz")) {
        file_gz = SPIFFS.open(dir.fileName(), "r");
        if (file_gz) { //Serial.println ("Error open file GZ");
          server.streamFile(file_gz, "application/x-gzip");
          file_gz.close();
        }
      }
      else
        server.serveStatic(dir.fileName().c_str(), SPIFFS, dir.fileName().c_str());

      Serial.print(F("HTTP server serving file: ")); Serial.print(dir.fileName()); Serial.print(F(", size: ")); Serial.println(dir.fileSize());
    }
  }

  isConn = WiFi.localIP() != 0;

  Serial.println("SSLAC16 ver" + String(version[0]) + "." + String(version[1]) + "rev" + String(version[2]));


  if (WiFi.localIP() != 0) {

    //------------------------------------------
    IPAddress tmp_ip;
    for (byte j = 0; j < 4; j++) {
      for (byte i = 0; i < 8; i++) {
        if (bitRead(WiFi.subnetMask()[j], i) == 0) bitWrite(broadcast[j], i, 1); else bitWrite(broadcast[j], i, 0);
      }

      broadcast[j] = WiFi.localIP()[j] | broadcast[j];
    }
    //------------------------------------------

  } else {
    broadcast = WiFi.softAPIP();
    broadcast[3] = 255;
  }
  _millis = millis();
  configTime((Time_Zone - 127) * 360, 0, "pool.ntp.org", "time.nist.gov");
  if (isConn) {
    byte i = 0;
    while ((sntp_get_current_timestamp() == 0) and (i < 32)) {
      delay(1000);
      Serial.print(".");
      i++;
    }
    if (sntp_get_current_timestamp() != 0) {
      tm.Hour = (hour(sntp_get_current_timestamp()));
      tm.Minute = (minute(sntp_get_current_timestamp()));
      tm.Second = (second(sntp_get_current_timestamp()));
      tm.Day = (day(sntp_get_current_timestamp()));
      tm.Month = (month(sntp_get_current_timestamp()));
      tm.Year = (year(sntp_get_current_timestamp()));
      msCurrent = (tm.Hour * 3600 + tm.Minute * 60 + tm.Second) * 1000;
      is_time_set = 1;
      setTimeRTC();
      Serial.println();
    }
  }
  for (byte i = 0; i < 16; i++) {
    int _ms = (tm.Hour * 60 + tm.Minute) * 60 + tm.Second;
    if (newCh[i].type == 0) {
      newCurrent[i] = getPWM(i, _ms * 1000);
      invPWM(i, newCurrent[i]);
    }
  }

  Serial.println(msStart);
}


void loop(void) {
  if (Current_ch == 16) 
    Current_ch = 0;

  delay(1);
  ticker();
  
  server.handleClient();
  
  if ((digitalRead(0) == 0) and (isPressed == 0)) {
    isPressed = 1;
  }
  
  if ((digitalRead(0) == 1) and (isPressed == 1)) {
    isPressed = 0;
    EmLight = EmLight == 0 ? 1 : 0;
  }

  setChannelPWM(Current_ch);
  Current_ch++;
}




