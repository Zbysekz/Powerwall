
void loop() {
  wdt_reset();

  if (last_i2c_request > 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    last_i2c_request--;
  }

  //If we are on the default SLAVE address signalize it
  if (badConfiguration || currentConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
  } else {
    HandlePanicMode();//reset i2c bus if no communication goin on for some time
  }

  if(inPanicMode)
    bypass_off();

  //Don't make this very large or watchdog will reset
  delay(250);
}

void HandlePanicMode(){
  if (last_i2c_request == 0 && inPanicMode == false) {//go to panic mode, when timeout of I2C requests
      
    green_pattern = GREEN_LED_PANIC;
    inPanicMode = true;
    //Try resetting the i2c bus
    Wire.end();
    init_i2c();

    error_counter++;
  }

  if (last_i2c_request > 0 && inPanicMode == true) {//return from panic mode
    green_pattern = GREEN_LED_PATTERN_STANDARD;
    inPanicMode = false;
  }
}


ISR(TIMER1_COMPA_vect) // timer interrupt
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
