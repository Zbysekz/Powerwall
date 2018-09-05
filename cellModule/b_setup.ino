void setup() {
  //Must be first line of setup()
  MCUSR &= ~(1 << WDRF); // reset status flag
  wdt_disable();

  //Pins PB0 (SDA) and PB2 (SCLOCK) are for the i2c comms with master
  //PINS https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md

  //pinMode(A3, INPUT);  //A3 pin 3 (PB3)
  //pinMode(A0, INPUT);  //reset pin A0 pin 5 (PB5)

  /*
    DDRB &= ~(1 << DDB3);
    PORTB &= ~(1 << PB3);
    DDRB &= ~(1 << DDB5);
    PORTB &= ~(1 << PB5);
  */

  pinMode(PB4, OUTPUT); //PB4 = PIN 3
  digitalWrite(PB4, LOW);

  ledGreen();
  delay(500);
  ledOff();

  //Load our EEPROM configuration
  if (!LoadConfigFromEEPROM()) {
    //Do something here for bad configuration??
  }

  cli();//stop interrupts

  analogValIndex = 0;

  initTimer1();
  initADC();

  // WDTCSR configuration:     WDIE = 1: Interrupt Enable     WDE = 1 :Reset Enable
  // Enter Watchdog Configuration mode:
  WDTCR |= (1 << WDCE) | (1 << WDE);

  // Set Watchdog settings - 4000ms timeout
  WDTCR = (1 << WDIE) | (1 << WDE) | (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);

  // Enable Global Interrupts
  sei();

  LEDReset();
  wait_for_buffer_ready();
  LEDReset();

  init_i2c();
}


static inline void initTimer1(void)
{
  TCCR1 |= (1 << CTC1);  // clear timer on compare match
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11) | (1 << CS10); //clock prescaler 16384
  OCR1C = 64;  //About eighth of a second trigger Timer1  (there are 488 counts per second @ 8mhz)
  TIMSK |= (1 << OCIE1A); // enable compare match interrupt
}
void init_i2c() {
  Wire.begin(myConfig.SLAVE_ADDR);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}
