
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


  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  Wire.setClockStretchLimit(1000);
  //Wire.begin(5,0); // I2C -- SDA GPIO5, SCL GPIOO /if we are using LCD it is already initialized

  
  ////////////////////////////////////////LCD/////////////////////////////////////////////////
 // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(500);

  // Clear the buffer.
  display.clearDisplay();

  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  ////////////////////////////////////////////////////////////////////////////////////////////
  
  Serial.println(F("Setup finished."));
  
  /* DONT USE OTA
  pinMode(5,INPUT_PULLUP);
  if(!digitalRead(5)){
    Serial.println(F("OTA MODE"));
    OTAmode=1;
  }*/
}
