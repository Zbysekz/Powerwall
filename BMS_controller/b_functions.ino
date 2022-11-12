
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

bool getSafetyConditions(){
   voltagesOk = true;
   validValues = true;
   temperaturesOk = true;
   errorWhichModule = 99;  
   oneOfCellIsLow = false;
   oneOfCellIsHigh = false;
   
  for(int i=0;i<modulesCount;i++){
    if(!moduleList[i].validValues){
      validValues = false;
      break;
    }
    if(moduleList[i].voltage_avg >= LIMIT_VOLT_HIGH2){
      voltagesOk = false;
      errorWhichModule = i+20;
    }else if(moduleList[i].voltage_avg >= LIMIT_VOLT_HIGH){
      oneOfCellIsHigh = true;
    }
    
    if(moduleList[i].voltage_avg <= LIMIT_VOLT_LOW2){
      voltagesOk = false;
      errorWhichModule = i;
    }else if(moduleList[i].voltage_avg <= LIMIT_VOLT_LOW){//will go from run to charge if in run
      oneOfCellIsLow = true;
    }
    
    if(moduleList[i].temperature_avg <= LIMIT_RACK_TEMPERATURE_LOW || moduleList[i].temperature_avg >= LIMIT_RACK_TEMPERATURE_HIGH){
      temperaturesOk = false;
      errorWhichModule = i;
    }
  }

  bool res = validValues && voltagesOk && modulesCount==REQUIRED_CNT_MODULES && temperaturesOk;
  

  if(!res){
    if(!validValues)
      errorStatus|=ERROR_I2C;
    if(modulesCount!=REQUIRED_CNT_MODULES)
      errorStatus|=ERROR_MODULE_CNT;
    if(!voltagesOk)
      errorStatus|=ERROR_VOLTAGE_RANGES;
    if(!temperaturesOk)
      errorStatus|=ERROR_TEMP_RANGES;
      
  }else{
    errorStatus = 0; // all ok
  }

  return res;
}

// ------------------------------------ Module operations -----------------------------------------------

bool ReadModuleQuick(struct  cell_module *module) {
  uint16_t voltage = 0, temperature = 0;
  uint8_t CRCVoltTemp = 0, xBurning;
  bool res;

  //Serial.println("\nQuickRead:");
  //Serial.println(module->address);
  uint8_t trials = 3;

  while(trials>0){
    res = Cell_read_voltage(module->address, voltage);
    res = res && Cell_read_board_temp(module->address, temperature);
    res = res && Cell_read_CRC(module->address, CRCVoltTemp);
    res = res && Cell_read_bypass_enabled_state(module->address, xBurning);

    uint8_t realCRC = uint8_t(voltage&0xFF)+uint8_t((voltage&0xFF00) >> 8) + uint8_t(temperature&0xFF)+uint8_t((temperature&0xFF00) >> 8);
    
    if (res && CRCVoltTemp != realCRC){
      Log("\nCRC mismatch in read Mod.quick!");
      Log(realCRC);
      Log(CRCVoltTemp);
      crcMismatchCounter ++;

      res = false;
    }
    if(res)break;
    trials--;
  }
  
  
  if(!res){
    Log("\nFailed to read Mod.quick!");
    Log(module->address);
    if(!Cell_resetI2c(module->address)){//reset module I2c command
      Log("\nFailed to reset I2C of module:");
      Log(module->address);
    }
    
    if(++(module->readErrCnt)>=3)
      module->validValues = false;//comm failure must happen 3x times to consider values as invalid
    return false;
  }
  
  //limit values to prevent affecting average too much - in case of one fail value
  voltage = min(500,voltage);
  temperature = min(700,temperature);
  //if (voltage <= 500 && temperature > 0 && temperature < 600) {

    module->voltage = voltage;
    // calculation of avg value///////////////
    module->voltage_buff[(module->voltAvgPtr)++] = module->voltage;
    if(module->voltAvgPtr>=VOLT_AVG_SAMPLES)
      module->voltAvgPtr = 0;

    module->voltage_avg = 0;
    for(int i=0;i<VOLT_AVG_SAMPLES;i++)
      module->voltage_avg+=module->voltage_buff[i];
    module->voltage_avg/=VOLT_AVG_SAMPLES;

    ////////////////////////////////////////

    module->temperature = temperature;
    // calculation of avg value///////////////
    module->temperature_buff[(module->tempAvgPtr)++] = module->temperature;
    if(module->tempAvgPtr>=TEMP_AVG_SAMPLES)
      module->tempAvgPtr = 0;

    module->temperature_avg = 0;
    for(int i=0;i<TEMP_AVG_SAMPLES;i++)
      module->temperature_avg+=module->temperature_buff[i];
    module->temperature_avg/=TEMP_AVG_SAMPLES;

    ////////////////////////////////////////
    
    
    //update min and max values
    /*if ( module->voltage > module->maxVoltage ) {
      module->maxVoltage = module->voltage;
    }
    if ( module->voltage < module->minVoltage ) {
      module->minVoltage = module->voltage;
    }*/

    module->burning = xBurning;

    module->validValues = true;
    module->readErrCnt = 0;
    return true;
  /*} else {
    Log("Resetting module I2C !!!!");
    i!Cell_resetI2c(module->address)){//reset module I2c command
      Log("\nFailed to reset I2C of module:");
      Log(module->address);
    }
    i++module->readErrCnt>=3)
      module->validValues = false;//comm failure must happen 3x times to consider values as invalid
    return false;
  }*/

  //return false;
}

bool ReadModule(struct  cell_module *module) {
  float voltCalib = 0, tempCalib = 0, voltCalib2 = 0, tempCalib2 = 0;
  bool res = true;
  
  res = ReadModuleQuick(module);
  if(res)
    res = res && Cell_read_voltage_calibration(module->address, voltCalib, voltCalib2);
  if(res)
    res = res && Cell_read_temperature_calibration(module->address, tempCalib, tempCalib2);
  if(!res){
    Log("\nFailed to read Module!");
    Log(module->address);
    return false;
  }
  module->voltageCalib = voltCalib;
  module->temperatureCalib = tempCalib;
  module->voltageCalib2 = voltCalib2;
  module->temperatureCalib2 = tempCalib2;
  return true;
}

bool ReadModules(bool quick) {
  bool res = true;
  Serial.println("Reading all modules");
  Serial.println(quick?" quickly":" fully");
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

  Log("Provisioning started");
  if (PingModule(DEFAULT_SLAVE_ADDR))
  {
    Log("Module with default address exists");
    for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
    {
      if (PingModule(address) == false) {
        //We have found a gap
        Cell_set_slave_address(DEFAULT_SLAVE_ADDR, (uint8_t)address);
        Log("Successfuly assigned address:");
        Log(address);
        return address;
      }
    }
  } 

  return 0;
}



void ScanModules() {
  Log("Scanning for modules");
  modulesCount=0;
  for (uint8_t address = MODULE_ADDRESS_RANGE_START; address <= MODULE_ADDRESS_RANGE_END; address++ )
  {
    if (PingModule(address) == true) {
      Log("Module discovered! Address:");    
      Log(address);
      
      moduleList[modulesCount].address = address;
      moduleList[modulesCount].validValues = false;
      //moduleList[modulesCount].minVoltage = 0xFFFF;
      //moduleList[modulesCount].maxVoltage = 0;
      
      modulesCount++;
    }
  }
  Log("End scanning.");
}

void PrintModuleInfo(struct  cell_module *module, bool withCal){
  Serial.println("Address: ");
  Serial.println(module->address);
  Serial.println(" V:");
  Serial.println(module->voltage);
  Serial.println(" T:");
  Serial.println(module->temperature);
  if(withCal){
    Serial.println(" VC:");
    Serial.println(module->voltageCalib);
    Serial.println(",");
    Serial.println(module->voltageCalib2);
    Serial.println(" TC:");
    Serial.println(module->temperatureCalib);
    Serial.println(",");
    Serial.println(module->temperatureCalib2);
  }
  Serial.println(" Valid:");
  Serial.println(module->validValues);
}

// END --------------------------------- Module operations ------------------------------------------ END
void Log(float f){
  char str[16];  
  sprintf(str, "%.2f", (double)f);
  Log(str);
}
void Log(uint16_t integer){
  Log((int)integer);// print to the serial port too:// print to the serial// print to the serial port too:
}
void Log(int integer){
  char str[16];
  sprintf(str, "%d", integer);
  Log(str);
}

void Log(const char *str){
  // print to the serial port too:
  Serial.println(str);
// open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  String filename = String(SDcardFileIndex);
  File dataFile = SD.open(filename+".log", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(str);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening log file");
  }
}
