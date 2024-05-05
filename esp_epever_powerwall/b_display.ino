void HandleDisplay(){
  if(!displayOk)return;
  display.home();

  if(!connectedToWifi){
    display.print(F("Wifi error!     \n"));
  }
  else if(commOk)
    display.print(F("Comm ok         \n"));
  else
    display.print(F("Comm error!     \n"));
    
  if(!errorNotValidData)
    display.print(F("Data valid!     \n"));
  else
    display.print(F("!Data NOT valid!\n"));

  display.println(selected_device);
  display.print(F("errors: "));
  display.println(timeoutReq);
  display.print(F("crc errors: "));
  display.println(crcErrorCnt);
  display.println(epeverRegPtr2);
  //display.print(F("register: "));
  display.println(epeverStatus);
  
  

  /*display.print(F("Batt.vol:"));
  display.print(float(batVoltage)/100.0,1);
  display.println(F(" V"));

  display.print(F("Temp:"));
  display.print(float(temperature)/100.0,1);
  display.println(F(" C"));

  display.print(F("Batt.pow:"));
  display.print(float(batteryPower)/100.0,0);
  display.println(F(" W"));*/
  
  /*char str[3];
  itoa(55, str, 10);
  display.drawString(0,0,str);*/

}
