
void loop() {

  //periodically connect to the server and exchange data
  if(CheckTimer(tmrServerComm, 5000L)){
    ExchangeCommunicationWithServer();
  }

  //process received data from server
  ProcessReceivedData();

  //handle display logic
  HandleDisplay();

  //scan modules
  if(CheckTimer(tmrScanModules, 5000L)){
    ReadModules(true);//quick read
  }


  //wait some time after boot-up, scan modules and read fully
  if(!xFullReadDone && (unsigned long)(millis() - tmrStartTime) > 3000L){
    ScanModules();
    delay(50);
    ReadModules(false);//full read
    xFullReadDone=true;
  }
  
}
