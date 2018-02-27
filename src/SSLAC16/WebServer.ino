
void setupWebServer() {
  //TODO: place web server initialization code here
}

void root_page() {
  File file = SPIFFS.open("/root_page.html", "r");
  server.streamFile(file, text_html);
  file.close();
}

void reboot() {
  server.send(200, text_plain, F("You are crazy !!!"));
  ESP.reset();
}
