// Interrupt service routine for the ADC completion
// Sequence of reading is like:

// voltageBuff[0] = ADC;
// voltageBuff[1] = ADC;
// ....
// voltageBuff[OVERSAMPLE_LOOP-1] = ADC;

// then anytime, when read counter reaches TEMP_READING_LOOP_FREQ, temperature is read, like:

// changed MUX
// skipping this ADC reading
// temperature = ADC; + changing MUX + next reading will be skipped


ISR(ADC_vect) {
  
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
    voltageBuff[voltageBufIdx] = value;

    voltageBufIdx++;

    if (voltageBufIdx == OVERSAMPLE_LOOP) {
      voltageBufIdx = 0;
      voltageBufferReady = 1;// indicates valid data, used in main logic
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
