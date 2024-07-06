

void ICACHE_FLASH_ATTR setup(){
  wdt_enable(WDTO_8S);
  Serial.begin(19200);
  bridgeSerial.begin(19200);

  Serial.println(F("Program start"));

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pin_LED, OUTPUT);
  
  digitalWrite(LED_BUILTIN, true);
  delay(300);
  digitalWrite(LED_BUILTIN, false);

  //WIFI
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);   

  ConnectToWifi();

  server.begin();
  ServerSetup();
  Serial.println("HTTP server started");

  
  ArduinoOTA.setHostname("ESP_bridge powerwall");
  ArduinoOTA.begin();

  EEPROM.begin(512);


  Serial.println(F("Setup finished"));

  buffer_ptr = 0;
  receive_state = 0;
  queue_ptr_actual = 0;
  queue_ptr_ready = 0;

  for(int i = 0;i<3;i++){
    digitalWrite(LED_BUILTIN, true);
    delay(300);
    digitalWrite(LED_BUILTIN, false);
    delay(300);
  }

  digitalWrite(pin_LED, true);

}
