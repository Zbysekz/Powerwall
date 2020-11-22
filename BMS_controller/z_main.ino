
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

  
  if(CheckTimer(tmrScanModules, 5000L)){//read modules periodically & balance
    
    if(ReadModules(true)){//quick read
      status_i2c = 1;//ok
      iFailCommCnt=0;
    }else{
      Serial.println(F("Error in scanning modules"));
      status_i2c = 2;//error in reading
      if(iFailCommCnt++>3){
        Serial.println(F("Communication failure several times, scanning restart"));
        iFailCommCnt = 0;
        modulesCount = 0;
        I2c.end(); //restart I2C hw
        delay(500);
        I2c.begin();
      }
    }

    if(getSafetyConditions(true))
      BalanceCells();
  }

  if(CheckTimer(tmrReadStatistics, 300000L)){//read statistics
    bool res = true;
    uint16_t iErrCnt,iBurnCnt;
    
    for(int i=0;i<modulesCount;i++){
      res = Cell_read_error_counter(moduleList[i].address, iErrCnt);
      res = res && Cell_read_burning_counter(moduleList[i].address, iBurnCnt);

      if(!res){
        Serial.print(F("\nError in reading statistics for module:"));
        Serial.println(moduleList[i].address);
        moduleList[i].iStatErrCnt = 0;
        moduleList[i].iBurningCnt = 0;
        
      }else{
        moduleList[i].iStatErrCnt = iErrCnt;
        moduleList[i].iBurningCnt = iBurnCnt;
      }
    }
    xReadyToSendStatistics = true;
  }

  if(modulesCount!=REQUIRED_CNT_MODULES && CheckTimer(tmrRetryScan, 5000L)){
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
 
  bool xSafetyConditions = getSafetyConditions(false);

  switch (stateMachineStatus){
    case 0://IDLE - diconnected state
      digitalWrite(PIN_MAIN_RELAY, false);
      digitalWrite(PIN_UPS_BTN, false);    
      

      if(xSafetyConditions && xReqRun){
        digitalWrite(PIN_MAIN_RELAY, true);
        tmrDelay=millis();
        stateMachineStatus = 1;
        Serial.println(F("CONNECTING for RUN"));
      }
      else if(xSafetyConditions && xReqChargeOnly){
        digitalWrite(PIN_MAIN_RELAY, true);
        tmrDelay=millis();
        stateMachineStatus = 20;
        Serial.println(F("CONNECTING for CHARGE only"));
      }
      
    break;
    
    // UPS can be after start in "sleep" mode, alternating between "On-Line" and "overload" LEDs
    // this happens usually when battery is cut off in run. We need to push start button shortly to switch it off, then can be started by holding long btn start
  
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
    case 10://RUN - discharging, possibly also charging at same time

      if (xReqDisconnect || !xSafetyConditions){
        
        tmrDelay=millis();
        digitalWrite(PIN_UPS_BTN, true);

        if(!xSafetyConditions){
          Serial.println(F("EMERGENCY shutdown"));
          xEmergencyShutDown = true;
        }else
          Serial.println(F("Disconnect shutdown"));
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

        if(xEmergencyShutDown)
          stateMachineStatus = 99;
        else
          stateMachineStatus = 0;
      }
    break;

    case 20://CHARGE ONLY - main relay is switched on but UPS's are off
      if(!xSafetyConditions){
        digitalWrite(PIN_MAIN_RELAY, false);
        stateMachineStatus = 99;
      }else if (xReqDisconnect){
        digitalWrite(PIN_MAIN_RELAY, false);
        stateMachineStatus = 0;
      }
    break;
    
    case 99://ERROR
      digitalWrite(PIN_MAIN_RELAY, false);
      digitalWrite(PIN_UPS_BTN, false);
      xEmergencyShutDown = false;
      if(xReqErrorReset){
        stateMachineStatus = 0;
      }

    break;
    default:;
    
  }

  //reset all signals after one iteration
  xReqRun = false;
  xReqChargeOnly = false;
  xReqDisconnect = false;
  xReqErrorReset = false;
  
}

//Balancing is done by burning energy on cell modules, cases:
// 1) if difference "Z" is bigger than threshold, burn energy. Z is calculated for each cell as: Z = thisCellV - min(otherCellsV)
// 2) we are fully charged, but one or more cells has Z bigger than some small threshold
void BalanceCells(){

  for(int i=0;i<modulesCount;i++){
    //calculate "Z"
    uint16_t min=999;
    for(int y=0;y<modulesCount;y++)
      if(i!=y){
        if(moduleList[y].voltage<min)
          min=moduleList[y].voltage;
      }

    if(moduleList[i].voltage > min){//if you are the lowest, do not balance
      if(moduleList[i].voltage - min > IMBALANCE_THRESHOLD){
        //start burning for that module
        //only if he is not burning already

        uint8_t xBurning = 0;
        bool res = Cell_read_bypass_enabled_state(moduleList[i].address, xBurning);

        if(res && !xBurning)
          res = Cell_set_bypass_voltage(moduleList[i].address, min + IMBALANCE_THRESHOLD);// burn to just match imbalance threshold
          
        if(!res){
          Serial.print(F("\nError while setting cell to burn! module:"));
          Serial.println(moduleList[i].address);
        }else{
          Serial.print(F("\nBURNING start! module:"));
          Serial.println(moduleList[i].address);
        }
      }
    }
  }
}
