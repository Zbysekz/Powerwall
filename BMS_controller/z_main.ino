
void loop() {

  wdt_reset();
  
  //periodically connect to the server and exchange data
  //and control heating
  if(CheckTimer(tmrServerComm, 10000L)){
    ExchangeCommunicationWithServer();

    if(xHeating)//count energy if we are heating rack
      if(iHeatingEnergyCons<65535)iHeatingEnergyCons ++;
  }

  if(oneOfCellIsHigh){
      solarConnected = false;
      tmrDelayAfterSolarReconnect = millis();
  }else{
    if ((unsigned long)(millis() - tmrDelayAfterSolarReconnect) > 600000L){
      solarConnected = true;//connect again
    }
  }

  //digitalWrite(PIN_SOLAR_IN, solarConnected);
  //digitalWrite(PIN_VENTILATOR, xHeating || );
  xSafetyConditions = getSafetyConditions();
  
  PowerStateMachine();//state machine for relay power control
  
  ProcessReceivedData();//process received data from server
  
  if(CheckTimer(tmrScanModules, 5000L)){//read modules periodically & balance
    
    if(ReadModules(true)){//quick read
      status_i2c = 1;//ok
      iFailCommCnt=0;
    }else{
      Serial.println(F("Error in scanning modules"));
      status_i2c = 2;//error in reading
      iFailCommCnt++;
      if(iFailCommCnt==2){
        xFullReadDone = false;//do complete scan
      }
      if(iFailCommCnt>3){
        Serial.println(F("Comm.fail.,scan restart"));
        iFailCommCnt = 0;
        modulesCount = 0;
        I2c.end(); //restart I2C hw
        delay(500);
        I2c.begin();
      }
      
    }

    if(xSafetyConditions)
      BalanceCells();
  }

  if(CheckTimer(tmrReadStatistics, 300000L)){//read statistics - each 5mins
    bool res = true;
    uint16_t iErrCnt,iBurnCnt;
    
    for(int i=0;i<modulesCount;i++){
      res = Cell_read_error_counter(moduleList[i].address, iErrCnt);
      res = res && Cell_read_burning_counter(moduleList[i].address, iBurnCnt);

      if(!res){
        Serial.print(F("\nErr in reading stats:"));
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
  //go through every active module and calculate median temperature (better than avg)
  bool atLeastOneValid = false;
  for(int i=0;i<modulesCount;i++){
    if(moduleList[i].validValues){
      temperature_median.add(moduleList[i].temperature);
      atLeastOneValid = true;
    }
  }
  
  if(atLeastOneValid){
    if(temperature_median.getMedian() < REQUIRED_RACK_TEMPERATURE - 1){
      xHeating = true;
    }
  
    if(temperature_median.getMedian() > REQUIRED_RACK_TEMPERATURE + 1){
      xHeating = false;
    }
  }else xHeating = false;//no valid values, no heating, but will report error
}

void PowerStateMachine(){

  switch (stateMachineStatus){
    case 0://IDLE - diconnected state
      digitalWrite(PIN_MAIN_RELAY, false);      

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
    case 1://WAIT TO CONTACTOR
      if(CheckTimer(tmrDelay, 1000L)){
        tmrDelay=millis();
        stateMachineStatus = 10;
      }
    break;
    case 10://RUN - discharging, possibly also charging at same time
      if (xReqDisconnect || !xSafetyConditions || xReqChargeOnly || oneOfCellIsLow){
        tmrDelay=millis();

        if(!xSafetyConditions){
          Serial.println(F("EMERGENCY shutdown"));
          errorStatus_cause = errorStatus;//to retain information
          xEmergencyShutDown = true;
        }else if (xReqChargeOnly)Serial.println(F("Going from RUN to charge only"));
        else
          Serial.println(F("Disconnect shutdown"));
        
        if(xSafetyConditions && (xReqChargeOnly || oneOfCellIsLow))
          stateMachineStatus = 20;
        else
          stateMachineStatus = 15;
      }
    break;
    case 15:
      if(CheckTimer(tmrDelay, 1000L)){
        digitalWrite(PIN_MAIN_RELAY, false);

        if(xEmergencyShutDown)
          stateMachineStatus = 99;
        else
          stateMachineStatus = 0;
      }
    break;

    case 20://CHARGE ONLY - main relay is switched on, solar breaker is also on but contactor behind DC/AC is off
      if(!xSafetyConditions){
        digitalWrite(PIN_MAIN_RELAY, false);
        errorStatus_cause = errorStatus;//to retain information
        stateMachineStatus = 99;
      }else if (xReqDisconnect){
        digitalWrite(PIN_MAIN_RELAY, false);
        stateMachineStatus = 0;
      }else if (xReqRun){
        tmrDelay=millis();
        stateMachineStatus = 1;
        Serial.println(F("GOING from CHARGE TO RUN"));
      }
    break;
    
    case 99://ERROR
      digitalWrite(PIN_MAIN_RELAY, false);
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

  //calculate sum voltage
  uint32_t sumVoltage = 0;
  for(int i=0;i<modulesCount;i++){
    sumVoltage += moduleList[i].voltage_avg;
  }
  uint16_t imbalanceThreshold = 0;
  if(sumVoltage>=CHARGED_LEVEL)imbalanceThreshold = IMBALANCE_THRESHOLD_CHARGED; else imbalanceThreshold = IMBALANCE_THRESHOLD;
  
  for(int i=0;i<modulesCount;i++){
    //calculate "Z"
    uint16_t min=999;
    for(int y=0;y<modulesCount;y++)
      if(i!=y){
        if(moduleList[y].voltage_avg<min)
          min=moduleList[y].voltage_avg;
      }

    if(moduleList[i].voltage_avg > min){//if you are the lowest, do not balance
      if(moduleList[i].voltage_avg - min > imbalanceThreshold){
        //start burning for that module
        //only if he is not burning already
        bool res = true;
        if(!moduleList[i].burning){
          res = Cell_set_bypass_voltage(moduleList[i].address, min + imbalanceThreshold);// burn to just match imbalance threshold
          //Serial.print(F("\nBURNING start! module:"));
          Serial.println(moduleList[i].address);
        }
        if(!res){
          Serial.print(F("\nError while setting cell to burn! module:"));
          Serial.println(moduleList[i].address);
        }
      }
    }
  }  
}
