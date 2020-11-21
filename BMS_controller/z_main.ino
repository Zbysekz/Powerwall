
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
    
    if(ReadModules(true))//quick read
      status_i2c = 1;//ok
    else{
      Serial.println("Error in scanning modules");
      status_i2c = 2;//error in reading
    }

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

  Serial.print(F("\nModules:"));
  Serial.println(modulesCount);
  Serial.print(F("\nActualTemp:"));
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
    
    // UPS can be after start in "sleep" mode, alternating between "On-Line" and "overload" LEDs
    // this happens usually when battery is cut off in run. We need to push start button three times to start UPS realiably
  
    case 1://WAIT TO CONTACTOR
      if(CheckTimer(tmrDelay, 1000L)){
        digitalWrite(PIN_UPS_BTN, true);//push start button for short time
        delay(200);
        digitalWrite(PIN_UPS_BTN, false);
        tmrDelay=millis();
        stateMachineStatus = 2;
      }
    break;
    case 2://WAIT PAUSE
      if(CheckTimer(tmrDelay, 1000L)){
        tmrDelay=millis();
        digitalWrite(PIN_UPS_BTN, true);
        stateMachineStatus = 3;
      }
    break;
    case 3://WAIT LONG START BUTTON
      if(CheckTimer(tmrDelay, 2500L)){
        digitalWrite(PIN_UPS_BTN, false);
        stateMachineStatus = 10;
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
