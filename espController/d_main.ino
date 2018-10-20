void HandleDisplay(){
  
  display.clearDisplay();
  display.setCursor(0,0);
  
  display.print("Wifi comm:");
  if(connectedToWifi)display.println("ok");else display.println("error");
  display.print("Server comm:");
  if(serverCommState)display.println("ok");else display.println("error");
  display.print("I2C status:");
  if(i2cstatus!=0)display.println("ok");else display.println("error");
  display.print("No. of modules:");
  display.println(modulesCount);

  display.display();
}
void loop() {
  
  if (WiFi.status() != WL_CONNECTED && connectedToWifi) {
    Serial.println("Connection to wifi was lost!");
    connectedToWifi=false;
  }
  
  //try to connect to server if we are not connected yet
  if (!connectedToWifi && cntRetryConnect == 0) {
    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, password);
    Serial.println("Waiting for connection result..");
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Attempt of connection failed!");
    }else {
      connectedToWifi=true;
      Serial.println("Connected!");
    }
  }

  HandleDisplay();

  if(cntScanModules == 0 ){
    ReadModules(true);//quick read
  }

  if (connectedToWifi && cntCommData == 0 ) {

   //Create connection, send data and check if any data were received, then close connection
    if (!wifiClient.connect(ipServer, 23)) {
          Serial.println("Connection to server failed! IP:"+ipServer.toString());
          serverCommState=false;
        } else {
          serverCommState=true;

        for(int i=0;i<modulesCount;i++){
          PrintModuleInfo(&moduleList[i]);
          /*uint8_t sbuf[] = {51+i,(moduleList[i].address)&0xFF,((moduleList[i].voltage)&0xFF00)>>8, (moduleList[i].voltage)&0xFF,((moduleList[i].temperature)&0xFF00)>>8, (moduleList[i].temperature)&0xFF};

          int cnt = Send(sbuf,8);
          if(cnt<=0){
            Serial.println("Write failed!");
          }*/
        }

        delay(50);
        while(wifiClient.available()){
          uint8_t rcv = wifiClient.read();
          Receive(rcv);
        }
            
    }
    wifiClient.stop();
  }

  ProcessReceived();


  
  if(cntCommData>0)
    cntCommData--;
  else
    cntCommData=10;

  if(cntScanModules>0)
    cntScanModules--;
  else
    cntScanModules=5;

  if(cntDelayAfterStart>50){//do it only once 5sec after start
    if(cntDelayAfterStart!=99){
      ScanModules();
      delay(50);
      ReadModules(false);//full read
      cntDelayAfterStart=99;
    }
  }else cntDelayAfterStart++;
    
  delay(100);
}
