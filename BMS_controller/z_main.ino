
void loop() {

  wdt_reset();
  
  //periodically connect to the server and exchange data
  //and control heating
  if(CheckTimer(tmrServerComm, 10000L)){
    ExchangeCommunicationWithServer();

    ControlHeating();
  }

  PowerStateMachine();//state machine for relay power control

  
  ProcessReceivedData();//process received data from server

  
  /*if(CheckTimer(tmrDisplay, 1000L)){//handle display logic
    Serial.println("Handle display");
    HandleDisplay();
    Serial.println("Handle display END");
  }*/

  
  if(CheckTimer(tmrScanModules, 5000L)){//read modules periodically
    Serial.println("Perioda!!!!--------");
    ReadModules(true);//quick read
    Serial.println("reading done");
  }


  if(modulesCount!=REQUIRED_CNT_MODULES && CheckTimer(tmrRetryScan, 10000L)){
    xFullReadDone = false; // if we have not all modules, keep scanning for them
  }


  //wait some time after boot-up or after provisioning, scan modules and read fully
  if(!xFullReadDone && (unsigned long)(millis() - tmrStartTime) > 3000L){
    ScanModules();
    delay(50);
    ReadModules(false);//full read
    xFullReadDone=true;
  }
  
}

void ControlHeating(){
  //calculate actual temperature
  uint32_t actualTemp=0;
  uint16_t cnt=0;
  //go through every active module and make avg temp
  for(int i=0;i<modulesCount;i++){
    if(moduleList[i].validValues){
      actualTemp+=moduleList[i].temperature;
      cnt++;
    }
  }
  if(cnt!=0)
    actualTemp/=cnt;
  else
    actualTemp=0;

  Serial.println(F("ActualTemp:"));
  Serial.println(actualTemp);

  if(cnt>0){
    if(actualTemp < REQUIRED_RACK_TEMPERATURE - 1){
      xHeating = true;
    }
  
    if(actualTemp > REQUIRED_RACK_TEMPERATURE + 1){
      xHeating = false;
    }
  }else xHeating = false;//no valid values, no heating, but will report error

  if(xHeating)
    digitalWrite(PIN_HEATING, true);
  else
    digitalWrite(PIN_HEATING, false);
}

void PowerStateMachine(){

  bool voltagesOk = true;

  for(int i=0;i<modulesCount;i++)
    if(moduleList[i].voltage < DISCHARGE_VOLT_LOW || moduleList[i].voltage > DISCHARGE_VOLT_HIGH){
      voltagesOk = false;
      break;
    }
  
  bool xSafetyConditions = voltagesOk && status_i2c==1 && status_eth==1 && modulesCount==REQUIRED_CNT_MODULES;

  switch (stateMachineStatus){
    case 0://IDLE
      digitalWrite(PIN_MAIN_RELAY, false);
      digitalWrite(PIN_UPS_BTN, false);    

      if(xSafetyConditions && xConnectBattery){
        digitalWrite(PIN_MAIN_RELAY, true);
        tmrDelay=millis();
        stateMachineStatus = 1;
        Serial.println(F("CONNECTING-----"));
      }
      
    break;
    case 1://WAIT TO CONTACTOR
      if(CheckTimer(tmrDelay, 1000L)){
        digitalWrite(PIN_UPS_BTN, true);
        tmrDelay=millis();
        stateMachineStatus = 2;
        Serial.println(F("UPS start-----"));
      }
    break;
    case 2://WAIT START BUTTON
      if(CheckTimer(tmrDelay, 2000L)){
        digitalWrite(PIN_UPS_BTN, false);
        stateMachineStatus = 10;
        Serial.println(F("UPS done-----"));
      }
    break;
    case 10://RUN
      if(!xSafetyConditions){
        stateMachineStatus = 20;
      }else if (xDisconnectBattery){
        tmrDelay=millis();
        digitalWrite(PIN_UPS_BTN, true);
        stateMachineStatus = 14;
      }
    break;
    case 14://WAIT STOP BUTTON
      if(CheckTimer(tmrDelay, 2000L)){
        digitalWrite(PIN_UPS_BTN, false);
        stateMachineStatus = 15;
      }
    break;
    case 15://WAIT FOR UPS SHUTDOWN
      if(CheckTimer(tmrDelay, 1000L)){
        digitalWrite(PIN_MAIN_RELAY, false);
        stateMachineStatus = 0;
      }
    break;
    case 20://ERROR
      digitalWrite(PIN_MAIN_RELAY, false);
      digitalWrite(PIN_UPS_BTN, false);
      if(xResetRequested){
        stateMachineStatus = 0;
      }
    break;
    default:;
    
  }

  //reset all signals after one iteration
  xConnectBattery = false;
  xDisconnectBattery = false; 
  xResetRequested = false;

  
}
