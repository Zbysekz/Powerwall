#include "d_main.h"

void loop() {
  wdt_reset();

  if (last_i2c_request > 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    last_i2c_request--;
  }

  //If we are on the default SLAVE address then use different LED pattern to indicate this
  if (myConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
  } else {
    //We have had at least one i2c request and not currently in PANIC mode
    if (last_i2c_request == 0 && inPanicMode == false) {

      previousLedState = green_pattern ;

      //Panic after a few seconds of not receiving i2c requests...
      panic();

      //Try resetting the i2c bus
      Wire.end();
      init_i2c();

      error_counter++;
    }

    if (last_i2c_request > 0 && inPanicMode == true) {
      //Just come back from a PANIC situation
      LEDReset();
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
    case read_voltage:
      //If we are in bypass then only return the voltage
      //thats taken outside of the resistor load being switched on
      if (ByPassEnabled) {
        sendUnsignedInt(ByPassVCCMillivolts);
      } else {
        sendUnsignedInt(VCCMillivolts);
      }

      break;

    case read_raw_voltage:
      sendUnsignedInt(last_raw_adc);
      break;

    case read_error_counter:
      sendUnsignedInt(error_counter);
      break;

    case read_bypass_voltage_measurement:
      sendUnsignedInt(ByPassVCCMillivolts);
      break;

    case read_bypass_enabled_state:
      sendByte(ByPassEnabled);
      break;

    case read_temperature:
      sendUnsignedInt((uint16_t)((float)temperature_probe * myConfig.TemperatureCalibration));
      break;

    case read_voltage_calibration:
      sendFloat(myConfig.VCCCalibration);
      break;

    case read_temperature_calibration:
      sendFloat(myConfig.TemperatureCalibration);
      break;

    case read_load_resistance:
      sendFloat(myConfig.LoadResistance);
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
  //Flash LED in sync with bit pattern


  if (green_pattern == 0) {
    if (flash_green)  {
      ledGreen();
    }
    flash_green = false;
  } else {
    ///Rotate pattern
    green_pattern = (byte)(green_pattern << 1) | (green_pattern >> 7);

    if (green_pattern & 0x01) {
      ledGreen();
    } else {
      ledOff();
    }
  }

  if (ByPassEnabled) {

    //TODO: We need to add in code here to check we don't overheat

    //This must go above the following "if (ByPassCounter > 0)" statement...
    if (ByPassCounter == 0 && analogValIndex == 0) {
      //We are in bypass and just finished an in-cycle voltage measurement
      ByPassVCCMillivolts = Update_VCCMillivolts();

      if (targetByPassVoltage >= ByPassVCCMillivolts) {
        //We reached the goal
        bypass_off();
      } else {
        //Try again
        ByPassCounter = BYPASS_COUNTER_MAX;
      }
    }

    if (ByPassCounter > 0)
    {
      //We are in ACTIVE BYPASS mode - the RESISTOR will be ACTIVE + BURNING ENERGY
      ByPassCounter--;
      digitalWrite(PB4, HIGH);

      if (ByPassCounter == 0)
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

  if (green_pattern == 0) {
    ledOff();
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

  if (reading_count == TEMP_READING_LOOP_FREQ ) {
    //Use A0 (RESET PIN) to act as an analogue input
    //note that we cannot take the pin below 1.4V or the CPU resets
    //so we use the top half between 1.6V and 2.56V (voltage reference)
    //we avoid switching references (VCC vs 2.56V) so the capacitors dont have to keep draining and recharging
    reading_count = 0;

    //We reduce the value by 512 as we have a DC offset we need to remove
    temperature_probe = value;

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

    reading_count++;

    if (reading_count == TEMP_READING_LOOP_FREQ) {
      //use ADC0 for temp probe input on next ADC loop

      //We have to set the registers directly because the ATTINYCORE appears broken for internal 2v56 register without bypass capacitor
      ADMUX = B10010000;

      //Set skipNextADC to delay the next TIMER1 call to ADC reading to allow the ADC to settle after changing MUX
      skipNextADC = true;
    }
  }

}


// function that executes whenever data is received from master
void receiveEvent(int howMany) {
  if (howMany <= 0) return;

  //If cmdByte is not zero then something went wrong in the i2c comms,
  //master failed to request any data after command
  if (cmdByte != 0) error_counter++;

  flashLed = true;

  cmdByte = Wire.read();
  howMany--;

  //Is it a command byte (there are other bytes to process) or not?
  if (bitRead(cmdByte, COMMAND_BIT)) {

    bitClear(cmdByte, COMMAND_BIT);

    switch (cmdByte) {
      case COMMAND_green_led_pattern:
        if (howMany == 1) {
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
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.VCCCalibration) {
            myConfig.VCCCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_temperature_calibration:
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.TemperatureCalibration) {
            myConfig.TemperatureCalibration = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_load_resistance:
        if (howMany == sizeof(float)) {
          float newValue = readFloat();
          //Only accept if its different
          if (newValue != myConfig.LoadResistance) {
            myConfig.LoadResistance = newValue;
            WriteConfigToEEPROM();
          }
        }
        break;

      case COMMAND_set_bypass_voltage:
        if (howMany == sizeof(uint16_t)) {
          uint16_t newValue = readUINT16();
          //Only accept if its a realistic value and the value is LESS than the last voltage reading
          if (newValue >= MIN_BYPASS_VOLTAGE && newValue <= MAX_BYPASS_VOLTAGE && newValue < VCCMillivolts) {
            //TODO: Are we sure we really need all these variables??
            targetByPassVoltage = newValue;
            ByPassVCCMillivolts = VCCMillivolts;
            ByPassCounter = BYPASS_COUNTER_MAX;
            green_pattern = GREEN_LED_PATTERN_BYPASS;
            ByPassEnabled = true;
          } else {
            //Disable
            bypass_off();
          }
        }
        break;


      case COMMAND_set_slave_address:
        //Set i2c slave address and write to EEPROM, then reboot
        if (howMany == 1 ) {
          uint8_t newAddress = Wire.read();
          //Only accept if its a different address
          if (newAddress != myConfig.SLAVE_ADDR && newAddress >= DEFAULT_SLAVE_ADDR_START_RANGE && newAddress <= DEFAULT_SLAVE_ADDR_END_RANGE) {
            myConfig.SLAVE_ADDR = newAddress;
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
      case read_voltage:
        VCCMillivolts = Update_VCCMillivolts();
        flash_green = true;
        break;
    }
  }

  // clear rx buffer
  while (Wire.available()) Wire.read();
}
