
void setupWebServer() 
{
  server.onNotFound(handleNotFound);
  server.on("/reboot", handleReboot);
  server.on("/sv", handleSv);
  server.on("/get", _get);
  server.on("/set", _set);
  server.on("/rescue", HTTP_GET, handleRescue);
  server.onFileUpload(handleFileUpload);
  server.on("/upload", HTTP_POST, handleUpload);
  
  server.begin();
  
  for (byte i = 0; i < MAX_RETRAY_COUNT; i++) 
  {
    if (MDNS.begin(esp_hostname.c_str())) 
    {
      MDNS.addService("http", "tcp", 80);
      break;
    }
  }

  delay(1000);
  
  server.serveStatic("/", SPIFFS, "/root_page.html");
  
  Dir dir = SPIFFS.openDir("/");   
  while (dir.next()) 
  {
    if (dir.fileName() != "/root_page.html") 
    {
      if (dir.fileName().endsWith(".gz")) 
      {
        File file_gz;
        file_gz = SPIFFS.open(dir.fileName(), "r");
        if (file_gz)
        { 
          server.streamFile(file_gz, "application/x-gzip");
          file_gz.close();
        }
      }
      else
      {
        server.serveStatic(dir.fileName().c_str(), SPIFFS, dir.fileName().c_str());
      }

      printToSerial(LOG_LEVEL_DEBUG, "HTTP server serving file: %s size %d", dir.fileName().c_str(), dir.fileSize());
    }
  }

  printToSerial(LOG_LEVEL_INFO, "Web Server setup done");
}

void handleNotFound() 
{
  if (server.uri().endsWith(".pdf")) 
  {
    server.send(404, TEXT_PLAIN, F("May be You are using 512k module\n\rIn this case no room for manual, sorry."));
  }
  
  String _resp = "Not found " + server.uri() + "\n";
  for (byte i = 0; i < server.args(); i++) 
  {
    printToSerial(LOG_LEVEL_DEBUG, "arg(%d) <%s> = %s", i, server.argName(i).c_str(), server.arg(i).c_str());

  }
  
  server.send(404, TEXT_PLAIN, _resp);
}

void handleReboot() 
{
  reboot();
}

void handleSv()
{
  saveAllEEPROM();
}

void handleRescue()
{
  server.send(200, TEXT_HTML, F("<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\"> <input\
        name=\"myfile1\" size=\"20\" lang=\"en\" type=\"file\"> <input value=\"Upload\" type=\"submit\"></form>"));
}

void handleFileUpload()
{
  if (server.uri() != "/upload") 
  {
    return;
  }
  
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) 
  {
    Serial.setDebugOutput(true);

    isSPIFFS = upload.filename.endsWith(".spiffs.bin");
    isFirmware = upload.filename.endsWith(".bin") and upload.filename.startsWith("0x0");
    
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (isSPIFFS) 
    {
      SPIFFS.format();
      if (!Update.begin(maxSketchSpace, U_SPIFFS)) 
      { 
        //start with max available size
        Update.printError(Serial);
      }
    }
    
    if (isFirmware) 
    {
      if (ESP.getFlashChipRealSize() != 4194304) 
      {
        return;
      }
      if (!Update.begin(maxSketchSpace)) 
      { 
        //start with max available size
        Update.printError(Serial);
      }
    }
    
    if (!isFirmware and !isSPIFFS) 
    {
      String filename = upload.filename;
      
      if (filename[0] != '/') 
      {
        filename = "/" + filename;
      }
      if (filename == "/") 
      {
        return;
      }
      
      fsUploadFile = SPIFFS.open(filename, "w");
      if (!fsUploadFile)
      {
        filename = String();
      }
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) 
  {
    if (isFirmware || isSPIFFS) 
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) 
      {
        Update.printError(Serial);
      }
    }
    else 
    {
      if (fsUploadFile)
      {
        fsUploadFile.write(upload.buf, upload.currentSize);
      }
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) 
  {
    if (isFirmware || isSPIFFS) 
    {
      if (!Update.end(true))
      { 
        Update.printError(Serial);
      }
    } 
    else 
    {
      if (fsUploadFile) 
      {
        fsUploadFile.close();
      }
      
      server.serveStatic(upload.filename.c_str(), SPIFFS, upload.filename.c_str(), "max-age=86400");
    }

    Serial.setDebugOutput(false);
  }
  yield();
}

void handleUpload()
{
  server.sendHeader("Connection", "close");
  if (isFirmware || isSPIFFS) 
  {
    server.send(200,  TEXT_HTML, F("<meta http-equiv=\"refresh\" content=\"0; URL='/reboot'\" />\n\r<script>alert('Need to reboot');</script>"));
    ESP.reset();
  } 
  else 
  {
    server.send(200,  TEXT_HTML, F("<meta http-equiv=\"refresh\" content=\"0; URL='/'\" />\n\r<script>alert('File uploaded');</script>"));
  }
}

void reboot()
{
  server.send(200, TEXT_PLAIN, F("You are crazy !!!"));
  ESP.reset();
}

