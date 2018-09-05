void panic() {
  green_pattern = GREEN_LED_PANIC;
  inPanicMode = true;
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
  EEPROM.put(EEPROM_CONFIG_ADDRESS, myConfig);
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, calculateCRC32((uint8_t*)&myConfig, sizeof(cell_module_config)));
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
    memcpy(&myConfig, &restoredConfig, sizeof(cell_module_config));
    return true;
  }

  //Config is not configured or gone bad, return FALSE
  return false;
}



void LEDReset() {
  green_pattern = GREEN_LED_PATTERN_STANDARD;
}

void factory_default() {
  EEPROM.put(EEPROM_CHECKSUM_ADDRESS, 0);
}

void Reboot() {
  TCCR1 = 0;
  TIMSK |= (1 << OCIE1A); //Disable timer1

  //Now power down loop until the watchdog timer kicks a reset
  ledGreen();

  /*
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    adc_disable();
    sleep_enable();
    sleep_cpu();
  */
  //Infinity
  while (1) {}
}

void wait_for_buffer_ready() {
  //Just delay here so the buffers are all ready before we service i2c
  while (!buffer_ready) {
    delay(100);
    wdt_reset();
  }
}


void sendUnsignedInt(uint16_t number) {
  Wire.write((byte)((number >> 8) & 0xFF));
  Wire.write((byte)(number & 0xFF));
}

void sendByte(uint8_t number) {
  Wire.write(number);
}

void sendFloat(float number) {
  float_to_bytes.val = number;

  Wire.write(float_to_bytes.b[0]);
  Wire.write(float_to_bytes.b[1]);
  Wire.write(float_to_bytes.b[2]);
  Wire.write(float_to_bytes.b[3]);
}

float readFloat() {
  float_to_bytes.b[0] = Wire.read();
  float_to_bytes.b[1] = Wire.read();
  float_to_bytes.b[2] = Wire.read();
  float_to_bytes.b[3] = Wire.read();

  return float_to_bytes.val;
}

void bypass_off() {
  targetByPassVoltage = 0;
  ByPassCounter = 0;
  ByPassEnabled = false;
  green_pattern = GREEN_LED_PATTERN_STANDARD;
}

inline void ledGreen() {
  DDRB |= (1 << DDB1);
  PORTB |=  (1 << PB1);
}

inline void ledOff() {
  //Low
  DDRB |= (1 << DDB1);
  PORTB &= ~(1 << PB1);
}

void initADC()
{
  /* this function initialises the ADC
        ADC Prescaler Notes:
    --------------------
     ADC Prescaler needs to be set so that the ADC input frequency is between 50 - 200kHz.
           For more information, see table 17.5 "ADC Prescaler Selections" in
           chapter 17.13.2 "ADCSRA – ADC Control and Status Register A"
          (pages 140 and 141 on the complete ATtiny25/45/85 datasheet, Rev. 2586M–AVR–07/10)
           Valid prescaler values for various clock speeds
       Clock   Available prescaler values
           ---------------------------------------
             1 MHz   8 (125kHz), 16 (62.5kHz)
             4 MHz   32 (125kHz), 64 (62.5kHz)
             8 MHz   64 (125kHz), 128 (62.5kHz)
            16 MHz   128 (125kHz)
    ADPS2 ADPS1 ADPS0 Division Factor
    000 2
    001 2
    010 4
    011 8
    100 16
    101 32
    110 64
    111 128
  */

  //NOTE The device requries a supply voltage of 3V+ in order to generate 2.56V reference voltage.
  // http://openenergymonitor.blogspot.co.uk/2012/08/low-level-adc-control-admux.html

  //REFS1 REFS0 ADLAR REFS2 MUX3 MUX2 MUX1 MUX0
  //Internal 2.56V Voltage Reference without external bypass capacitor, disconnected from PB0 (AREF)
  //ADLAR =0 and PB3 (B0011) for INPUT (A3)
  //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
  ADMUX = B10010011;

  /*
    ADMUX = (INTERNAL2V56_NO_CAP << 4) |
          (0 << ADLAR)  |     // dont left shift ADLAR
          (0 << MUX3)  |     // use ADC3 for input (PB4), MUX bit 3
          (0 << MUX2)  |     // use ADC3 for input (PB4), MUX bit 2
          (1 << MUX1)  |     // use ADC3 for input (PB4), MUX bit 1
          (1 << MUX0);       // use ADC3 for input (PB4), MUX bit 0
  */

  /*
    #if (F_CPU == 1000000)
    //Assume 1MHZ clock so prescaler to 8 (B011)
    ADCSRA =
      (1 << ADEN)  |     // Enable ADC
      (0 << ADPS2) |     // set prescaler bit 2
      (1 << ADPS1) |     // set prescaler bit 1
      (1 << ADPS0);      // set prescaler bit 0
    #endif
  */
  //#if (F_CPU == 8000000)
  //8MHZ clock so set prescaler to 64 (B110)
  ADCSRA =
    (1 << ADEN)  |     // Enable ADC
    (1 << ADPS2) |     // set prescaler bit 2
    (1 << ADPS1) |     // set prescaler bit 1
    (0 << ADPS0) |       // set prescaler bit 0
    (1 << ADIE);     //enable the ADC interrupt.
  //#endif
}


float Update_VCCMillivolts() {
  //Oversampling and take average of ADC samples use an unsigned integer or the bit shifting goes wonky
  uint32_t extraBits = 0;
  for (int k = 0; k < OVERSAMPLE_LOOP; k++) {
    extraBits = extraBits + analogVal[k];
  }
  //Shift the bits to match OVERSAMPLE_LOOP size (buffer size of 8=3 shifts, 16=4 shifts)
  //Assume perfect reference of 2560mV for reference - we will correct for this with VCCCalibration

  uint16_t raw = (extraBits >> 4);
  //TODO: DONT THINK WE NEED THIS ANY LONGER!
  //unsigned int raw = map((extraBits >> 4), 0, 1023, 0, 2560);

  //TODO: Get rid of the need for float variables....
  return (int)((float)raw * myConfig.VCCCalibration);
}
