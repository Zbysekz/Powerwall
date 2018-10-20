#include "d_main.h"

void loop() {
  wdt_reset();

  if (last_i2c_request > 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    last_i2c_request--;
  }

  //If we are on the default SLAVE address signalize it
  if (currentConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
  } else {
    
    if (last_i2c_request == 0 && inPanicMode == false) {//go to panic mode, when timeout of I2C requests

      previousLedState = green_pattern ;
      green_pattern = GREEN_LED_PANIC;
      inPanicMode = true;
      //Try resetting the i2c bus
      Wire.end();
      init_i2c();

      error_counter++;
    }

    if (last_i2c_request > 0 && inPanicMode == true) {//return from panic mode
      green_pattern = GREEN_LED_PATTERN_STANDARD;
      green_pattern = previousLedState;
      inPanicMode = false;
    }

  }

  //Dont make this very large or watchdog will reset
  delay(250);
}


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

    case READOUT_raw_voltage:
      sendUnsignedInt(last_raw_adc);
      break;

    case READOUT_error_counter:
      sendUnsignedInt(error_counter);
      break;

    case READOUT_bypass_voltage_measurement:
      sendUnsignedInt(voltageMeasurement_bypass);
      break;

    case READOUT_bypass_enabled_state:
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

    case READOUT_load_resistance:
      sendFloat(currentConfig.bypassResistance);
      break;

    default:
      //Dont do anything - timeout
      break;
  }

  //Clear cmdByte
  cmdByte = 0;

  //Reset when we last processed a request, if this times out master has stopped communicating with module
  last_i2c_request = 150;
}


ISR(TIMER1_COMPA_vect)
{
  /////////////////Flash LED in sync with bit pattern
  if (green_pattern == 0) {
    if (ledFlash)  {
      ledON();
    }
    ledFlash = false;
  } else {
    ///Rotate pattern
    green_pattern = (byte)(green_pattern << 1) | (green_pattern >> 7);

    if (green_pattern & 0x01) {
      ledON();
    } else {
      ledOFF();
    }
  }
  ///////////////////////////////////////////////////
  
  if (bypassEnabled) {
    //This must go above the following "if (bypassCnt > 0)" statement...
    if (bypassCnt == 0 && analogValIndex == 0) {
      //We are in bypass and just finished an in-cycle voltage measurement
      voltageMeasurement_bypass = getVoltageMeasurement();

      if (targetBypassVoltage >= voltageMeasurement_bypass) {
        //We reached the goal
        bypass_off();
      } else {
        //Try again
        bypassCnt = BYPASS_COUNTER_MAX;
      }
    }

    if (bypassCnt > 0)
    {
      //We are in ACTIVE BYPASS mode -> BURNING ENERGY in resistor
      bypassCnt--;
      digitalWrite(PB4, HIGH);

      if (bypassCnt == 0)
      {
        //We have just finished this timed ACTIVE BYPASS mode, disable resistor
        //and measure resting voltage now before possible re-enable.
        digitalWrite(PB4, LOW);

        //Reset voltage ADC buffer
        analogValIndex = 0;
      }
    }

  } else {
    //Safety check we ensure bypass is always off if not enabled
    digitalWrite(PB4, LOW);
  }

  //trigger ADC reading
  ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect) {
  // Interrupt service routine for the ADC completion
  //uint8_t adcl = ADCL;
  //uint16_t value = ADCH << 8 | adcl;

  uint16_t value = ADCW;

  //If we skip this ADC reading, quit ISR here
  if (skipNextADC) {
    skipNextADC = false;
    return;
  }

  if (tempReadingCnt == TEMP_READING_LOOP_FREQ ) {
    //Use A0 (RESET PIN) to act as an analogue input
    //note that we cannot take the pin below 1.4V or the CPU resets
    //so we use the top half between 1.6V and 2.56V (voltage reference)
    //we avoid switching references (VCC vs 2.56V) so the capacitors dont have to keep draining and recharging
    tempReadingCnt = 0;

    //We reduce the value by 512 as we have a DC offset we need to remove
    tempSensorValue = value;

    // use ADC3 for input for next reading (voltage)
    ADMUX = B10010011;

    //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the ADC to settle after changing MUX
    skipNextADC = true;

  } else {

    //Populate the rolling buffer with values from the ADC
    last_raw_adc = value;
    analogVal[analogValIndex] = value;

    analogValIndex++;

    if (analogValIndex == OVERSAMPLE_LOOP) {
      analogValIndex = 0;
      buffer_ready = 1;
    }

    tempReadingCnt++;

    if (tempReadingCnt == TEMP_READING_LOOP_FREQ) {
      //use ADC0 for temp probe input on next ADC loop

      //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
      ADMUX = B10010000;

      //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the ADC to settle after changing MUX
      skipNextADC = true;
    }
  }

}


// function that executes whenever data is received from master
void receiveEvent(int bytesCnt) {
  if (bytesCnt <= 0) return;

  //If cmdByte is not zero then something went wrong in the i2c comms,
  //master failed to request any data after command
  if (cmdByte != 0) error_counter++;

  cmdByte = Wire.read();
  bytesCnt--;

  //Is it a command byte (there are other bytes to process) or not?
  if (bitRead(cmdByte, COMMAND_BIT)) {

    bitClear(cmdByte, COMMAND_BIT);

    switch (cmdByte) {
      case COMMAND_green_led_pattern:
        if (bytesCnt == 1) {
          green_pattern = Wire.read();
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

      case COMMAND_set_load_resistance:
        if (bytesCnt == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != currentConfig.bypassResistance) {
            currentConfig.bypassResistance = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_bypass_voltage:
        if (bytesCnt == sizeof(uint16_t)) {
          uint16_t newValue = readUINT16();
          //Only accept if its a realistic value and the value is LESS than the last voltage reading
          if (newValue >= MIN_BYPASS_VOLTAGE && newValue <= MAX_BYPASS_VOLTAGE && newValue < voltageMeasurement) {
            //TODO: Are we sure we really need all these variables??
            targetBypassVoltage = newValue;
            voltageMeasurement_bypass = voltageMeasurement;
            bypassCnt = BYPASS_COUNTER_MAX;
            green_pattern = GREEN_LED_PATTERN_BYPASS;
            bypassEnabled = true;
          } else {
            //Disable
            bypass_off();
          }
        }
        break;


      case COMMAND_set_slave_address:
        //Set i2c slave address and write to EEPROM, then reboot
        if (bytesCnt == 1 ) {
          uint8_t newAddress = Wire.read();
          //Only accept if its a different address
          if (newAddress != currentConfig.SLAVE_ADDR && newAddress >= DEFAULT_SLAVE_ADDR_START_RANGE && newAddress <= DEFAULT_SLAVE_ADDR_END_RANGE) {
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

  // clear rx buffer
  while (Wire.available()) Wire.read();
}
