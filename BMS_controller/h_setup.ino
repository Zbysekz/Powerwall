
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


  ////////////////////////////////////////////////////////////////////////////////////////////
  
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  Serial.print(F("\nEthernet started "));
  
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet shield was not found!"));

    status_eth=10;
    while (true);// do nothing, no point running without Ethernet hardware, WDT will reset Arduino
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println(F("Ethernet cable is not connected."));
    status_eth=20;
  }

  
  I2c.begin();// SDA=PC4, SCL=PC5
  I2c.setSpeed(false);//100kHz
  I2c.pullup(false);// no pullups, we have external
  I2c.timeOut(500);//500ms timeout
  
  
  tmrStartTime = millis();
  stateMachineStatus=0;

  Serial.println(F("\nSetup finished."));
}
