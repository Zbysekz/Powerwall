
//-----------------------------------------OTA------------------------------------------
void otaError(ota_error_t error)
{
  Serial.println(F("OTA Error."));
  switch (error) {
    case OTA_AUTH_ERROR:
      Serial.println("auth. error"); break;
    case OTA_BEGIN_ERROR:
      Serial.println("begin error"); break;
    case OTA_CONNECT_ERROR:
      Serial.println("connect error"); break;
    case OTA_RECEIVE_ERROR:
      Serial.println("receive error"); break;
    case OTA_END_ERROR:
      Serial.println("end error"); break;
    default:
      Serial.println("unknown error");
  }
}

void otaProgress(unsigned int actual, unsigned int maxsize)
{
  if (actual == maxsize - (maxsize / 10)) {
    Serial.println("OTA progress: 90%");
  }
  if (actual == maxsize) {
    Serial.println("OTA progress: 100%");
  }
}

void otaStart()
{
  Serial.println(F("OTA Start."));
}

void otaEnd()
{
  Serial.println(F("OTA End."));
}
//-----------------------------------------OTA------------------------------------------ END


//-----------------------------------------WIFI------------------------------------------
int WIFI_Send(uint8_t d[],uint8_t d_len){
  uint8_t data[6+d_len];
 
  data[0]=111;//start byte
  data[1]=222;//start byte

  data[2]=d_len;//length

  for(int i=0;i<d_len;i++)
    data[3+i]=d[i];//data
  
  int crc = 0;
  for(int i=0;i<d_len+1;i++)
    crc+=data[2+i];

  data[3+d_len]=crc/256;
  data[4+d_len]=crc%256;
  data[5+d_len]=222;//end byte

  return wifiClient.write(data,6+d_len);
}

void WIFI_ProcessReceived(){
  for(int i=0;i<RXQUEUESIZE;i++)
    if(rxBufferLen[i]>0){
      WIFI_ProcessReceived(rxBuffer[i]);
      rxBufferLen[i]=0;
    }
}

void WIFI_Receive(uint8_t rcv){
  
    switch(readState){
    case 0:
      if(rcv==111){readState=1;}//start token
      break;
    case 1:
      if(rcv==222)readState=2;else { Serial.write("ERR1");readState=0;}//second start token
      break;
    case 2:
      rxLen = rcv;//length
      if(rxLen>50){readState=0;Serial.write("ERR2");}else{ readState=3;
        rxPtr=0;
        crcReal=rxLen;
        //choose empty stack
        rxBufPtr=99;
        for(int i=0;i<RXQUEUESIZE;i++)
          if(rxBufferLen[i]==0)
            rxBufPtr=i;
        if(rxBufPtr==99){
          Serial.write("FULL BUFF!");
          readState=0;
        }
      }
      break;
    case 3:
      rxBuffer[rxBufPtr][rxPtr++] = rcv;//data
      crcReal+=rcv;
      if(rxPtr>=RXBUFFSIZE || rxPtr>=rxLen){
        readState=4;
      }
      break;
    case 4:
      crcH=rcv;//high crc
      readState=5;
      break;
    case 5:
      crcL=rcv;//low crc

      if(crcReal == crcL+(uint16_t)crcH*256){//crc check
        readState=6;
      }else {readState=0;Serial.write("CRC MISMATCH!");}
      break;
    case 6:
      if(rcv==222){//end token
        rxBufferLen[rxBufPtr]=rxLen;//set this packet to finished
        readState=0;
      }else readState=0;
      break;
    }
}
//-----------------------------------------WIFI------------------------------------------ END

//-----------------------------------------CELL MODULES-------------------------------------

void PrintModuleInfo(struct  cell_module *module) {
  Serial.print("Mod: ");
  Serial.print(module->address);
  Serial.print(" V:");
  Serial.print(module->voltage);
  Serial.print(" VC:");
  Serial.print(module->voltage_calib);
  Serial.print(" T:");
  Serial.print(module->temperature);
  Serial.print(" TC:");
  Serial.print(module->temperature_calib);
  Serial.print(" R:");
  Serial.print(module->loadResistance);
  Serial.println("");
}

void CheckModuleQuick(struct  cell_module *module) {//read voltage, temperature, and checks validity of voltage
  module->voltage = cell_read_voltage(module->address);
  module->temperature = cell_read_board_temp(module->address);

  if (module->voltage >= 0 && module->voltage <= 5000) {//valid values (in range)

    if ( module->voltage > module->max_voltage) {
      module->max_voltage = module->voltage;
    }
    if ( module->voltage < module->min_voltage) {
      module->min_voltage = module->voltage;
    }

    module->valid_values = true;
  } else {
    module->valid_values = false;
  }
}

void CheckModuleFull(struct  cell_module *module) {//same as quick, plus voltage and temp calibrations and load Resistance 
  CheckModuleQuick(module);
  module->voltage_calib = cell_read_voltage_calibration(module->address);
  module->temperature_calib = cell_read_temperature_calibration(module->address);
  module->loadResistance = cell_read_load_resistance(module->address);
}

void ScanI2CBus() {

  cellArray_index = 0;//points to one of cell modules, it is the next one in the queue to communicate with

  cellArray_cnt = 0;//reset number of cell modules

  //Scan the i2c bus looking for modules on start up
  for (uint8_t address = DEFAULT_SLAVE_ADDR_START_RANGE; address <= DEFAULT_SLAVE_ADDR_END_RANGE; address++ )
  {
    if (testModuleExists(address) == true) {
      //We have found a module
      cell_module m;
      m.address = address;
      //Default values
      m.valid_values = false;
      m.min_voltage = 0xFFFF;
      m.max_voltage = 0;
      cellArray[cellArray_cnt] = m;

      CheckModuleFull( &cellArray[cellArray_cnt] );

      //Switch off bypass if its on
      command_set_bypass_voltage(address, 0);

      PrintModuleInfo( &cellArray[cellArray_cnt] );

      cellArray_cnt++;
    }
  }
}
//-----------------------------------------CELL MODULES------------------------------------- END
