
int Send(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//length

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data

  uint16_t crc = CRC16(data+2, d_len+1);
  
  data[3+d_len]=uint8_t(crc/256);
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return ethClient.write(data,6+d_len);
}
void ProcessReceivedData(uint8_t data[]){
  int res=0;//aux temp
  int len = data[0];
  //uint16_t auxVal = 0;
  
  switch(data[1]){//by ID
    case 0://patern
      //Serial.print(F("\nLED pattern:"));
      Cell_green_led_pattern(data[2], data[3]);
        //Serial.println(F("failed"));
      //else
        //Serial.println(F("ok"));
    break;
    case 1:
      ScanI2C();
    break;
    case 2:
      Provision();
      //cant call ScanModules() here, because it is too fast, cell nedds some time to process
      xFullReadDone = false;
      tmrStartTime = millis();//use timer in main loop to make full scan
    break;
    case 3:
      ScanModules();
    break;
    case 4: // set voltage calibration to specific cell
      float_to_bytes.buffer[0]=data[3];
      float_to_bytes.buffer[1]=data[4];
      float_to_bytes.buffer[2]=data[5];
      float_to_bytes.buffer[3]=data[6];
      
      Serial.print(F("Calibrating voltage for module:"));
      Serial.println(data[2]);
      Serial.println(float_to_bytes.val,3);
      
      res = Cell_set_voltage_calibration(data[2],float_to_bytes.val);
      if(res){
        //Serial.println(F("Success"));
        ReadModules(false);//read all fully
      }/*else{
        Serial.print(F("Failed!"));
       }*/
    break;
    case 15: // set voltage calibration to specific cell
      float_to_bytes.buffer[0]=data[3];
      float_to_bytes.buffer[1]=data[4];
      float_to_bytes.buffer[2]=data[5];
      float_to_bytes.buffer[3]=data[6];
      
      //Serial.print(F("Calibrating voltage for module:"));
      //Serial.println(data[2]);
      //Serial.println(float_to_bytes.val,3);
      
      res = Cell_set_voltage_calibration2(data[2],float_to_bytes.val);
      if(res){
        //Serial.println(F("Success"));
        ReadModules(false);//read all fully
      }/*else{
        Serial.print(F("Failed!"));
       }*/
    break;
    case 5: // set temperature calibration to specific cell
      float_to_bytes.buffer[0]=data[3];
      float_to_bytes.buffer[1]=data[4];
      float_to_bytes.buffer[2]=data[5];
      float_to_bytes.buffer[3]=data[6];
      
      //Serial.print(F("Calibrating temperature for module:"));
      //Serial.println(data[2]);
      //Serial.println(float_to_bytes.val,3);
      
      res = Cell_set_temperature_calibration(data[2],float_to_bytes.val);
      if(res){
        //Serial.println(F("Success"));
        ReadModules(false);//read all fully
      }/*else{
        Serial.print(F("Failed!"));
       }*/
    break;
    case 16: // set temperature calibration to specific cell
      float_to_bytes.buffer[0]=data[3];
      float_to_bytes.buffer[1]=data[4];
      float_to_bytes.buffer[2]=data[5];
      float_to_bytes.buffer[3]=data[6];
      
      //Serial.print(F("Calibrating temperature for module:"));
      //Serial.println(data[2]);
      //Serial.println(float_to_bytes.val,3);
      
      res = Cell_set_temperature_calibration2(data[2],float_to_bytes.val);
      if(res){
        //Serial.println(F("Success"));
        ReadModules(false);//read all fully
      }/*else{
        Serial.print(F("Failed!"));
       }*/
    break;
    case 6:
      /*if(Cell_read_raw_voltage(data[2], auxVal)){
        //Serial.print(F("Read:"));
        Serial.println(auxVal);
      }*/
    break;
    case 7:
      if(len==3){
        Serial.println(F("Setting new address"));
        Cell_set_slave_address(data[2],data[3]);
      }
     break;
    case 10:
      xReqRun = true;
      Serial.println(F("RUN CMD"));
     break;
    case 11:
      xReqChargeOnly = true;
      Serial.println(F("CHARGE ONLY CMD"));
     break;
    case 12:
      xReqDisconnect = true;
      Serial.println(F("DISCONNECT CMD"));
      break;
    case 13:
      xReqErrorReset = true;
      Serial.println(F("RESET ERR CMD"));
      break;
    case 14:
      if(len==4){
        Cell_set_bypass_voltage(data[2],data[3]*256+data[4]);
        Serial.println(F("MANUALLY BURNING!"));
      }
      break;
    case 199://ending packet
      xServerEndPacket = true;
    break;
    default:
      Serial.println(F("Not defined command!"));
      
  }
}

void ProcessReceivedData(){
  for(int i=0;i<RXQUEUESIZE;i++)
    if(rxBufferMsgReady[i]==true){
      ProcessReceivedData(rxBuffer[i]);
      rxBufferMsgReady[i]=false;//mark as processed
    }
}

void Receive(uint8_t rcv){
    switch(readState){
    case 0:
      if(rcv==111){readState=1;}//start token
      break;
    case 1:
      if(rcv==222)readState=2;else { //second start token
        if(errorCnt_dataCorrupt<255)errorCnt_dataCorrupt++; 
        readState=0;
        Serial.println(F("RxErr0"));
      }
      break;
    case 2:
      rxLen = rcv;//length
      if(rxLen>RXBUFFSIZE){//should not be so long
        readState=0;
        if(errorCnt_dataCorrupt<255)errorCnt_dataCorrupt++;
        Serial.println(F("RxErr1")); 
      }else{ 
        readState=3;
        rxPtr=0;
        //choose empty stack
        rxBufPtr=99;
        for(gi=0;gi<RXQUEUESIZE;gi++){
          if(rxBufferMsgReady[gi]==false)
            rxBufPtr=gi;
        }
        if(rxBufPtr==99){
          if(errorCnt_BufferFull<255)errorCnt_BufferFull++;
           Serial.println(F("RxErr2"));
          readState=0;
        }else{
          rxBuffer[rxBufPtr][rxPtr++]=rxLen;//at the start is length
        }
      }
      break;
    case 3://receiving data itself
      rxBuffer[rxBufPtr][rxPtr++] = rcv;
      
      if(rxPtr>=RXBUFFSIZE || rxPtr>=rxLen+1){
        readState=4;
      }
      break;
    case 4:
      crcH=rcv;//high crc
      readState=5;
      break;
    case 5:
      crcL=rcv;//low crc

      if(CRC16(rxBuffer[rxBufPtr], rxLen+1) == crcL+(uint16_t)crcH*256){//crc check
        readState=6;
      }else {
        readState=0;//CRC not match
        if(errorCnt_CRCmismatch<255)errorCnt_CRCmismatch++;
        Serial.println(F("RxErr3"));
      }
      break;
    case 6:
      if(rcv==222){//end token
        rxBufferMsgReady[rxBufPtr]=true;//mark this packet as complete
      }else{
        Serial.println(F("RxErr4"));
        if(errorCnt_dataCorrupt<255)errorCnt_dataCorrupt++; 
      }
      readState=0;
      break;
    }
}

//calculation of CRC16, corresponds to CRC-16/XMODEM on https://crccalc.com/﻿
uint16_t CRC16(uint8_t* bytes, uint8_t _len)
{
    const uint16_t generator = 0x1021; /* divisor is 16bit */
    uint16_t crc = 0; /* CRC value is 16bit */

    for (int b=0;b<_len;b++)
    {
        crc ^= (uint16_t)(bytes[b] << 8); /* move byte into MSB of 16bit CRC */

        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x8000) != 0) /* test for MSb = bit 15 */
            {
                crc = (uint16_t)((crc << 1) ^ generator);
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void ExchangeCommunicationWithServer(){

    int cnt = 0;//aux var
    int retCon = ethClient.connect(ipServer, 23);
    if (retCon!=1) {
          Serial.print(F("Connection to server failed! error code:"));
          Serial.println(retCon);
          status_eth = 30;
    } else {
       status_eth = 1;//ok status

      if(xCalibDataRequested){// calibration data
          Serial.println(F("Sending calibration"));
          for(uint8_t i=0;i<modulesCount;i++){
            PrintModuleInfo(&moduleList[i], true);

            float_to_bytes.val = moduleList[i].voltageCalib;

            byte a = float_to_bytes.buffer[0];
            byte b = float_to_bytes.buffer[1];
            byte c = float_to_bytes.buffer[2];
            byte d = float_to_bytes.buffer[3];
            
            float_to_bytes.val = moduleList[i].temperatureCalib;

            byte e = float_to_bytes.buffer[0];
            byte f = float_to_bytes.buffer[1];
            byte g = float_to_bytes.buffer[2];
            byte h = float_to_bytes.buffer[3];
            
            sendBuff[0] = uint8_t(41+i);
            sendBuff[1] = uint8_t((moduleList[i].address-MODULE_ADDRESS_RANGE_START+1)&0xFF);
            sendBuff[2] = a;
            sendBuff[3] = b;
            sendBuff[4] = c;
            sendBuff[5] = d;
            sendBuff[6] = e;
            sendBuff[7] = f;
            sendBuff[8] = g;
            sendBuff[9] = h;
            
            cnt = Send(sendBuff,10);
            if(cnt<=0){
              Serial.println(F("Sending calibration failed!"));
            }
          }
      }else if(xReadyToSendStatistics){
          bool xFail = false;

          sendBuff[0] = 69;
          sendBuff[1] = uint8_t(iHeatingEnergyCons&0xFF);
          sendBuff[2] = uint8_t((iHeatingEnergyCons&0xFF00)>>8);
          sendBuff[3] = uint8_t(crcMismatchCounter);

          cnt = Send(sendBuff,4);
          if(cnt<=0){
            Serial.println(F("Sending statistics failed!"));
            xFail = true;
          }else{
            iHeatingEnergyCons = 0;
            crcMismatchCounter = 0;
          }
          
          for(uint8_t i=0;i<modulesCount;i++){
              float ff[1024];
              ff[0] = ff[22]+3+3;
              sendBuff[0] = uint8_t(71+i);
              sendBuff[1] = uint8_t((moduleList[i].address-MODULE_ADDRESS_RANGE_START+1)&0xFF);
              sendBuff[2] = uint8_t(((moduleList[i].iStatErrCnt)&0xFF00)>>8);
              sendBuff[3] = uint8_t((moduleList[i].iStatErrCnt)&0xFF);
              sendBuff[4] = uint8_t(((moduleList[i].iBurningCnt)&0xFF00)>>8);
              sendBuff[5] = uint8_t((moduleList[i].iBurningCnt)&0xFF);
  
              cnt = Send(sendBuff,6);
              if(cnt<=0){
                Serial.println(F("Sending statistics failed!"));
                xFail = true;
                break;//do not continue if one of them failed
              } 
          }
          if(!xFail)
            xReadyToSendStatistics=false;
          
        }else{ // normal data
          SendStatus(); 
          if(CheckTimer(tmrSendData, 60000L)){
            for(uint8_t i=0;i<modulesCount;i++){
              if(moduleList[i].validValues){
                
                sendBuff[0] = uint8_t(11+i);
                sendBuff[1] = uint8_t((moduleList[i].address-MODULE_ADDRESS_RANGE_START+1)&0xFF);
                sendBuff[2] = uint8_t(((moduleList[i].voltage_avg)&0xFF00)>>8);
                sendBuff[3] = uint8_t((moduleList[i].voltage_avg)&0xFF);
                sendBuff[4] = uint8_t(((moduleList[i].temperature_avg)&0xFF00)>>8);
                sendBuff[5] = uint8_t((moduleList[i].temperature_avg)&0xFF);
                
                cnt = Send(sendBuff,6);
                if(cnt<=0){
                  Serial.println(F("Sending data failed!"));
                } 
              }
            }
          }
      }
        tmrCommTimeout = millis();
        xServerEndPacket = false;
        while((millis() - tmrCommTimeout <4L*1000) && !xServerEndPacket){
          wdt_reset();
          if(ethClient.available()){
            uint8_t rcv = ethClient.read();
            Receive(rcv);
            //Serial.println(rcv);
          }
        }
  
      ethClient.stop();
    }
}

void SendEventStart(){
  sendBuff[0] = 1;
  sendBuff[1] = 0;
  
  int cnt_ = Send(sendBuff,2);
  if(cnt_<=0){
    Serial.println(F("Sending status failed!"));
  } 
}

void SendStatus(){
  sendBuff[0] = 10;
  sendBuff[1] = stateMachineStatus;
  sendBuff[2] = errorStatus;
  sendBuff[3] = errorStatus_cause;
  sendBuff[4] = ((uint8_t)xHeating<<1) | ((uint8_t)solarConnected<<0); // various states
  sendBuff[5] = (uint8_t)errorWhichModule;
  
  int cnt_ = Send(sendBuff,6);
  if(cnt_<=0){
    Serial.println(F("Sending status failed!"));
  } 
}
