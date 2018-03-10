byte readByte(int &addr)
{
  byte res = EEPROM.read(addr);
  addr++;
  return res;  
}

void writeByte(int &addr, byte val)
{
  EEPROM.write(addr, val);
  addr++;
}

int readEmLight(int address) 
{
  byte _buff[sizeof emLight];
  int i;
  for (i = 0; i < sizeof emLight; i++) 
  {
    _buff[i] = EEPROM.read(address);
    address++;
  }
  memcpy(emLight, _buff, sizeof emLight);
  printToSerial(LOG_LEVEL_DEBUG, "readEmligth = %d", address);
  return address;
}

int saveEmLight(int address) 
{
  byte _buff[sizeof emLight];
  memcpy(_buff, emLight, sizeof emLight);
  int i;
  for (i = 0; i < sizeof emLight; i++) 
  {
    EEPROM.write(address, _buff[i]);
    yield();
    address++;
  }
  return address;
}

int readNewAlarm(int address) 
{
  byte _buff[sizeof newAlarm];
  int i;
  for (i = 0; i < sizeof newAlarm; i++) 
  {
    _buff[i] = EEPROM.read(address);
    address++;
  }
  memcpy(newAlarm, _buff, sizeof newAlarm);
  printToSerial(LOG_LEVEL_DEBUG, "NewAlarm = %d", address);
  return address;
}

int saveNewAlarm(int address) 
{
  byte _buff[sizeof newAlarm];
  memcpy(_buff, newAlarm, sizeof newAlarm);
  int i;
  for (i = 0; i < sizeof newAlarm; i++) 
  {
    EEPROM.write(address, _buff[i]);
    yield();
    address++;
  }
  return address;
}

int readNewCh(int address) 
{
  byte _buff[sizeof newCh];
  int i;
  for (i = 0; i < sizeof newCh; i++) 
  {
    _buff[i] = EEPROM.read(address);
    address++;
  }
  memcpy(newCh, _buff, sizeof newCh);
  printToSerial(LOG_LEVEL_DEBUG, "readNewCh = %d", address);
  return address;
}

int saveNewCh(int address) 
{
  byte _buff[sizeof newCh];
  memcpy(_buff, newCh, sizeof newCh);
  int i;
  for (i = 0; i < sizeof newCh; i++) 
  {
    EEPROM.write(address, _buff[i]);
    yield();
    address++;
  }
  
  saveVersion();

  return address;
}

_ver readVersion() 
{
  _ver ver;
  ver.major = EEPROM.read(4093);
  ver.minor = EEPROM.read(4094);
  ver.rel   = EEPROM.read(4095);
  return ver;
}

void saveVersion() 
{
  EEPROM.write(4093, currentVersion.major);
  EEPROM.write(4093, currentVersion.minor);
  EEPROM.write(4093, currentVersion.rel);
}

void readAllEEPROM() 
{
  EEPROM.begin(EEPROMMaxAddr);
  
  int addr = 0;
  byte ssid_addr = 0;
  byte passwd_addr = 0;
  
  for (byte i = 0; i < 16; i++) 
  {
    int a = EEPROM.read(addr);
    int b = EEPROM.read(addr + 1);
    ch[i].Max = a * 256 + b;

    if (ch[i].Max > 4095) 
    {
      clearEEPROM();
      break;
    }
    addr += 2;
  }
  
  addr += 5;
  
  pSDA = EEPROM.read(addr);
  printToSerial(LOG_LEVEL_DEBUG, "pSDA Addr = %d", addr);
  if ((pSDA < 1) || (pSDA > 15)) 
  {
    pSDA = 4;
  }
  addr++;
  
  pSCL = EEPROM.read(addr);
  printToSerial(LOG_LEVEL_DEBUG, "pSCL Addr = %d", addr);
  if ((pSCL < 1) || (pSCL > 15)) 
  {
    pSCL = 5;
  }
  addr++;
  
  //isAlone removed
  addr++;
  
  pOneWire = EEPROM.read(addr);
  if ((pOneWire > 16) || (pOneWire == pSDA) || (pOneWire == pSCL) || (pOneWire == 0)) 
  {
    pOneWire = 13;
  }
  addr++;
  
  printToSerial(LOG_LEVEL_DEBUG, "Start old ch vlue Addr = %d", addr);
  for (byte i = 0; i < 16; i++) 
  {
    int a = EEPROM.read(addr);
    int b = EEPROM.read(addr + 1);
    ch[i].Min = a * 256 + b;
    if (ch[i].Min > ch[i].Max) 
    {
      ch[i].Min = 0;
    }
    addr += 2;
  }
  addr++;
  printToSerial(LOG_LEVEL_DEBUG, "after old ch value reading Addr = %d", addr);
  
  for (byte i = 0; i < 16; i++) 
  {
    ch[i].Inv = EEPROM.read(addr);
    if (ch[i].Inv > 1) 
    {
      ch[i].Inv = 0;
    }
    addr++;
  }
  printToSerial(LOG_LEVEL_DEBUG, "Old ch Inv Addr = %d", addr);
  
  int a = EEPROM.read(addr);
  int b = EEPROM.read(addr + 1);
  pwmFreq = a * 256 + b;
  if ((pwmFreq > 1500) or (pwmFreq < 40)) 
  {
    pwmFreq = 1000;
  }
  addr += 2; 
  printToSerial(LOG_LEVEL_DEBUG, "pwmFreq Addr = %d", addr);
  
  timeZone = EEPROM.read(addr);
  printToSerial(LOG_LEVEL_DEBUG, "TimeZone end Addr = %d", addr);
  
  printToSerial(LOG_LEVEL_DEBUG, "ds18x20 start Addr = %d", addr);
  addr = rDS18x20(100);
  printToSerial(LOG_LEVEL_DEBUG, "DS18x20 end Addr = %d", addr);
  
  ssid_addr = EEPROM.read(447);
  printToSerial(LOG_LEVEL_DEBUG, "ssid Addr = %d", addr);
  
  ssid = "";
  if ((ssid_addr < 32) and (ssid_addr > 0)) 
  {
    for (addr = 448; addr < 448 + ssid_addr; addr++) 
    {
      ssid += char(EEPROM.read(addr));
    }
  }
  
  passwd_addr = EEPROM.read(479);
  printToSerial(LOG_LEVEL_DEBUG, "password Addr = %d", addr);

  password = "";
  if ((passwd_addr < 32) and (passwd_addr > 0)) 
  {
    for (addr = 480; addr < 480 + passwd_addr; addr++) 
    {
      password += char(EEPROM.read(addr));
    }
  }
  addr = 576;
  
  printToSerial(LOG_LEVEL_DEBUG, "Group begin Addr = %d", addr);
  addr = readGroup(addr);
  printToSerial(LOG_LEVEL_DEBUG, "Group end Addr = %d", addr);
  
  printToSerial(LOG_LEVEL_DEBUG, "Alarm begin Addr = %d", addr);
  addr = 864;
  printToSerial(LOG_LEVEL_DEBUG, "Alarm end Addr = %d", addr);
  
  printToSerial(LOG_LEVEL_DEBUG,"EmLight begin Addr = %d", addr);
  addr = readEmLight(addr);
  printToSerial(LOG_LEVEL_DEBUG,"EmLight end Addr = %d", addr);
  
  addr = 1024;
  
  printToSerial(LOG_LEVEL_DEBUG, "old chanell begin schedule Addr = %d", addr);
  for (byte i = 0; i < 16; i++) 
  {
    ch[i].Sunrise.Hour = EEPROM.read(addr); 
    addr++; 
    ch[i].Sunrise.Minute = EEPROM.read(addr); 
    addr++;
    
    ch[i].Day.Hour = EEPROM.read(addr); 
    addr++; 
    ch[i].Day.Minute = EEPROM.read(addr); 
    addr++;
    
    ch[i].Sunset.Hour = EEPROM.read(addr); 
    addr++; 
    ch[i].Sunset.Minute = EEPROM.read(addr); 
    addr++;
    
    ch[i].Night.Hour = EEPROM.read(addr); 
    addr++; 
    ch[i].Night.Minute = EEPROM.read(addr); 
    addr++;
    
    yield();
  }
  addr++;
  printToSerial(LOG_LEVEL_DEBUG, "old chanell end schedule Addr = %d", addr);
 
  for (byte j = 0; j < 16; j++) 
  {
    for (byte i = 0; i < 16; i++) 
    {
      ch[j].Desc[i] = EEPROM.read(addr);
      addr++;
    }
  }
  printToSerial(LOG_LEVEL_DEBUG, "end of old ch description Addr = %d ", addr);
  
  printToSerial(LOG_LEVEL_DEBUG, "Sensors start Addr = %d", addr);
  for (byte j = 0; j < 8; j++) 
  {
    for (byte i = 0; i < 16; i++) 
    {
      Sensor[j].Desc[i] = EEPROM.read(addr);
      addr++;
    }
  }
  printToSerial(LOG_LEVEL_DEBUG, "Sensors end Addr = %d", addr);
  
  printToSerial(LOG_LEVEL_DEBUG, "Start of old ch sensor Addr = %d", addr);
  for (byte j = 0; j < 16; j++) 
  {
    for (byte i = 0; i < sizeof ch[j].ds18x20_addr; i++) 
    {
      ch[j].ds18x20_addr[i] = EEPROM.read(addr);
      addr++;
    }
  }
  printToSerial(LOG_LEVEL_DEBUG, "End of old ch sensor Addr = %d", addr);
  
  byte _length = EEPROM.read(addr);
  printToSerial(LOG_LEVEL_DEBUG, "Hostname len %d at Addr = %d", _length, addr);
  for (byte j = 0; j < _length; j++) 
  {
    addr++;
    byte i = EEPROM.read(addr);
    if ((i > 32) and (i < 127)) 
    {
      espHostname += char(i);
    }
  }
  addr++;
  
  //tAlarm removed
  addr += 3;

  
  isHidePassword = EEPROM.read(addr);
  printToSerial(LOG_LEVEL_DEBUG, "isHidePassword Addr = %d", addr);
  addr++;
   
  _ver ver = readVersion();
  if (ver.minor < 36) 
  {
    convertPWM();
  } 
  else 
  {
    addr = readNewCh(addr);
  }

  printToSerial(LOG_LEVEL_DEBUG, "Last Addr = %d", addr);

  printToSerial(LOG_LEVEL_INFO, "EEPROM read done");
}

void saveAllEEPROM() 
{
  server.send(200, TEXT_PLAIN, "\n\r");//Why here?
  EEPROM.begin(EEPROMMaxAddr);
  
  int addr = 0;
  
  for (byte i = 0; i < 16; i++) 
  {
    int a = ch[i].Max / 256;
    int b = ch[i].Max % 256;
    EEPROM.write(addr, a);
    EEPROM.write(addr + 1, b);
    addr += 2;
  }
  
  addr += 5;
  
  EEPROM.write(addr, pSDA);
  addr++;
  
  EEPROM.write(addr, pSCL);
  addr++;
  
  //isAlone removed
  addr++;
  
  EEPROM.write(addr, pOneWire);
  addr++;

  for (byte i = 0; i < 16; i++) {
    
    int a = ch[i].Min / 256;
    int b = ch[i].Min % 256;
    EEPROM.write(addr, a);
    EEPROM.write(addr + 1, b);
    addr += 2;
  }
  addr++;
  
  for (byte i = 0; i < 16; i++) 
  {
    EEPROM.write(addr, ch[i].Inv);
    addr++;
  }
  
  int a = pwmFreq / 256;
  int b = pwmFreq % 256;
  EEPROM.write(addr, a);
  EEPROM.write(addr + 1, b);
  addr += 2;
  
  EEPROM.write(addr, timeZone);
  
  sDS18x20(100);
  
  EEPROM.write(447, ssid.length());
  
  for (addr = 448; addr < 448 + ssid.length(); addr++)
  {
    EEPROM.write(addr, ssid[addr - 448]);
  }
  
  EEPROM.write(479, password.length());
  for (addr = 480; addr < 480 + password.length(); addr++)
  {
    EEPROM.write(addr, password[addr - 480]);
  }
  
  addr = 576;
  addr = saveGroup(addr);
  
  addr = 864;
  addr = saveEmLight(addr);
  
  addr = 1024;
  for (byte i = 0; i < 16; i++) 
  {
    EEPROM.write(addr, ch[i].Sunrise.Hour); addr++; EEPROM.write(addr, ch[i].Sunrise.Minute); addr++;
    EEPROM.write(addr, ch[i].Day.Hour); addr++; EEPROM.write(addr, ch[i].Day.Minute); addr++;
    EEPROM.write(addr, ch[i].Sunset.Hour); addr++; EEPROM.write(addr, ch[i].Sunset.Minute); addr++;
    EEPROM.write(addr, ch[i].Night.Hour); addr++; EEPROM.write(addr, ch[i].Night.Minute); addr++;
    yield();
  }
  addr++;
  
  for (byte j = 0; j < 16; j++) 
  {
    for (byte i = 0; i < 16; i++) 
    {
      EEPROM.write(addr, ch[j].Desc[i]);
      addr++;
    }
  }
  
  for (byte j = 0; j < 8; j++) 
  {
    for (byte i = 0; i < 16; i++) 
    {
      EEPROM.write(addr, Sensor[j].Desc[i]);
      addr++;
    }
  }
  
  for (byte j = 0; j < 16; j++) 
  {
    for (byte i = 0; i < sizeof ch[j].ds18x20_addr; i++) 
    {
      EEPROM.write(addr, ch[j].ds18x20_addr[i] );
      addr++;
    }
  }

  if (espHostname.length() < 23)
  {
    EEPROM.write(addr, espHostname.length());
  }
  addr++;
  
  for (byte j = 0; j < espHostname.length(); j++) 
  {
    EEPROM.write(addr, espHostname[j]);
    addr++;
  }
  
  //tAlarm removed
  addr+=3;
  
  EEPROM.write(addr, isHidePassword);
  addr++;
  
  addr = saveNewCh(addr);
  
  EEPROM.commit();

  printToSerial(LOG_LEVEL_INFO, "Write to EEPROM done");
}

void clearEEPROM() 
{
  EEPROM.begin(EEPROMMaxAddr);
  
  for (int i = 0; i < EEPROMMaxAddr; i++) 
  {
    EEPROM.write(i, 0);
    yield();
  }
  EEPROM.commit();

  printToSerial(LOG_LEVEL_INFO, "Clearing EEPROM done");
  
  server.sendContent(F("EEPROM cleared"));//Why here?
}

void sDS18x20(int address) 
{
  byte _buff[sizeof Sensor ];
  memcpy(buff, Sensor, sizeof Sensor);
  for (int i = 0; i < sizeof Sensor; i++) 
  {
    EEPROM.write(address + i, buff[i]);
    yield();
  }
}

int saveGroup(int address) 
{
  byte _buff[sizeof group];
  memcpy(_buff, group, sizeof group);
  int i;
  for (i = 0; i < sizeof group; i++) 
  {
    EEPROM.write(address + i, _buff[i]);
    yield();
  }
  return address + i;
}

int rDS18x20(int address) 
{
  for (int i = 0; i < sizeof Sensor; i++) 
  {
    buff[i] = EEPROM.read(address + i);
  }
  memcpy(Sensor, buff, sizeof Sensor);
  
  printToSerial(LOG_LEVEL_DEBUG, "ds1820 = %d", address + sizeof Sensor);
  
  return address + sizeof Sensor;
}

int readGroup (int address) 
{
  byte _buff[sizeof group];
  int i;
  for (i = 0; i < sizeof group; i++) 
  {
    _buff[i] = EEPROM.read(address + i);
  }
  memcpy(group, _buff, sizeof group);
  
  printToSerial(LOG_LEVEL_DEBUG, "readGroup = %d", address + i);
  
  return address + i;
}

