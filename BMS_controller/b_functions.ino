
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

bool getSafetyConditions(bool printDetail){
   bool voltagesOk = true;

  for(int i=0;i<modulesCount;i++)
    if(moduleList[i].voltage <= LIMIT_VOLT_LOW || moduleList[i].voltage >= LIMIT_VOLT_HIGH){
      voltagesOk = false;
      break;
    }

  bool res = voltagesOk && status_eth==1 && modulesCount==REQUIRED_CNT_MODULES;
  
  if(!res && printDetail){//print reasons, only if not met
    Serial.print(F("\nSafety cond - voltages:"));
    Serial.print(voltagesOk);
    Serial.print(F(" eth:"));
    Serial.print(status_eth);
    Serial.print(F(" modCnt:"));
    Serial.println(modulesCount);
  }
  return res;
}

// ------------------------------------ Module operations -----------------------------------------------

bool ReadModuleQuick(struct  cell_module *module) {
  uint16_t voltage = 0, temperature = 0;
  bool res = true;
  
  res = Cell_read_voltage(module->address, voltage);
  res = res && Cell_read_board_temp(module->address, temperature);
  if(!res){
    Serial.print(F("\nFailed to read Mod.quick!"));
    Serial.println(module->address);
    if(++module->readErrCnt>=3)
      module->validValues = false;//comm failure must happen 3x times to consider values as invalid
    return false;
  }
  module->voltage = voltage;
  module->temperature = temperature;
  
  if (module->voltage >= 0 && module->voltage <= 500 && module->temperature > 0 && module->temperature < 600) {

    if ( module->voltage > module->maxVoltage ) {
      module->maxVoltage = module->voltage;
    }
    if ( module->voltage < module->minVoltage ) {
      module->minVoltage = module->voltage;
    }

    module->validValues = true;
    module->readErrCnt = 0;
  } else {
    module->validValues = false;
  }

  return true;
}

bool ReadModule(struct  cell_module *module) {
  float voltCalib = 0, tempCalib = 0;
  bool res = true;
  
  res = ReadModuleQuick(module);
  res = res && Cell_read_voltage_calibration(module->address, voltCalib);
  res = res && Cell_read_temperature_calibration(module->address, tempCalib);
  if(!res){
    Serial.print(F("\nFailed to read Module!"));
    Serial.println(module->address);
    return false;
  }
  module->voltageCalib = voltCalib;
  module->temperatureCalib = tempCalib;
  return true;
}

bool ReadModules(bool quick) {
  bool res = true;
  Serial.print(F("Reading all modules"));
  Serial.println(quick?F(" quickly"):F(" fully"));
  for(uint8_t i=0;i<modulesCount;i++){
    if(moduleList[i].address!=0){
      if(quick){
        res = res && ReadModuleQuick(&moduleList[i]);
        PrintModuleInfo(&moduleList[i], false);
      }
      else{
        res = res && ReadModule(&moduleList[i]);
        PrintModuleInfo(&moduleList[i], true);
      }
    }
  }
  return res;
}
bool PingModule(uint8_t address) { 
  bool ok = !(I2c._start() || I2c._sendAddress(SLA_W(address)) || I2c._stop());
  
  return ok;
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
        Cell_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
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

void PrintModuleInfo(struct  cell_module *module, bool withCal){
  Serial.print(F("Address: "));
  Serial.print(module->address);
  Serial.print(F(" V:"));
  Serial.print(module->voltage);
  Serial.print(F(" T:"));
  Serial.print(module->temperature);
  if(withCal){
    Serial.print(F(" VC:"));
    Serial.print(module->voltageCalib,3);
    Serial.print(F(" TC:"));
    Serial.print(module->temperatureCalib,3);
  }
  Serial.print(F(" Valid:"));
  Serial.print(module->validValues);
  Serial.println(F(""));
}

// END --------------------------------- Module operations ------------------------------------------ END
