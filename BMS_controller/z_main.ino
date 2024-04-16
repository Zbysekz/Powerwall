
void loop() {

  wdt_reset();
  
  //periodically connect to the server and exchange data
  //and control heating
  if(CheckTimer(tmrServerComm, 10000L)){
    ExchangeCommunicationWithServer();

    if(xHeating)//count energy if we are heating rack
      if(iHeatingEnergyCons<65535)iHeatingEnergyCons ++;
  }

  /*if(oneOfCellIsHigh){
      solarConnected = false;
      tmrDelayAfterSolarReconnect = millis();
  }else{
    if ((unsigned long)(millis() - tmrDelayAfterSolarReconnect) > 600000L){
      solarConnected = true;//connect again
    }
  }*/

  digitalWrite(PIN_GARAGE, garage_contactor);
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
      Log("Error in scanning modules");
      status_i2c = 2;//error in reading
      iFailCommCnt++;
      if(iFailCommCnt==2){
        xFullReadDone = false;//do complete scan
      }
      if(iFailCommCnt>3){
        Log("Comm.fail.,scan restart");
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
        Log("\nErr in reading stats:");
        Log(moduleList[i].address);
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
      if(xSafetyConditions && xReqRun){
        tmrDelay=millis();
        nextState = 1;
        Log("CONNECTING for RUN");
        SendEvent(5,1);
      }
      else if(xSafetyConditions && xReqChargeOnly){
        digitalWrite(PIN_MAIN_RELAY, true);
        tmrDelay=millis();
        nextState = 20;
        Log("CONNECTING for CHARGE only");
        SendEvent(5,2);
      }
      
    break;
    case 1://WAIT TO CONTACTOR
      if(CheckTimer(tmrDelay, 1000L)){
        tmrDelay=millis();
        nextState = 10;
      }
    break;
    case 10://RUN - discharging, possibly also charging at same time
      
      if(!xSafetyConditions){
          errorStatus_cause = errorStatus;//to retain information
          xEmergencyShutDown = true;
          nextState = 99;
          Log("EMERGENCY shutdown");
          SendEvent(5,3);
      }
      else if ((xReqChargeOnly || oneOfCellIsLow)){
        nextState = 20;
        Log("Going from RUN to charge only");
        SendEvent(5,4);
      }else if (xReqDisconnect){
        nextState = 0;
        Log("Disconnect shutdown");
        SendEvent(5,5);
      }
      break;
    case 20://CHARGE ONLY - main relay is switched on, solar breaker is also on but contactor behind DC/AC is off
      if(!xSafetyConditions){
        errorStatus_cause = errorStatus;//to retain information
        nextState = 99;
      }else if (xReqDisconnect){
        nextState = 0;
      }else if (xReqRun){
        tmrDelay=millis();
        nextState = 1;
        Log("GOING from CHARGE TO RUN");
        SendEvent(5,6);
      }
    break;
    
    case 99://ERROR
      if(xReqErrorReset){
        nextState = 0;
      }
    break;
    default:;
  }

  //rewrite to outputs
  digitalWrite(PIN_MAIN_RELAY, (stateMachineStatus==1 || stateMachineStatus==10 || stateMachineStatus==20));



  //reset all signals after one iteration
  xReqRun = false;
  xReqChargeOnly = false;
  xReqDisconnect = false;
  xReqErrorReset = false;

  stateMachineStatus = nextState;
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
          Log("\nBURNING start! module:");
          Log(moduleList[i].address);
        }
        if(!res){
          Log("\nError while setting cell to burn! module:");
          Log(moduleList[i].address);
        }
      }
    }
  }  
}
