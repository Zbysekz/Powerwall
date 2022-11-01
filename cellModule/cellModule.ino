/*
   ____  ____  _  _  ____  __  __  ___
  (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
   )(_) )_)(_  \  /  ) _ < )    ( \__ \
  (____/(____) (__) (____/(_/\/\_)(___/
  (c) 2017 Stuart Pittaway - original author
  (c) 2020 Zbysek Zapadlik - overall remake, bugfixes, replaced wire with i2c library with timeouts, code divided to tabs ...
  
  This is the code for the cell module (one is needed for each series cell in a modular battery array (pack))
  This code runs on ATTINY85 processors and compiles with Arduino 1.8.13 environment
  You will need a seperate programmer to program the ATTINY chips - another Arduino can be used
  Settings ATTINY85, 8MHZ INTERNAL CLOCK, LTO enabled, BOD disabled, Timer1=CPU
  Use this board manager for ATTINY85
  http://drazzy.com/package_drazzy.com_index.json
  not this one https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
  ATTINY85/V-10PU data sheet
  http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf
  To manually configure ATTINY fuses use "avrdudess" sw (GUI)
  Efuse: 0xff Hfuse:0xd5 Lfuse0xe2
  If you burn incorrect fuses to ATTINY85 you may need to connect a crystal over the pins to make it work again!
  
*/
