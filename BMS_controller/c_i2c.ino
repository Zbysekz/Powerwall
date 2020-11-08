
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
  uint8_t ret = Wire.requestFrom((uint8_t)cell_id, (uint8_t)2);
  status_i2c = ret==2?1:2;//error code 2
  
  uint8_t high = Wire.read();
  uint8_t low = Wire.read();
  return word(high, low);
}

uint8_t read_uint8_t_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  uint8_t ret = Wire.requestFrom((uint8_t)cell_id, (uint8_t)1);
  status_i2c = ret==1?1:3;//error code 3
  
  return (uint8_t)Wire.read();
}

float read_float_from_cell(uint8_t cell_id, uint8_t cmd) {
  send_command(cell_id, cmd);
  uint8_t ret = Wire.requestFrom((uint8_t)cell_id, (uint8_t)4);

  status_i2c = ret==4?1:4;//error code 4
  
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
float cell_read_voltage_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_voltage_calibration);
}
float cell_read_temperature_calibration(uint8_t cell_id) {
  return read_float_from_cell(cell_id, read_temperature_calibration);
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
