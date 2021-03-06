
void setupWiFi()
{
  if (digitalRead(0) == 0) 
  {
    isHidePassword = 0;
    isSoftAPDisabled = 0;
  }
  
  if(isSoftAPDisabled == 1)
  {
    WiFi.mode(WIFI_STA);
    printToSerial(LOG_LEVEL_INFO, "Soft AP disabled");
  }
  else
  {
    startSoftAP();
    WiFi.mode(WIFI_AP_STA);
  }
  
  connectWiFi();

  printToSerial(LOG_LEVEL_INFO, "WiFi setup done");
}

void connectWiFi()
{
  if(ssid != "")
  {   
    WiFi.begin(ssid.c_str(), password.c_str());
  
    int tmp_a = 0;
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      tmp_a++;
      if (tmp_a > MAX_RETRAY_COUNT) 
      {
        break;
      }
    }
  }
  
  isConn = WiFi.localIP() != 0;

  if(isConn)
  {
    printToSerial(LOG_LEVEL_INFO, "WiFi connected. IP adress: %s", WiFi.localIP().toString().c_str());
  }
}

void reconnectWiFi()
{ 
  WiFi.disconnect(true);
  
  connectWiFi();
}

void disconnectWiFi()
{
  WiFi.disconnect(true);

  ssid = "";
  password = "";

  isConn = 0;
}

void startSoftAP() 
{
  if (espHostname.length() == 0) 
  {
    espHostname = "SSLAC";
  }
  
  String SSID_AP = espHostname;
  SSID_AP += "_";
  String PASSWORD_AP;
  
  if (ESP.getChipId() < 10000000) 
  {
    SSID_AP += String(10000000 + ESP.getChipId());
    PASSWORD_AP = String(10000000 + ESP.getChipId());
  } 
  else 
  {
    SSID_AP += String(ESP.getChipId());
    PASSWORD_AP = String(ESP.getChipId());
  }
  
  if (isHidePassword == 1) 
  {
    SSID_AP = espHostname;
    if (isConn) 
    {
      PASSWORD_AP = password;
    }
  }

  WiFi.softAP(SSID_AP.c_str(), PASSWORD_AP.c_str());

  printToSerial(LOG_LEVEL_INFO, "Soft AP started: %s / %s. IP address: %s", SSID_AP.c_str(), PASSWORD_AP.c_str(), WiFi.softAPIP().toString().c_str());
}



