void HandleDisplay(){
  
  display.clearDisplay();
  
  display.print(F("Network status:"));
  if(status_eth==1)
    display.print(F("ok"));
  else{ 
    display.println(F("error"));
    display.print(status_eth);
  }
  
  display.print(F("I2C status:"));
  if(status_i2c==1)
    display.print(F("ok"));
  else{
    display.println(F("error"));
    display.print(status_i2c);
  }
  
  display.print(F("No. of modules:"));
  char str[3];
  itoa(55, str, 10);
  display.drawString(0,0,str);

}
