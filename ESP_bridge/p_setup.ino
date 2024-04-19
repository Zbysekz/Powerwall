

void ICACHE_FLASH_ATTR setup(){

  Serial.begin(9600);
  bridgeSerial.begin(9600);

  Serial.println(F("Program start"));
  
  //WIFI
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);   

  ConnectToWifi();

  server.begin();
  ServerSetup();
  Serial.println("HTTP server started");

  
  ArduinoOTA.setHostname("ESP_bridge cellarControl");
  ArduinoOTA.begin();


  EEPROM.begin(512);


  digitalWrite(LED_BUILTIN, false);
  delay(300);
  digitalWrite(LED_BUILTIN, true);

  Serial.println(F("Setup finished"));

  buffer_ptr = 0;
  receive_state = 0;

}
