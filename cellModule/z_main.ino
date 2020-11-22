
void loop() {
  wdt_reset();
  
  if (i2cTmr > 0) {
    //Count down loop for requests to see if i2c bus hangs or controller stops talking
    i2cTmr--;
  }

  //If we are on the default SLAVE address signalize it
  if (!badConfiguration && currentConfig.SLAVE_ADDR != DEFAULT_SLAVE_ADDR) {
    HandlePanicMode();//reset i2c bus if no communication going on for some time
  }

  if(inPanicMode)
    bypass_off();

  //Don't make this very large or watchdog will reset
  delay(250);
}

void HandlePanicMode(){
  if (i2cTmr == 0 && inPanicMode == false) {//go to panic mode, when timeout of I2C requests

    green_pattern = GREEN_LED_PANIC;
    inPanicMode = true;
    //Try resetting the i2c bus
    Wire.end();
    init_i2c();

    error_counter++;
  }

  if (i2cTmr > 0 && inPanicMode == true) {//return from panic mode
    green_pattern = GREEN_LED_PATTERN_STANDARD;
    inPanicMode = false;
  }
}


ISR(TIMER1_COMPA_vect) // timer interrupt
{
  /////////////////Flash LED in sync with bit pattern
  if (green_pattern == 0) {//if not showing any pattern
    if (ledFlash)  {
      ledON();
    }else ledOFF();
    ledFlash = false;
  } else {
    green_pattern = (byte)(green_pattern << 1) | (green_pattern >> 7);//rotate pattern left

    if (green_pattern & 0x01) {
      ledON();
    } else {
      ledOFF();
    }
  }
  ///////////////////////////////////////////////////
  
  if (bypassEnabled) {// burning energy in resistor
    if(iBurningCounter<65535)iBurningCounter++;//counting burned Wh, reset by reading
    
    if (bypassCnt == 0 && voltageBufferReady) {
      //We are in bypass and just filled in whole buffer with voltage measurements
      voltageMeasurement_bypass = getVoltageMeasurement();

      if (targetBypassVoltage >= voltageMeasurement_bypass) {//We have reached the goal
        bypass_off();
        green_pattern = GREEN_LED_PATTERN_STANDARD;
      } else { //Keep burning
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
        voltageBufIdx = 0;//we want whole one cycle
        voltageBufferReady = false;
      }
    }

  } else {
    digitalWrite(PB4, LOW);//Safety check we ensure bypass is always off if not enabled
  }

  //trigger ADC reading
  ADCSRA |= (1 << ADSC);
}
