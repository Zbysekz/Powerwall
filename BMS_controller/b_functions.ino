
// ------------------------------------------- general -----------------------------------------------

//function for creating periodic block code
//returns true if period has passed, resetting the timer
bool CheckTimer(unsigned long &timer, const unsigned long period){
   if((unsigned long)(millis() - timer) >= period){//protected from rollover by type conversion
    timer = millis();
    return true;
   }else{
    return false;
   }
}

void ConnectBattery(){
  if(status_i2c==1 && status_eth==1 && modulesCount==REQUIRED_CNT_MODULES){
    digitalWrite(PIN_MAIN_RELAY, true);
  }
}

void DisconnectBattery(){
    digitalWrite(PIN_MAIN_RELAY, false);
}

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

  PrintModuleInfo(module);
}

void ReadModules(bool quick) {
  Serial.print(F("Reading all modules"));
  Serial.println(quick?F(" quickly"):F(" fully"));
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

uint8_t Provision() {//finding cell modules with default addresses

  Serial.println(F("Provisioning started"));
  if (PingModule(DEFAULT_SLAVE_ADDR))
  {
    Serial.println(F("Module with default address exists"));
    for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
    {
      if (PingModule(address) == false) {
        //We have found a gap
        command_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
        Serial.print(F("Successfuly assigned address:"));
        Serial.println(address);
        return address;
        break;
      }
    }
  } 

  return 0;
}



void ScanModules() {
  Serial.println(F("Scanning for modules"));
  modulesCount=0;
  for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
  {
    if (PingModule(address) == true) {
      Serial.print(F("Module discovered! Address:"));    
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
  Serial.print(F("Address: "));
  Serial.print(module->address);
  Serial.print(F(" V:"));
  Serial.print(module->voltage);
  Serial.print(F(" T:"));
  Serial.print(module->temperature);
  Serial.print(F(" VC:"));
  Serial.print(module->voltageCalib,3);
  Serial.print(F(" TC:"));
  Serial.print(module->temperatureCalib,3);
  Serial.println(F(""));
}

// END --------------------------------- Module operations ------------------------------------------ END

void Log(String str){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File logFile = SD.open("log.txt", (O_APPEND | O_WRITE));

  // if the file opened okay, write to it:
  if (logFile) {
    Serial.println(str);
    logFile.println(str);
    // close the file:
    logFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening log.txt!");
    iSDcardError = 2;
  }
}

void Log(int intStr){
  Log(String(intStr));
}
