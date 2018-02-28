
void setupWiFi()
{
  //вместо скрытия пароля сделать отключение точки доступа
  if (digitalRead(0) == 0) 
  {
    isHidePassword = 0;
  }
  
  startSoftAP();
  
  WiFi.mode(WIFI_AP_STA);
  
  //isConn = false;
  //if (foundWiFi(ssid)) 
  //{
    connectWiFi();
  //}   
}

void connectWiFi()
{
  isConn = false;
  
  WiFi.begin(ssid.c_str(), password.c_str());

  int tmp_a = 0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    tmp_a++;
    //isConn = true;
    if (tmp_a > 128) 
    {
      //isConn = false;
      break;
    }
  }
  
  isConn = WiFi.localIP() != 0;
}

void reconnectWiFi()
{ 
  WiFi.disconnect(true);
  
  connectWiFi();
}

void startSoftAP() 
{
  if (esp_hostname.length() == 0) 
  {
    esp_hostname = "SSLAC";
  }
  
  String SSID_AP = esp_hostname;
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
    SSID_AP = esp_hostname;
    if (isConn) 
    {
      PASSWORD_AP = password;
    }
  }

  WiFi.softAP(SSID_AP.c_str(), PASSWORD_AP.c_str());
  Serial.print(F("Starting AP new : ")); Serial.print(SSID_AP); Serial.print("/"); Serial.println(PASSWORD_AP);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
}

bool foundWiFi(String _SSID) 
{
  foundedNet = WiFi.scanNetworks();
  bool result = false;
  if (foundedNet == 0) 
  {
    result = false;
  } 
  else 
  {
    for (int i = 0; i < foundedNet; ++i) 
    {
      if (_SSID == WiFi.SSID(i)) 
      {
        result = true;
        break;
      }
    }
  }
  return result;
}


