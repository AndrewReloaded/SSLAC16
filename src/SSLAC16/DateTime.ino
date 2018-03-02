#define PCF8563address 0x51
#define DS1307address 0x68

void setupDateTime()
{
  _millis = millis();
 
  checkRTC();
 
  if (isConn) 
  {   
    syncDateTimeWithSntp();
  }
  else
  {
    getRTCDateTime();
  }

  printToSerial(LOG_LEVEL_INFO, "Date and Time setup done");
}

void setupTimeZone()
{
  configTime(Time_Zone * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void checkRTC() 
{
  isRTC = 0;
  
  Wire.beginTransmission(PCF8563address);
  if (Wire.endTransmission() == 0)
  {
    isRTC += 2;
  }
  
  Wire.beginTransmission(DS1307address);
  if (Wire.endTransmission() == 0)
  {
    isRTC += 1;
  }
  
  if (isRTC == 3) 
  {
    isRTC = 2;
  }
  
  if (isRTC == 0) 
  {
    printToSerial(LOG_LEVEL_WARN, "RTC not found");
  }
}

void getRTCDateTime()
{
  if (isRTC == 1) 
  {
    readDS1307(tm);
  }
  else if (isRTC == 2) 
  {
    readPCF8563(tm);
  }
}

void setRTCDateTime() 
{  
  if (isRTC == 1)
  {
    setDS1307(tm);
  }
  else if (isRTC == 2)
  {
    setPCF8563(tm);
  }
}

void syncDateTimeWithSntp()
{
  uint32 currentTimestamp = sntp_get_current_timestamp();
  if (currentTimestamp != 0) 
  {
    tm.Hour = (hour(currentTimestamp));
    tm.Minute = (minute(currentTimestamp));
    tm.Second = (second(currentTimestamp));
    tm.Day = (day(currentTimestamp));
    tm.Month = (month(currentTimestamp));
    tm.Year = (year(currentTimestamp));
    msCurrent = (tm.Hour * 3600 + tm.Minute * 60 + tm.Second) * 1000;
    
    setRTCDateTime();
    
    is_time_set = 1;

    printToSerial(LOG_LEVEL_INFO, "Date and Time are synchronized with SNTP %d:%d:%d %d.%d.%d (timestamp = %d)", tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tm.Year, currentTimestamp);   
  }
  else
  {
    printToSerial(LOG_LEVEL_WARN, "Date and Time are not synchronized with SNTP");
  }
}

byte bcdToDec(byte value) 
{
  return ((value / 16) * 10 + value % 16);
}

byte decToBcd(byte value) 
{
  return (value / 10 * 16 + value % 10);
}

void setPCF8563(tmElements_t &aTm) 
{
  // this sets the time and date to the PCF8563
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  
  Wire.write(decToBcd(aTm.Second));
  Wire.write(decToBcd(aTm.Minute));
  Wire.write(decToBcd(aTm.Hour));
  Wire.write(decToBcd(aTm.Day));
  Wire.write(decToBcd(aTm.Wday));
  Wire.write(decToBcd(aTm.Month));
  Wire.write(decToBcd(aTm.Year));
  
  Wire.endTransmission();
}

void setDS1307(tmElements_t &aTm)
{
  Wire.beginTransmission(DS1307address);
  Wire.write((uint8_t)0x00); // reset register pointer  
  
  Wire.write(decToBcd(aTm.Second)) ;   
  Wire.write(decToBcd(aTm.Minute));
  Wire.write(decToBcd(aTm.Hour));      // sets 24 hour format
  Wire.write(decToBcd(aTm.Wday));   
  Wire.write(decToBcd(aTm.Day));
  Wire.write(decToBcd(aTm.Month));
  Wire.write(decToBcd(tmYearToY2k(tm.Year))); 

  Wire.endTransmission();
}

void readPCF8563(tmElements_t &aTm) 
{
  // this gets the time and date from the PCF8563
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  Wire.endTransmission();
  
  Wire.requestFrom(PCF8563address, 7);
  aTm.Second = bcdToDec(Wire.read() & B01111111); // remove VL error bit
  aTm.Minute = bcdToDec(Wire.read() & B01111111); // remove unwanted bits from MSB
  aTm.Hour   = bcdToDec(Wire.read() & B00111111);
  aTm.Day    = bcdToDec(Wire.read() & B00111111);
  aTm.Wday   = bcdToDec(Wire.read() & B00000111);
  aTm.Month  = bcdToDec(Wire.read() & B00011111);  // remove century bit, 1999 is over
  aTm.Year   = bcdToDec(Wire.read());
}

void readDS1307(tmElements_t &aTm)
{
  Wire.beginTransmission(DS1307address);
  Wire.write((uint8_t)0x00); 
  Wire.endTransmission();

  // request the 7 data fields   (secs, min, hr, dow, date, mth, yr)
  Wire.requestFrom(DS1307address, tmNbrFields);
  aTm.Second = bcdToDec(Wire.read() & 0x7f);   
  aTm.Minute = bcdToDec(Wire.read() );
  aTm.Hour   = bcdToDec(Wire.read() & 0x3f);  // mask assumes 24hr clock
  aTm.Wday   = bcdToDec(Wire.read() );
  aTm.Day    = bcdToDec(Wire.read() );
  aTm.Month  = bcdToDec(Wire.read() );
  aTm.Year   = y2kYearToTm((bcdToDec(Wire.read())));
}

void ticker() 
{
  _millis = millis();
  
  if ((msStart == 0) and (msCurrent == 0)) 
  {
    msStart = (tm.Hour * 3600 + tm.Minute * 60 + tm.Second) * 1000;
    printToSerial(LOG_LEVEL_DEBUG, "msStart == 0 and msCurrent == 0 -> msStart = %d", msStart);
    
    if (msStart > _millis)
    {
      msStart = msStart - _millis;
    }
    msCurrent = msStart + _millis;
  }
  
  if ((msStart == 0) and (msCurrent > 0)) 
  {
    msCurrent = (tm.Hour * 3600 + tm.Minute * 60 + tm.Second) * 1000;
    printToSerial(LOG_LEVEL_DEBUG, "msStart == 0 and msCurrent > 0 -> msCurrent = %d", msCurrent);
    
    while (1) 
    {
      yield();
      if (msCurrent > _millis) 
      {
        msStart = msCurrent - _millis;
        printToSerial(LOG_LEVEL_DEBUG, "msStart rol: %d", msStart);
        break;
      } 
      else
      { 
        msCurrent += 86400000;
      }
    }
  }

  if (_millis % 1000 == 0) 
  {
    if (isRTC == 0) 
    {
      tm.Hour = (msCurrent / 1000)  % 86400L / 3600;
      tm.Minute = (msCurrent / 1000) % 3600 / 60;
      tm.Second = (msCurrent / 1000) % 60;
    }
    else
    {
      getRTCDateTime();
    }

    if ((is_time_set == 0) and (isConn))
    {
      syncDateTimeWithSntp();
    }
  }

  if (_millis % 5000 == 0) 
  {
    sensors.requestTemperatures();
    for (byte i = 0; i < 8; i++) 
    {
      if (group[i].alarmIndex != 255) 
      {
        byte _index = group[i].alarmIndex;
        if (group[i].temp < Sensor[_index].Temp) 
        {
          group[i].isAlarm = 1;
          
          for (byte j = 0; j < 16; j++) 
          {
            if ((newCh[j].group == i) and (newCh[j].type == 0)) 
            {
              if (newCurrent[j] > group[i].step) 
              {
                newCurrent[j] -= group[i].step;
                invPWM(j, newCurrent[j]);
              }

            }
          }
        }
        else 
        {
          group[i].isAlarm = 0;
        }

      } 
      else 
      {
        group[i].isAlarm = 0;
      }
    }

    for (byte i = 0; i < cSensor; i++) 
    {
      Sensor[i].Temp = sensors.getTempC(Sensor[i].addr);
    }
  }

  if (_millis % 10 == 0) 
  {
    if (msCurrent >= 86400000)
    {
      msCurrent = 0;
    }
    else
    {
      msCurrent += 10;
    }
  }
}



