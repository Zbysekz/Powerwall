void ICACHE_FLASH_ATTR setup() {
 
  Serial.begin(19200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) { }
  
  Serial.print("\nESP Starting... ");

  Serial.println("IP:");
  Serial.println(ip.toString());

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);   
      /*
      //Creating own wifi
      WiFi.persistent(false);
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP_SETUP");
      */

  Serial.println("Setting up APOTA");
  
  ArduinoOTA.setHostname("APOTA");
  ArduinoOTA.onStart(otaStart);
  ArduinoOTA.onEnd(otaEnd);
  ArduinoOTA.onProgress(otaProgress);
  ArduinoOTA.onError(otaError);
  ArduinoOTA.begin();

  Wire.begin(2,0); // SDA GPIO2, SCL GPIOO -> D4, D3 ------ I2C

  initWire();

  //setup timer1
  os_timer_setfn(&timer1, timer1Callback, NULL);
  os_timer_arm(&timer1, 500, true);
  
  Serial.println(F("Setup finished."));
  pinMode(5,INPUT_PULLUP);
  if(!digitalRead(5)){
    Serial.println(F("OTA MODE"));
    OTAmode=1;
  }
}
