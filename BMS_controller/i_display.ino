void HandleDisplay(){
  if(!displayOk)return;
  display.home();
  Serial.println(F("After home"));

  if(CheckTimer(tmrPageChange, 5000L)){
    if(showPage<1)showPage++;else showPage=0;
    display.clearDisplay();
  }

  switch(showPage){
    case 0:
      GeneralOverview();
    break;
    case 1:
      ModuleStats();
    break;
  }

  /*char str[3];
  itoa(55, str, 10);
  display.drawString(0,0,str);*/

}

void GeneralOverview(){
  display.print(F("\n\nEth status:\n"));
  if(status_eth==1){
    display.print(F("ok\n"));
  }else{ 
    display.print(F("error:"));
    display.println(status_eth);
  }
  
  display.print(F("I2C status:\n"));
  if(status_i2c==1){
    display.println(F("ok\n"));
  }else{
    display.print(F("error:"));
    display.println(status_i2c);
  }
  
  display.print(F("Modules cnt:"));
  display.println(modulesCount);

}

void ModuleStats(){
  display.print(F("\nVoltage|temp\n"));
  for(uint8_t i=0;i<6;i++){
    display.print(moduleList[i].voltage/100.0,2);
    display.print(F("    "));
    display.println(moduleList[i].temperature/10.0,1);
  }
}
