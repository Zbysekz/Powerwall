/*
   ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/
  (c) 2017 Stuart Pittaway
  This is the code for the cell module (one is needed for each series cell in a modular battery array (pack))
  This code runs on ATTINY85 processors and compiles with Arduino 1.8.13 environment
  You will need a seperate programmer to program the ATTINY chips - another Arduino can be used
  Settings ATTINY85, 8MHZ INTERNAL CLOCK, LTO enabled, BOD disabled, Timer1=CPU
  Use this board manager for ATTINY85
  http://drazzy.com/package_drazzy.com_index.json
  not this one https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
  ATTINY85/V-10PU data sheet
  http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
  To manually configure ATTINY fuses use the command....
  .\bin\avrdude -p attiny85 -C etc\avrdude.conf -c avrisp -P COM5 -b 19200 -B2 -e -Uefuse:w:0xff:m -Uhfuse:w:0xdf:m -Ulfuse:w:0xe2:m
  If you burn incorrect fuses to ATTINY85 you may need to connect a crystal over the pins to make it work again!

  (c) 2020 Zbysek Zapadlik
  General code improvements
  Code is divided to sections - arduino IDE compiles files alphabetically.
*/
