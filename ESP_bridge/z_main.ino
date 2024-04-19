
void loop(){

  ArduinoOTA.handle();

  if (CheckTimer(tmrSendDataToServer, 15000UL)){
      CommWithServer(false);
      //Serial.println(ntp.formattedTime("%d. %B %Y")); // dd. Mmm yyyy
      //Serial.println(ntp.formattedTime("%A %T")); // Www hh:mm:ss
  }
  if(bridgeSerial.available()>0){
    Serial.println("RECEIVING from arduino");
  }
  while(bridgeSerial.available() > 0) {
    uint8_t rx = bridgeSerial.read();
    Serial.print(rx);
    Serial.print(";");
    
    read_buffer[buffer_ptr++] = rx;
    if (buffer_ptr>=BUFFER_SIZE){
      receive_state = 0;
      buffer_ptr = 0;
    }
    switch(receive_state){
      case 0:
        if(rx==111)
          receive_state++;
        else{
          receive_state=0;
          buffer_ptr = 0;
          invalid_packets1 ++;
        }
      break;
      case 1:
      if(rx==222){
          receive_state++;
      }else{
          receive_state=0;
          buffer_ptr = 0;
          invalid_packets1 ++;
        }
      break;
      case 2:
        expected_len = rx+5; //two start flags + crc two bytes + end flag
        receive_state++;
        ResetTimer(tmrTimeoutReceive);
        break;
      default:
        if(buffer_ptr-1 == expected_len){
          if(rx == 222){
              Serial.println("completed, going to send it to server");
              CommWithServer(true);
              ResetTimer(tmrSendDataToServer);
              receive_state=0;
              buffer_ptr = 0;
              ok_packets ++;
          }else{
            receive_state=0;
            buffer_ptr = 0;
            invalid_packets1 ++;
          }
        } else if (buffer_ptr-1 > expected_len){ // TODO add timeout, when nothing received for certain time
            receive_state=0;
            buffer_ptr = 0;   
            invalid_packets2++;       
        }else{
          if (CheckTimer(tmrTimeoutReceive, 100UL)){
            invalid_packets3++;  
          }
        }
      
    }
  }
 
}
