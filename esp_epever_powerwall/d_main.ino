void loop() {
  ArduinoOTA.handle();

  if(displayOk)
    HandleDisplay();
    
  ProcessReceived();

  if(epeverReadReq)
    HandleEpever(selected_device);

  if (WiFi.status() != WL_CONNECTED && connectedToWifi) {
    LOG("Connection to wifi was lost!");
    connectedToWifi=false;
  }
  
  //zkusíme se připojit k serveru pokud ještě nejsme
  if (!connectedToWifi && cntRetryConnect == 0) {
    LOG("Connecting to wifi");
    WiFi.begin(ssid, password);
    LOG("Waiting for connection result..");
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      LOG("Attempt of connection failed!");
    }else {
      connectedToWifi=true;
      LOG("Connected!");
    }
  }    

  if(!epeverReadReq && (millis()-tmrEpeverRead > 4000L)){
    epeverReadReq = true;
    tmrEpeverRead = millis();

    /*if(cnt_single_epever++>5){
      cnt_single_epever=0;
      epeverRegPtr=0;
      epeverRegPtr2=0;
      epeverStatus=0;
      if(selected_device++ >=EPEVER_MODULES){
        selected_device = 0;
      }
    }*/
  }

  //error evaluation - if we don't have valid data for few secs, report error
  if((millis()-tmrNotValidData > 10000L)){
    errorNotValidData = true;
    tmrNotValidData = millis();
  }

 if(connectedToWifi && (millis() - tmrSendToServer> 60000L)) {
  LOG("Sending to server...");
    //vytvorime spojeni, posleme data a precteme jestli nejaka neprisla, pak zavreme spojeni
    if (!wifiClient.connect(ipServer, 3666)) {
          LOG("Connection to server failed! IP:"+ipServer.toString());
          commOk=false;
          delay(100);
        } else {

        digitalWrite(2,false); // LED ON
        
        const int buff_size = 2+16 * EPEVER_MODULES; // 16 bytes
        uint8_t sbuf[buff_size];

        sbuf[0]=106;

        epever_statuses = 0;
        for(int i=0;i<EPEVER_MODULES;i++){
          epever_statuses |= epever_data[i].comm_status==1?(1<<i):0;
        }
        sbuf[1]=epever_statuses;
        
        uint8_t ptr = 2;
        for (int i=0;i<EPEVER_MODULES;i++){
          sbuf[ptr++] = (uint8_t)(epever_data[i].batVoltage/256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].batVoltage%256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].temperature/256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].temperature%256);
          sbuf[ptr++] = (uint8_t)((epever_data[i].batteryPower&0xFF000000)>>24);
          sbuf[ptr++] = (uint8_t)((epever_data[i].batteryPower&0x00FF0000)>>16);
          sbuf[ptr++] = (uint8_t)((epever_data[i].batteryPower&0x0000FF00)>>8);
          sbuf[ptr++] = (uint8_t)(epever_data[i].batteryPower&0x000000FF);
          sbuf[ptr++] = (uint8_t)((epever_data[i].generatedEnergy&0xFF000000)>>24);
          sbuf[ptr++] = (uint8_t)((epever_data[i].generatedEnergy&0x00FF0000)>>16);
          sbuf[ptr++] = (uint8_t)((epever_data[i].generatedEnergy&0x0000FF00)>>8);
          sbuf[ptr++] = (uint8_t)(epever_data[i].generatedEnergy&0x000000FF);
          sbuf[ptr++] = (uint8_t)(epever_data[i].batteryStatus/256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].batteryStatus%256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].chargerStatus/256);
          sbuf[ptr++] = (uint8_t)(epever_data[i].chargerStatus%256);
        }
        //push UART data to server
        int cnt = Send(sbuf,ptr);
        if(cnt<=0){
            LOG(F("Write failed!"));
          }else{
            LOG(F("Sent to server:"));
            LOG(cnt);      
          }

        tmrCommTimeout = millis();
        xServerEndPacket = false;
        while((millis() - tmrCommTimeout <4L*1000) && !xServerEndPacket){
          if(wifiClient.available()){
            uint8_t rcv = wifiClient.read();
            Receive(rcv);
          }
        }
        commOk = true;
      
      wifiClient.stop();

      digitalWrite(2,true); // LED OFF
    }
    tmrSendToServer = millis();
  }
  
}
