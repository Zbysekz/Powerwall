
void loop() {

  //periodically connect to the server and exchange data
  if(CheckTimer(tmrServerComm, 10000L)){
    ExchangeCommunicationWithServer();
  }

  //process received data from server
  ProcessReceivedData();

  //handle display logic
  if(CheckTimer(tmrDisplay, 1000L)){
    HandleDisplay();
  }

  //scan modules
  if(CheckTimer(tmrScanModules, 5000L)){
    ReadModules(true);//quick read
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
