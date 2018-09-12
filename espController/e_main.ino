void loop() {

  if(OTAmode){
    ArduinoOTA.handle();
    return;
  }

  if(!connectedToWifi){//Try to connect to server if not yet connected
    Serial.println("Connecting to wifi");
    WiFi.begin(ssid, password);
    Serial.println("Waiting for connection result..");

    unsigned long wifiConnectStart = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        // Check to see if
        if (WiFi.status() == WL_CONNECT_FAILED) {
          Serial.println("Failed to connect to WiFi");
          //ESP.deepSleep(SLEEP_PERIOD);
        }
    
        delay(500);
        // Only try for 15 seconds.
        if (millis() - wifiConnectStart > 15000) {
          Serial.println("Failed to connect to WiFi");
          delay(20000);//then wait for 20secs and try again
          break;
        }
    }
    Serial.println("Connected!");    
    connectedToWifi=true;
  }

    
  if(connectedToWifi){
    //Create connection, send data and check if any data were received, then close connection
    if (!wifiClient.connect(ipServer, 23)) {
          Serial.println("Connection to server failed! IP:"+ipServer.toString());
        } else {
        uint8_t sbuf[] = {101,int(temp*100)/256, int(temp*100)%256,(int(pressure*100)&0xFF0000)>>16,(int(pressure*100)&0xFF00)>>8,int(pressure*100)&0xFF,(int(supplyVoltage)&0xFF00)>>8,int(supplyVoltage)&0xFF};

        int cnt = WIFI_Send(sbuf,8);
        if(cnt<=0){
          Serial.println("Write failed!");
        }

        delay(50);
        while(wifiClient.available()){
          uint8_t rcv = wifiClient.read();
          WIFI_Receive(rcv);
          Serial.write(rcv);
        }
            
        }
        wifiClient.stop();
  }

  delay(5000);
}
void timer1Callback(void *pArg) {//called every 0,5s

  if (runProvisioning) {
    Serial.println("Provisioning is started");
    uint8_t newAddress = Provision();

    if (newAddress > 0) {
      Serial.print("Found ");
      Serial.println(newAddress);

      cellArray[cellArray_cnt].address = newAddress;
      cellArray[cellArray_cnt].min_voltage = 0xFFFF;
      cellArray[cellArray_cnt].max_voltage = 0;
      cellArray[cellArray_cnt].bypassVoltage = 0;
      cellArray[cellArray_cnt].valid_values = false;

      //Dont attempt to read here as the module will be rebooting
      //check_module_quick( &cellArray[cellArray_max] );
      cellArray_cnt++;
    }

    runProvisioning = false;
    return;
  }



  //Execute this routine for every cell in array, sequentially (one module per cycle)
  if (cellArray_cnt > 0 && cellArray_index >= 0) {//Ensure we have some cell modules to check
    if (cellArray[cellArray_index].update_calibration) {  
      if (cellArray[cellArray_index].factoryReset) {
        Serial.print("Factory reset for ");
        Serial.println(cellArray_index);
        command_factory_reset(cellArray[cellArray_index].address);
      } else {
        Serial.print("Updating calibration for ");
        Serial.println(cellArray_index);
        //Check to see if we need to configure the calibration data for this module
        command_set_voltage_calibration(cellArray[cellArray_index].address, cellArray[cellArray_index].voltage_calib);
        command_set_temperature_calibration(cellArray[cellArray_index].address, cellArray[cellArray_index].temperature_calib);

        command_set_load_resistance(cellArray[cellArray_index].address, cellArray[cellArray_index].loadResistance);
      }
      cellArray[cellArray_index].update_calibration = false;
    }

    CheckModuleQuick( &cellArray[cellArray_index] );

    if (cellArray[cellArray_index].bypassVoltage > 0) {
      Serial.print("Balancing target for ");
      Serial.println(cellArray_index);
      command_set_bypass_voltage(cellArray[cellArray_index].address, cellArray[cellArray_index].bypassVoltage);
      cellArray[cellArray_index].bypassVoltage = 0;
    }

    cellArray_index++;
    if (cellArray_index >= cellArray_cnt) {
      cellArray_index = 0;
    }
  }
  
}
void WIFI_ProcessReceived(uint8_t data[]){
  switch(data[0]){//by ID
    case 0:
      Serial.println("Hello world!");
    break;
    case 1:
      Serial.println("Command received:");
     
      Serial.print(data[1],DEC);
      Serial.print(data[2],DEC);
    break;
  }
}
