
void setup() {
  wdt_enable(WDTO_8S);
   
  Serial.begin(19200);
  Serial.setTimeout(2000);
  bridgeSerial.begin(19200);

  Serial.print(F("Program start"));
  status_i2c = 1;

  //inputs/outputs
  pinMode(PIN_MAIN_RELAY, OUTPUT);
  pinMode(PIN_GARAGE, OUTPUT);

  digitalWrite(PIN_MAIN_RELAY, false);
  digitalWrite(PIN_GARAGE, false);

  delay(1000);

  ////////////////////////////////////////////////////////////////////////////////////////////
  
  I2c.begin();// SDA=PC4, SCL=PC5
  I2c.setSpeed(false);//100kHz
  I2c.pullup(false);// no pullups, we have external
  I2c.timeOut(500);//500ms timeout
  
  
  tmrStartTime = millis();
  stateMachineStatus=0;
  nextState = 0;
  offset_portion = 0;
  offset_portion2 = 0;
  solarConnected = true;
  SendEvent(5,0);
  Log("\nSetup finished.");
}
