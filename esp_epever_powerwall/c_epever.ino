
void Epever_SendRequest(uint16_t address, uint8_t device_id){
  uint8_t buff[10];

  buff[0] = device_id+1;
  buff[1] = 0x04;
  buff[2] = uint8_t((address&0xFF00)>>8);
  buff[3] = uint8_t(address&0xFF);
  buff[4] = 0x00;
  buff[5] = 0x01;
  uint16_t crc = CRC16_modbus(buff,6);
  buff[6] = uint8_t(crc&0xFF);
  buff[7] = uint8_t((crc&0xFF00)>>8);


  for(int i=0;i<8;i++){
    Serial.write(buff[i]);
  }
}



void Epever_Receive(uint8_t device_id){

  while(Serial.available()>0){
    if(serial_rxPtr<SERIAL_RX_BUFF_SIZE){
      serial_rcvBuff[serial_rxPtr] = (uint8_t)Serial.read();
      if(serial_rxPtr==0){
        if(serial_rcvBuff[serial_rxPtr]==device_id+1){
          serial_rxPtr++; // go further only if it is for you
        }
      }else{
        serial_rxPtr++;
      }
    }
  }
   
  if(serial_rxPtr>=7){
    serial_rxPtr = 0;
    if(serial_rcvBuff[0]==(device_id+1) && serial_rcvBuff[1]==0x04 && serial_rcvBuff[2]==0x02){
      crc = CRC16_modbus(serial_rcvBuff,5);

      if(crc==(uint16_t)(serial_rcvBuff[6])*256 + serial_rcvBuff[5]){
        rxValue = (uint16_t)(serial_rcvBuff[3])*256 + serial_rcvBuff[4];
        epeverDataValid = true;
        return;
      }else{
        crcErrorCnt++;
        epeverStatus = 0;}
    }
  }
}
void EpeverValueReceived(uint32_t val, uint8_t device_id){

  switch(epeverRegPtr){//store what we wanted to read
    case 0:
      epever_data[device_id].batVoltage = val;
    break;
    case 1:
     epever_data[device_id].temperature = val;
    break;
    case 2:
      epever_data[device_id].batteryPower = val;
    break;
    case 3:
      epever_data[device_id].generatedEnergy = val;
    break;
    case 4:
      epever_data[device_id].batteryStatus = val;
    break;
    case 5:
      epever_data[device_id].chargerStatus = val;
    break;
    
  }
  //shift to next read register
  if(epeverRegPtr<EPEVER_READ_REG_WORD_LEN-1){
    
    if(epeverReadRegisters_word[epeverRegPtr])
      epeverRegPtr2+=2; 
    else 
      epeverRegPtr2++;

    epeverRegPtr2 = epeverRegPtr2 < EPEVER_READ_REG_LEN ? epeverRegPtr2 : EPEVER_READ_REG_LEN-1;//just ensure check
    epeverRegPtr++;
    
  }else{
    epeverRegPtr=0;//start over again
    epeverRegPtr2=0;
    epever_data[device_id].comm_status = 1; // ok
    epeverReadReq = false;
    tmrNotValidData = millis();
    errorNotValidData = false;
  }
}
void HandleEpever(uint8_t device_id){
  
    switch(epeverStatus){
       case 0:
        tmrTimeout = millis();
        if(millis()-tmrRequest > 50L){
          tmrRequest = millis();
          Epever_SendRequest(epeverReadRegisters[epeverRegPtr2], device_id);
          epeverDataValid = false;
          epeverStatus = 1;
        }
       break;
       case 1:
        if(epeverDataValid) {
          epeverDataValid = false;

          if(epeverReadRegisters_word[epeverRegPtr]){//it will be double word, we need one more request
            tempRxValue = rxValue;
            Epever_SendRequest(epeverReadRegisters[epeverRegPtr2+1], device_id);
            epeverStatus=2;
          }else{
            EpeverValueReceived(rxValue, device_id);//it is byte
            epeverStatus=0; 
          }
        }
        
       break;
       case 2:
        if(epeverDataValid) {//double word is complete
          EpeverValueReceived(tempRxValue + rxValue*65536L, device_id);
          epeverStatus=0;
        }
        
       break;
    }

    if(epeverStatus!=0 && (millis()-tmrTimeout > 400L)){//timeout happened in some step
       epeverStatus = 0;
       epeverDataValid = false;
       serial_rxPtr = 0;
       timeoutReq ++;
       epever_data[device_id].comm_status = 2; // timeout
    }

  Epever_Receive(device_id);
}
