
inline void ledON() {
  DDRB |= (1 << DDB1);
  PORTB |=  (1 << PB1);
}

inline void ledOFF() {
  DDRB |= (1 << DDB1);
  PORTB &= ~(1 << PB1);
}

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

void WriteConfigToEEPROM() {
  EEPROM.put(EEPROM_CONFIG_ADDRESS, currentConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, calculateCRC32((uint8_t*)&currentConfig, sizeof(cell_module_config)));
}

bool LoadConfigFromEEPROM() {
  cell_module_config restoredConfig;
  uint32_t existingChecksum;

  EEPROM.get(EEPROM_CONFIG_ADDRESS, restoredConfig);
  EEPROM.get(EEPROM_CHECKSUM_ADDRESS, existingChecksum);

  // Calculate the checksum of an entire buffer at once.
  uint32_t checksum = calculateCRC32((uint8_t*)&restoredConfig, sizeof(cell_module_config));

  if (checksum == existingChecksum) {
    //Clone the config into our global variable and return all OK
    memcpy(&currentConfig, &restoredConfig, sizeof(cell_module_config));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}


void factory_default() {
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, 0);
}

void Reboot() {
  TCCR1 = 0;
  TIMSK |= (1 << OCIE1A); //Disable timer1

  //Now power down loop until the watchdog timer kicks a reset
  ledON();

  /*
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    adc_disable();
    sleep_enable();
    sleep_cpu();
  */
  //Infinity
  while (1) {}
}


void sendUnsignedInt(uint16_t number) {
  TinyWireS.send((byte)((number >> 8) & 0xFF));
  TinyWireS.send((byte)(number & 0xFF));
}

void sendByte(uint8_t number) {
  TinyWireS.send(number);
}

void sendFloat(float number) {
  float_to_bytes.val = number;

  TinyWireS.send(float_to_bytes.b[0]);
  TinyWireS.send(float_to_bytes.b[1]);
  TinyWireS.send(float_to_bytes.b[2]);
  TinyWireS.send(float_to_bytes.b[3]);
}

float readFloat() {
  float_to_bytes.b[0] = TinyWireS.receive();
  float_to_bytes.b[1] = TinyWireS.receive();
  float_to_bytes.b[2] = TinyWireS.receive();
  float_to_bytes.b[3] = TinyWireS.receive();

  return float_to_bytes.val;
}
uint16_t readUINT16() {
  uint16_t_to_bytes.b[0] = TinyWireS.receive();
  uint16_t_to_bytes.b[1] = TinyWireS.receive();
  return uint16_t_to_bytes.val;
}

void bypass_off() {
  targetBypassVoltage = 0;
  bypassCnt = 0;
  bypassEnabled = false;
}


float getVoltageMeasurement() {
  //Oversampling and take average of ADC samples use an unsigned integer
  uint32_t sum = 0;
  for (int k = 0; k < OVERSAMPLE_LOOP; k++) {
    sum += voltageBuff[k];
  }
  //Shift the bits to match OVERSAMPLE_LOOP size (buffer size of 8=3 shifts, 16=4 shifts)
  //Assume perfect reference of 2560mV for reference - we will correct for this with voltageCalibration

  uint16_t raw = (uint16_t)(sum / OVERSAMPLE_LOOP);

  return (uint16_t)((float)raw * currentConfig.voltageCalibration + currentConfig.voltageCalibration2);
}
