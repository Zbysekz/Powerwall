
void scan();
uint8_t provision();
void PrintModuleInfo(struct  cell_module *module);
// ----------------------------------- TCP communication stack ---------------------------------------

int Send(uint8_t d[],uint8_t d_len){
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
void ProcessReceived(uint8_t data[]){
  switch(data[0]){//by ID
    case 0:
      Serial.println("Hello world!");
    break;
    case 1:
      ScanI2C();
    break;
    case 2:
      Provision();
    break;
  }
}

void ProcessReceived(){
  for(int i=0;i<RXQUEUESIZE;i++)
    if(rxBufferLen[i]>0){
      ProcessReceived(rxBuffer[i]);
      rxBufferLen[i]=0;
    }
}

void Receive(uint8_t rcv){
  
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
// END -------------------------------- TCP communication stack ------------------------------------ END


//------------------------------------- I2C -------------------------------------------------------------

uint8_t  send_command(uint8_t cell_id, uint8_t cmd) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t  send_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue) {
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(byteValue);  //Value
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t  send_command(uint8_t cell_id, uint8_t cmd, float floatValue) {
  float_to_bytes.val = floatValue;
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(float_to_bytes.buffer[0]);
  Wire.write(float_to_bytes.buffer[1]);
  Wire.write(float_to_bytes.buffer[2]);
  Wire.write(float_to_bytes.buffer[3]);
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t send_command(uint8_t cell_id, uint8_t cmd, uint16_t Value) {
  uint16_t_to_bytes.val = Value;
  Wire.beginTransmission(cell_id); // transmit to device
  Wire.write(cmd);  //Command configure device address
  Wire.write(uint16_t_to_bytes.buffer[0]);
  Wire.write(uint16_t_to_bytes.buffer[1]);
  uint8_t ret = Wire.endTransmission();  // stop transmitting
  return ret;
}

uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}


uint16_t read_uint16_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);
  return (word((uint8_t)Wire.read(), (uint8_t)Wire.read()));
}

uint8_t read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)1);
  return (uint8_t)Wire.read();
}

float read_float_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  i2cstatus = Wire.requestFrom((uint8_t)cell_id, (uint8_t)4);
  float_to_bytes.buffer[0] = (uint8_t)Wire.read();
  float_to_bytes.buffer[1] = (uint8_t)Wire.read();
  float_to_bytes.buffer[2] = (uint8_t)Wire.read();
  float_to_bytes.buffer[3] = (uint8_t)Wire.read();
  return float_to_bytes.val;
}

void clear_buffer() {
  while (Wire.available())  {
    Wire.read();
  }
}

uint8_t cell_green_led_default(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_green_led_default ));
}

uint8_t cell_green_led_pattern(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_green_led_pattern ), (uint8_t)202);
}

uint8_t command_factory_reset(uint8_t cell_id) {
  return send_command(cell_id, cmdByte( COMMAND_factory_default ));
}

uint8_t command_set_slave_address(uint8_t cell_id, uint8_t newAddress) {
  return send_command(cell_id, cmdByte( COMMAND_set_slave_address ), newAddress);
}

uint8_t command_set_voltage_calibration(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_voltage_calibration ), value);
}
uint8_t command_set_temperature_calibration(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_temperature_calibration ), value);
}
uint8_t command_set_load_resistance(uint8_t cell_id, float value) {
  return send_command(cell_id, cmdByte(COMMAND_set_load_resistance), value);
}
float cell_read_voltage_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_voltage_calibration);
}
float cell_read_temperature_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_temperature_calibration);
}
float cell_read_load_resistance(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_load_resistance);
}
uint16_t cell_read_voltage(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_voltage);
}
uint16_t cell_read_bypass_enabled_state(uint8_t cell_id) {
  return read_uint8_t_from_cell(cell_id, read_bypass_enabled_state);
}

uint16_t cell_read_raw_voltage(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_raw_voltage);
}

uint16_t cell_read_error_counter(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_error_counter);
}

uint16_t cell_read_board_temp(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_temperature);
}

uint16_t cell_read_bypass_voltage_measurement(uint8_t cell_id) {
  return read_uint16_from_cell(cell_id, read_bypass_voltage_measurement);
}

uint8_t command_set_bypass_voltage(uint8_t cell_id, uint16_t  value) {
  return send_command(cell_id, cmdByte(COMMAND_set_bypass_voltage), value);
}

//END ---------------------------------- I2C ------------------------------------------------------- END

// ------------------------------------ Module operations -----------------------------------------------

void ReadModuleQuick(struct  cell_module *module) {
  module->voltage = cell_read_voltage(module->address);
  module->temperature = cell_read_board_temp(module->address);

  if (module->voltage >= 0 && module->voltage <= 5000) {

    if ( module->voltage > module->maxVoltage ) {
      module->maxVoltage = module->voltage;
    }
    if ( module->voltage < module->minVoltage ) {
      module->minVoltage = module->voltage;
    }

    module->validValues = true;
  } else {
    module->validValues = false;
  }
}

void ReadModule(struct  cell_module *module) {
  ReadModuleQuick(module);
  module->voltageCalib = cell_read_voltage_calibration(module->address);
  module->temperatureCalib = cell_read_temperature_calibration(module->address);
  module->loadResistance = cell_read_load_resistance(module->address);

  PrintModuleInfo(module);
}
void ReadModules(bool quick) {
  for(uint8_t i=0;i<modulesCount;i++){
    if(moduleList[i].address!=0){
      if(quick)
        ReadModuleQuick(&moduleList[i]);
      else
        ReadModule(&moduleList[i]);
    }
  }
}
bool PingModule(uint8_t address) {  
  Wire.beginTransmission(address);
  byte error2 = Wire.endTransmission();
  if (error2 == 0)
  {
      return true;
  }

  return false;
}

uint8_t Provision() {

  Serial.println("Provisioning started");
  if (PingModule(DEFAULT_SLAVE_ADDR))
  {
    Serial.println("Module with default address exists");
    for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
    {
      if (PingModule(address) == false) {
        //We have found a gap
        command_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
        Serial.print("Successfuly assigned address:");
        Serial.println(address);
        return address;
        break;
      }
    }
  } 

  return 0;
}

void ScanI2C() {
  Serial.println("Start scanning");
  for (uint8_t address = 1; address <= 127; address++ )
  {
    if (PingModule(address) == true) {
      Serial.print("Found device! Address:");    
      Serial.println(address);
    }
  }
  Serial.println("End scanning.");
}
void ScanModules() {
  Serial.println("Scanning for modules");
  modulesCount=0;
  for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
  {
    if (PingModule(address) == true) {
      Serial.print("Module discovered! Address:");    
      Serial.println(address);
      
      moduleList[modulesCount].address = address;
      moduleList[modulesCount].validValues = false;
      moduleList[modulesCount].minVoltage = 0xFFFF;
      moduleList[modulesCount].maxVoltage = 0;
      
      modulesCount++;
    }
  }
  Serial.println("End scanning.");
}

void PrintModuleInfo(struct  cell_module *module){
  Serial.print("Address: ");
  Serial.print(module->address);
  Serial.print(" V:");
  Serial.print(module->voltage);
  Serial.print(" VC:");
  Serial.print(module->voltageCalib);
  Serial.print(" T:");
  Serial.print(module->temperature);
  Serial.print(" TC:");
  Serial.print(module->temperatureCalib);
  Serial.print(" R:");
  Serial.print(module->loadResistance);
  Serial.println("");
}

// END --------------------------------- Module operations ------------------------------------------ END



