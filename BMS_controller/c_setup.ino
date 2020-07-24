
void setup() {
 
  Serial.begin(19200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) { }
  
  Serial.print(F("\nArduino Starting... "));

  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet shield was not found!"));

    display.clearDisplay();
    display.print(F("HW eth not found!"));
    status_eth=10;
    while (true);// do nothing, no point running without Ethernet hardware
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println(F("Ethernet cable is not connected."));
    status_eth=20;
  }

  
  

  Wire.setTimeout(1000);  //1000ms timeout
  Wire.setClock(100000);  //100khz
  //Wire.begin(5,0); // I2C -- SDA GPIO5, SCL GPIOO /if we are using LCD it is already initialized

  
  ////////////////////////////////////////LCD/////////////////////////////////////////////////
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin();// for ESP display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    
  // Clear the buffer.
  display.clearDisplay();

  display.setFont(u8x8_font_chroma48medium8_r);
  ////////////////////////////////////////////////////////////////////////////////////////////
  
  Serial.println(F("Setup finished."));


  tmrStartTime = millis();
  /* DONT USE OTA
  pinMode(5,INPUT_PULLUP);
  if(!digitalRead(5)){
    Serial.println(F("OTA MODE"));
    OTAmode=1;
  }*/
}
