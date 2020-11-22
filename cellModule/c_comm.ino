//cellModule -> cellController
// function that executes whenever data is requested by master (this answers requestFrom command)
void requestEvent() {
  switch (cmdByte) {
    case READOUT_voltage:
      if (bypassEnabled) {
        sendUnsignedInt(voltageMeasurement_bypass);
      } else {
        sendUnsignedInt(voltageMeasurement);
      }

      break;
    case READOUT_bypass_voltage_measurement:
      sendUnsignedInt(voltageMeasurement_bypass);
      break;
      
    case READOUT_raw_voltage:
      sendUnsignedInt(last_raw_adc);
      break;

    case READOUT_error_counter:
      sendUnsignedInt(error_counter);
      break;

    case READOUT_bypass_state:
      sendByte(bypassEnabled);
      break;

    case READOUT_temperature:
      sendUnsignedInt((uint16_t)((float)tempSensorValue * currentConfig.tempSensorCalibration));
      break;

    case READOUT_voltage_calibration:
      sendFloat(currentConfig.voltageCalibration);
      break;

    case READOUT_temperature_calibration:
      sendFloat(currentConfig.tempSensorCalibration);
      break;
    case READOUT_burningCounter:
      sendUnsignedInt(iBurningCounter);
      iBurningCounter = 0;//resest after it is read
      break;

    default:
      //Dont do anything - timeout
      break;
  }

  //Clear cmdByte
  cmdByte = 0;

  //if this times out, master has stopped communicating with module
  i2cTmr = 80;//150
}


//cellController -> cellModule
/**
 * The I2C data received handler
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly
 */
void receiveEvent(uint8_t bytesCnt) {
  if (bytesCnt <= 0) return;

  //If cmdByte is not zero then something went wrong in the i2c comms,
  //master failed to request any data after command
  if (cmdByte != 0) error_counter++;

  cmdByte = TinyWireS.receive();
  bytesCnt--;

  //Is it a command
  if (bitRead(cmdByte, COMMAND_BIT)) {

    bitClear(cmdByte, COMMAND_BIT);

    switch (cmdByte) {
      case COMMAND_green_led_pattern:
        if (bytesCnt == 1) {
          green_pattern = TinyWireS.receive();
        }
        break;

      case COMMAND_green_led_default:
        green_pattern = GREEN_LED_PATTERN_STANDARD;
        break;

      case COMMAND_factory_default:
        factory_default();
        Reboot();
        break;

      case COMMAND_set_voltage_calibration:
        if (bytesCnt == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != currentConfig.voltageCalibration) {
            currentConfig.voltageCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_temperature_calibration:
        if (bytesCnt == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != currentConfig.tempSensorCalibration) {
            currentConfig.tempSensorCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_bypass_voltage:
        if (bytesCnt == sizeof(uint16_t)) {
          uint16_t newValue = readUINT16();
          //Only accept if its a realistic value and the value is LESS than the last voltage reading
          if (newValue >= MIN_BYPASS_VOLTAGE && newValue <= MAX_BYPASS_VOLTAGE && newValue < voltageMeasurement) {
            targetBypassVoltage = newValue;
            voltageMeasurement_bypass = voltageMeasurement;//init bypass voltage
            bypassCnt = BYPASS_COUNTER_MAX;
            green_pattern = GREEN_LED_PATTERN_BYPASS;
            bypassEnabled = true;
          } else {
            //Disable
            bypass_off();
            green_pattern = GREEN_LED_PATTERN_STANDARD;
          }
        }
        break;


      case COMMAND_set_slave_address:
        //Set i2c slave address and write to EEPROM, then reboot
        if (bytesCnt == 1 ) {
          uint8_t newAddress = TinyWireS.receive();
          //Only accept if its a different address
          if (newAddress != currentConfig.SLAVE_ADDR && newAddress >= SLAVE_ADDR_START_RANGE && newAddress <= SLAVE_ADDR_END_RANGE) {
            currentConfig.SLAVE_ADDR = newAddress;
            WriteConfigToEEPROM();
            Reboot();
          }
        }
        break;

    }

    cmdByte = 0;
  } else {
    //Its a READ request

    switch (cmdByte) {
      case READOUT_voltage:
        voltageMeasurement = getVoltageMeasurement();
        ledFlash = true;
        break;
    }
  }

}
