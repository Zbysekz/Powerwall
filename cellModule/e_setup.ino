void initADC();

static inline void initTimer1(void)
{
  TCCR1 |= (1 << CTC1);  // clear timer on compare match
  TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11) | (1 << CS10); //clock prescaler 16384
  OCR1C = 64;  //About eighth of a second trigger Timer1  (there are 488 counts per second @ 8mhz)
  TIMSK |= (1 << OCIE1A); // enable compare match interrupt
}

void init_i2c() {
  TinyWireS.begin(currentConfig.SLAVE_ADDR);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
}

void disable_i2c(){
  USICR = 0; //control register
  USISR = 0; //status register
}

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

  pinMode(PB4, OUTPUT); //bypass - burning resistor
  digitalWrite(PB4, LOW);

  ledON();
  delay(500);
  ledOFF();

  green_pattern = GREEN_LED_PATTERN_STANDARD;
    
  //Load our EEPROM configuration
  if (!LoadConfigFromEEPROM()) {
    badConfiguration = true;
  }
  if (badConfiguration || currentConfig.SLAVE_ADDR == DEFAULT_SLAVE_ADDR) {
    green_pattern = GREEN_LED_PATTERN_UNCONFIGURED;
    currentConfig.SLAVE_ADDR = DEFAULT_SLAVE_ADDR;//need to be here, compiler probably doesn't init struct properly
  }

  cli();//stop interrupts

  voltageBufIdx = 0;

  initTimer1();
  initADC();

  // WDTCSR configuration:     WDIE = 1: Interrupt Enable     WDE = 1 :Reset Enable
  // Enter Watchdog Configuration mode:
  WDTCR |= (1 << WDCE) | (1 << WDE);

  // Set Watchdog settings - 4000ms timeout
  WDTCR = (1 << WDIE) | (1 << WDE) | (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);

  // Enable Global Interrupts
  sei();

  init_i2c();
  
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
