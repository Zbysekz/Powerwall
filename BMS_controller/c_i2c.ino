
void ScanI2C() {
  Serial.println(F("Start scanning"));
  for (uint8_t address = 1; address <= 127; address++ )
  {
    if (PingModule(address) == true) {
      Serial.print(F("Found device! Address:"));    
      Serial.println(address);
    }
  }
  Serial.println(F("End scanning."));
}

bool Send_command(uint8_t cell_id, uint8_t cmd) {
  bool res = true;
  
  res = res && I2c._start()==0;
  res = res && I2c._sendAddress(SLA_W(cell_id))==0;
  res = res && I2c._sendByte(cmd)==0;
  res = res && I2c._stop()==0;
  
  return res;
}

bool Send_command(uint8_t cell_id, uint8_t cmd, uint8_t byteValue) {
  bool res = true;

  res = res && I2c._start()==0;
  res = res && I2c._sendAddress(SLA_W(cell_id))==0;
  res = res && I2c._sendByte(cmd)==0;
  res = res && I2c._sendByte(byteValue)==0;
  res = res && I2c._stop()==0;
  
  return res;
}

bool Send_command(uint8_t cell_id, uint8_t cmd, float floatValue) {

  float_to_bytes.val = floatValue;
  bool res = true;
  res = res && I2c._start()==0;
  res = res && I2c._sendAddress(SLA_W(cell_id))==0;
  res = res && I2c._sendByte(cmd)==0;
  res = res && I2c._sendByte(float_to_bytes.buffer[0])==0;
  res = res && I2c._sendByte(float_to_bytes.buffer[1])==0;
  res = res && I2c._sendByte(float_to_bytes.buffer[2])==0;
  res = res && I2c._sendByte(float_to_bytes.buffer[3])==0;
  res = res && I2c._stop()==0;
  
  return res;
}

bool Send_command(uint8_t cell_id, uint8_t cmd, uint16_t u16Value) {
  uint16_t_to_bytes.val = u16Value;
  bool res = true;
  res = res && I2c._start()==0;
  res = res && I2c._sendAddress(SLA_W(cell_id))==0;
  res = res && I2c._sendByte(cmd)==0;
  res = res && I2c._sendByte(uint16_t_to_bytes.buffer[0])==0;
  res = res && I2c._sendByte(uint16_t_to_bytes.buffer[1])==0;
  res = res && I2c._stop()==0;
 
}

uint8_t cmdByte(uint8_t cmd) {
  bitSet(cmd, COMMAND_BIT);
  return cmd;
}


bool Read_uint16_from_cell(uint8_t cell_id, uint8_t cmd, uint16_t &value) {
  if(!Send_command(cell_id, cmd))
    return false;
  uint8_t ret = I2c.read(cell_id, 2);

  if(ret == 0 && I2c.available()==2){
    uint8_t high = I2c.receive();
    uint8_t low = I2c.receive();
    value = word(high, low);
    return true;
  }else return false;
}

bool Read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd, uint8_t &value) {
  if(!Send_command(cell_id, cmd))
    return false;
  uint8_t ret = I2c.read(cell_id, 1);

  if(ret == 0 && I2c.available()==1){
    value = I2c.receive();
    return true;
  }else return false;
}

bool Read_float_from_cell(uint8_t cell_id, uint8_t cmd, float &value) {
  if(!Send_command(cell_id, cmd))
    return false;
  uint8_t ret = I2c.read(cell_id, 4);

  if(ret == 0 && I2c.available()==4){
    float_to_bytes.buffer[0] = I2c.receive();
    float_to_bytes.buffer[1] = I2c.receive();
    float_to_bytes.buffer[2] = I2c.receive();
    float_to_bytes.buffer[3] = I2c.receive();
    
    value = float_to_bytes.val;
    return true;
  }else return false;
}

//COMMANDS --------------------------------------------------------
bool Cell_green_led_default(uint8_t cell_id) {
  return Send_command(cell_id, cmdByte( COMMAND_green_led_default ));
}

bool Cell_green_led_pattern(uint8_t cell_id, uint8_t pattern) {
  return Send_command(cell_id, cmdByte( COMMAND_green_led_pattern ), pattern);
}
bool Cell_set_slave_address(uint8_t cell_id, uint8_t newAddress) {
  return Send_command(cell_id, cmdByte( COMMAND_set_slave_address ), newAddress);
}

bool Cell_set_voltage_calibration(uint8_t cell_id, float value) {
  return Send_command(cell_id, cmdByte(COMMAND_set_voltage_calibration ), value);
}
bool Cell_set_temperature_calibration(uint8_t cell_id, float value) {
  return Send_command(cell_id, cmdByte(COMMAND_set_temperature_calibration ), value);
}
bool Cell_set_bypass_voltage(uint8_t cell_id, uint16_t  value) {
  return Send_command(cell_id, cmdByte(COMMAND_set_bypass_voltage), value);
}

//READ --------------------------------------------------------------
bool Cell_read_voltage_calibration(uint8_t cell_id, float &value) {
  return Read_float_from_cell(cell_id, read_voltage_calibration, value);
}
bool Cell_read_temperature_calibration(uint8_t cell_id, float &value) {
  return Read_float_from_cell(cell_id, read_temperature_calibration, value);
}
bool Cell_read_voltage(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_voltage, value);
}
bool Cell_read_bypass_enabled_state(uint8_t cell_id, uint8_t &value) {
  return Read_uint8_t_from_cell(cell_id, read_bypass_enabled_state, value);
}

bool Cell_read_raw_voltage(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_raw_voltage, value);
}

bool Cell_read_error_counter(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_error_counter, value);
}

bool Cell_read_burning_counter(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_burning_counter, value);
}

bool Cell_read_board_temp(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_temperature, value);
}

bool Cell_read_bypass_voltage_measurement(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_bypass_voltage_measurement, value);
}

bool Cell_read_bypass_voltage_measurement(uint8_t cell_id, uint16_t &value) {
  return Read_uint16_from_cell(cell_id, read_bypass_voltage_measurement, value);
}
