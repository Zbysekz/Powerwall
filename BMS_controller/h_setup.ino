
void setup() {

  wdt_enable(WDTO_8S);
   
  Serial.begin(19200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize. use this only if you want to start/reset UNO when serial console is connected
  //while(!Serial) { }
  
  Serial.print(F("\nArduino Starting... "));

  status_eth = 1;//ok statuses
  status_i2c = 1;

  //inputs/outputs
  pinMode(PIN_MAIN_RELAY, OUTPUT);
  pinMode(PIN_HEATING, OUTPUT);
  pinMode(PIN_UPS_BTN, OUTPUT);
  
  digitalWrite(PIN_MAIN_RELAY, false);
  digitalWrite(PIN_HEATING, false);
  digitalWrite(PIN_UPS_BTN, false);


  ////////////////////////////////////////LCD/////////////////////////////////////////////////
    Serial.print(F("\nLCD init "));
  if(display.begin()){// for ESP display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    
    // Clear the buffer.
    display.clearDisplay();
    display.setFont(u8x8_font_chroma48medium8_r);
    displayOk=true;
  }
  ////////////////////////////////////////////////////////////////////////////////////////////
  
  ////////////
  Ethernet.begin(mac, ip);

  Serial.print(F("\nEthernet started "));
  
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
  
  Serial.println(F("\nSetup finished."));

  tmrStartTime = millis();
  /* DONT USE OTA
  pinMode(5,INPUT_PULLUP);
  if(!digitalRead(5)){
    Serial.println(F("OTA MODE"));
    OTAmode=1;
  }*/

  stateMachineStatus=0;
}
